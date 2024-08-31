import logging
import os.path
import requests
import subprocess
import sys
import yatest.common
import yatest.common.network


logger = logging.getLogger(__name__)


class UnableToStartException(BaseException):
    pass


class MegamindServer:
    def __init__(self, non_requestable_test_name, joker, env_data, url_requester=None):
        megamind_bin = yatest.common.binary_path('alice/megamind/scripts/run/run')
        with yatest.common.network.PortManager() as pm:
            joker_host, joker_port = joker.host_port.split(sep=':', maxsplit=1)
            self.port = pm.get_port()
            args = [
                megamind_bin,
                '--test-mode',
                '-p', str(self.port),
                '-c', yatest.common.source_path(os.path.join(yatest.common.context.project_path, '..', 'configs', 'dev', 'megamind.pb.txt')),
                '--random-seed-salt', '12345',
                '--rtlog-filename', '/dev/null',
                '--via-proxy-host', joker_host,
                '--via-proxy-port', joker_port,
                '--via-proxy-headers', 'x-yandex-joker:{}'.format(joker.test_stub_id(non_requestable_test_name)),
            ]

            self._megamind = subprocess.Popen(args, stdout=sys.stdout, stderr=sys.stderr, env=env_data)
            if url_requester:
                resp = url_requester(self.make_url('ping'), check_condition=lambda: self.raise_if_terminated())
                if resp != 'pong':
                    if self._megamind.poll():
                        raise UnableToStartException('Unable to start megamind server (something is wrong with megamind_server): %s', self._megamind.communicate()[1])
                    # FIXME (petrk) Add config for tests and put there timeout for megamind starting process.
                    raise UnableToStartException('Unable to start megamind server (consider to raise ping/pong tries): %s', resp)

    def stop(self):
        resp = requests.get('http://localhost:{}/admin?action=shutdown'.format(self.port))
        logger.debug('megamind server stop response: %s', resp.content)
        try:
            self._megamind.wait(timeout=2)
        except subprocess.TimeoutExpired:
            logger.error('Unable to shutdown megamind server: %s', resp.content)

    def make_url(self, path):
        return 'http://localhost:{}/{}'.format(self.port, path)

    def raise_if_terminated(self):
        if self._megamind.poll():
            raise UnableToStartException('Unable to start megamind server: %s', self._megamind.communicate()[1])
