import logging
import socket

from alice.library.python.utils.network import wait_port
from yatest import common as yc


logger = logging.getLogger(__name__)


class Megamind:
    _dir = 'alice/megamind'
    _megamind_err = 'megamind.err'
    _megamind_out = 'megamind.out'
    _testing_out_stuff = 'test-results/py3test/testing_out_stuff'

    def __init__(self, port, config=None, additional_cmd_params=[], wait_port=True):
        self._port = port
        self._config_path = config or yc.source_path(f'{self._dir}/configs/dev/megamind.pb.txt')
        self._additional_cmd_params = additional_cmd_params
        self._wait_port = wait_port

    def __enter__(self):
        logger.info(f'Starting {Megamind.__name__} with config {self._config_path}')
        binary_path = yc.binary_path(f'{self._dir}/scripts/run/run')
        cmd_params = [
            binary_path,
            '--server', yc.binary_path(f'{self._dir}/server/megamind_server'),
            '-c', self._config_path,
            '-p', str(self.port),
        ] + self._additional_cmd_params

        self._process = yc.process.execute(
            cmd_params,
            wait=False,
            stderr=yc.output_path(self._megamind_err),
            stdout=yc.output_path(self._megamind_out),
        )

        if self._wait_port:
            self.wait_port()

        return self

    def __exit__(self, type, value, traceback):
        self._process.terminate()
        self._process.wait()

    def wait_port(self):
        wait_port(Megamind.__name__, self._port, timeout_seconds=120,
                  message_if_failure=f'\nWHAT TO DO:'
                  f'\n  1) Look for errors in the log <TESTING_OUT_STUFF>/{self._megamind_err}'
                  '\n  2) If this doesn\'t help try to start the service manually (see exact command in the '
                  '<TESTING_OUT_STUFF>/run.log) and see if it succeeds.')

    @property
    def port(self):
        return self._port

    @property
    def grpc_port(self):
        return self.port + 3

    @staticmethod
    def port_count():
        return 4

    @property
    def srcrwr(self):
        return f'MEGAMIND_ALIAS:{socket.gethostname()}:{self.grpc_port}'
