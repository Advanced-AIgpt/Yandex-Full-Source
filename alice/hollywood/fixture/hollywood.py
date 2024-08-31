import logging
import socket
import urllib.request

from alice.library.python.utils.network import wait_port
from alice.library.python.testing.auth import auth
from yatest_lib.ya import TestMisconfigurationException
from library.python.vault_client.instances import Production as VaultClient
from yatest import common as yc


logger = logging.getLogger(__name__)


def _check_scenarios(data):
    assert data is None or isinstance(data, (list, str)), 'Scenarios must be given by list, str or None'
    return [data] if isinstance(data, str) else data or []


def _load_environment(secret_id=auth.BassDev):
    try:
        return VaultClient().get_version(secret_id)['value']
    except Exception as e:
        logger.info(
            f'Failed to get token from Vault secret: {e}.\n'
            f'It is OK if you are in CI, otherwise check whether you have access to secret "{secret_id}"'
        )
        return {'MUSICKIT_HLS_SECRET_KEY': 'dummy'}


class Hollywood:
    _hollywood_err = 'hollywood.err'
    _hollywood_out = 'hollywood.out'
    _testing_out_stuff = 'test-results/py3test/testing_out_stuff'

    def __init__(self, port, scenarios=None, shard='all', config=None, wait_port=True):
        self._port = port
        self._scenarios = _check_scenarios(scenarios)
        self._shard = shard
        self._shard_dir = f'alice/hollywood/shards/{self._shard}'
        self._wait_port = wait_port

        hw_config = 'test' if self._scenarios else 'dev'
        self._config_path = config or yc.source_path(f'{self._shard_dir}/{hw_config}/hollywood.pb.txt')

    def __enter__(self):
        logger.info(f'Starting {Hollywood.__name__} shard {self._shard} with config {self._config_path}')
        binary_path = yc.binary_path(f'{self._shard_dir}/server/hollywood_server')
        cmd_params = [
            binary_path,
            '-c', self._config_path,
            '--app-host-config-port', str(self.port),
            '--use-signal-filter',
        ]

        def try_path(key: str, path: str):
            try:
                fast_data_path = yc.binary_path(path)
                cmd_params.extend([key, fast_data_path])
            except TestMisconfigurationException as e:
                logger.info(e)

        try_path('--fast-data-path', f'{self._shard_dir}/prod/fast_data')
        try_path('--scenario-resources-path', f'{self._shard_dir}/prod/resources')
        try_path('--common-resources-path', f'{self._shard_dir}/prod/common_resources')
        try_path('--hw-services-resources-path', f'{self._shard_dir}/prod/hw_services_resources')

        for scenario in self._scenarios:
            cmd_params.extend(['--scenarios', scenario])

        self._process = yc.process.execute(
            cmd_params,
            env=_load_environment(),
            wait=False,
            stderr=yc.output_path(self._hollywood_err),
            stdout=yc.output_path(self._hollywood_out),
        )
        logger.info(f'Starting Hollywood, pid {self._process.process.pid}')

        if self._wait_port:
            self.wait_port()

        return self

    def __exit__(self, type, value, traceback):
        try:
            url = f'http://localhost:{self.port}/admin?action=shutdown'
            resp = urllib.request.urlopen(url)
            assert resp.status == 200
            self._process.wait(timeout=10)
        except:
            try:
                self._process.terminate()
                self._process.wait(timeout=10)
            finally:
                raise RuntimeError('Graceful shutdown failed')

    def wait_port(self):
        wait_port(Hollywood.__name__, self._port, timeout_seconds=300, sleep_between_attempts_seconds=1,
                  message_if_failure=f'\nWHAT TO DO:'
                  f'\n  1) Look for errors in the log <TESTING_OUT_STUFF>/{self._hollywood_err}'
                  '\n  2) If this doesn\'t help try to start the service manually (see exact command in the '
                  '<TESTING_OUT_STUFF>/run.log) and see if it succeeds.')

    @property
    def port(self):
        return self._port

    @property
    def name(self):
        return 'Hollywood'

    @property
    def grpc_port(self):
        return self.port + 1

    @staticmethod
    def port_count():
        return 2

    @property
    def srcrwr(self):
        return f'HOLLYWOOD_{self._shard.upper()}:{socket.gethostname()}:{self.grpc_port}'
