import logging
import os
import socket

from alice.library.python.testing.auth import auth
from alice.library.python.utils.network import wait_port
from alice.hollywood.scripts.graph_generator.library.graph_generator import generate_backends_patch
from library.python.vault_client.instances import Production as VaultClient
from yatest import common as yc


logger = logging.getLogger(__name__)


def _get_tvm_secret(secret_id=auth.BassDev):
    try:
        return VaultClient().get_version(secret_id)['value']['TVM2_SECRET']
    except Exception as e:
        raise Exception(
            f'Failed to get secret from Vault, reason: {e}.\n'
            f'Check whether you have access to secret https://yav.yandex-team.ru/secret/{secret_id}\n'
            f'Most likely you are not a part of abc:bassdevelopers group') from e


class AppHost:
    _app_host_launcher_err = 'app_host_launcher.err'
    _app_host_launcher_out = 'app_host_launcher.out'

    def __init__(self, port, disable_all_remote_backends=False,
                 launch_timeout_seconds=300):
        self._port = port
        self._disable_all_remote_backends = disable_all_remote_backends
        self._launch_timeout_seconds = launch_timeout_seconds

        self._local_app_host_dir = yc.output_path(f'local_app_host_dir_{port}')
        assert self._local_app_host_dir.isascii(), (
            f'AppHost dir is not allowed to have non-ascii chars: {self._local_app_host_dir}.\n'
            'If you are using @pytest.mark.parametrize in some tests with non-ascii param '
            'values, this could be a cause. Add id to your pytest.param as described\n'
            'https://wiki.yandex-team.ru/alice/hollywood/integrationtests/'
            '#ispolzovanieparametrizedljakomandkalisesrusskimibukvami'
        )

    def __enter__(self):
        logger.info(f'Starting {AppHost.__name__} with local_app_host_dir={self._local_app_host_dir}')
        arcadia_path = yc.source_path('')
        logger.info(f'Arcadia path is {arcadia_path}')

        if self._disable_all_remote_backends:
            generate_backends_patch(
                yc.source_path('apphost/conf/verticals/ALICE'),
                self.local_app_host_dir,
                self.devnull_port,
            )
            logger.info('Created backends_patch.json')

        app_host_launcher_binary_path = yc.binary_path('apphost/tools/app_host_launcher/app_host_launcher')
        cmd_params = [
            app_host_launcher_binary_path,
            'setup',
            '--local-arcadia-path', arcadia_path,
            '--port', str(self.port),
            '--force-yes',
            '--install-path', os.path.join(self.local_app_host_dir, ''),
        ]
        env = {'UNISTAT_DAEMON_PORT': str(self.unistat_daemon_port)}

        if self._disable_all_remote_backends:
            cmd_params.extend(['--local-resolve'])

        if not self._disable_all_remote_backends:
            # There is no acces to tvm api from ci, that is why in tests apphost
            # works without tvm support.
            env.update({'TVM_SECRET': _get_tvm_secret()})
            cmd_params.extend(['--tvm-id', '2000860'])

        cmd_params.extend([
            '--local-arcadia-path-for-binaries', yc.binary_path(''),
            'arcadia',
            '--vertical',
            'ALICE',
            '--configuration', 'ctype=test;geo=sas',
        ])
        if self._disable_all_remote_backends:
            cmd_params.extend([
                '--backends-patch-path', os.path.join(self.local_app_host_dir, 'ALICE/backends_patch.json'),
                '--load-only-used-backends',
            ])

        logger.info(f'Apphost command {" ".join(cmd_params)}')

        self._process = yc.process.execute(
            cmd_params,
            env=env,
            wait=False,
            stderr=yc.output_path(self._app_host_launcher_err),
            stdout=yc.output_path(self._app_host_launcher_out),
        )
        return self

    def __exit__(self, type, value, traceback):
        self._process.terminate()
        self._process.wait(check_exit_code=False)

    def wait_port(self):
        wait_port(AppHost.__name__, self.http_adapter_port, timeout_seconds=self._launch_timeout_seconds,
                  message_if_failure=f'\nWHAT TO DO:'
                  f'\n  1) Look for errors in the log <TESTING_OUT_STUFF>/{self._app_host_launcher_err}'
                  '\n  2) If this doesn\'t help try to start the service manually (see exact command in the '
                  '<TESTING_OUT_STUFF>/run.log) and see if it succeeds.')

    @property
    def host(self):
        return socket.gethostname()

    @property
    def port(self):
        '''
        This is the "base port".
        '''
        return self._port + 1  # Because app_host uses `base_port - 1` too https://a.yandex-team.ru/arc_vcs/apphost/docs/pages/apphost_ports.md?rev=r8986857

    @property
    def grpc_port(self):
        return self.port + 1

    @property
    def http_adapter_port(self):
        return self.port + 4

    @property
    def devnull_port(self):
        return self.port + 5

    @property
    def unistat_daemon_port(self):
        return self.port + 7

    @property
    def local_app_host_dir(self):
        return self._local_app_host_dir

    @staticmethod
    def port_count():
        return 9

    @property
    def url(self):
        return f'http://{self.host}:{self.http_adapter_port}/speechkit/app/pa/'

    @property
    def apply_url(self):
        return f'http://{self.host}:{self.http_adapter_port}/speechkit/apply/'
