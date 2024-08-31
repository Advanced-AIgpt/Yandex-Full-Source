import logging
import socket

from alice.library.python.utils.network import wait_port
from yatest import common as yc


logger = logging.getLogger(__name__)


class Bass:
    _dir = 'alice/bass'
    _bass_err = 'bass.err'
    _bass_out = 'bass.out'
    _testing_out_stuff = 'test-results/py3test/testing_out_stuff'

    def __init__(self, port, config=None, wait_port=True):
        self._port = port
        self._config_path = config or yc.source_path(f'{self._dir}/configs/localhost_config.json')
        self._wait_port = wait_port

    def __enter__(self):
        logger.info(f'Starting {Bass.__name__} with config {self._config_path}')
        binary_path = yc.binary_path(f'{self._dir}/scripts/run/run')
        cmd_params = [
            binary_path,
            '--server', yc.binary_path(f'{self._dir}/bin/bass_server'),
            '-p', str(self.port),
            self._config_path,
        ]
        self._process = yc.process.execute(
            cmd_params,
            wait=False,
            stderr=yc.output_path(self._bass_err),
            stdout=yc.output_path(self._bass_out),
        )

        if self._wait_port:
            self.wait_port()

        return self

    def __exit__(self, type, value, traceback):
        self._process.terminate()
        self._process.wait()

    def wait_port(self):
        wait_port(Bass.__name__, self._port, timeout_seconds=300,
                  message_if_failure=f'\nWHAT TO DO:'
                  f'\n  1) Look for errors in the log <TESTING_OUT_STUFF>/{self._bass_err}'
                  '\n  2) If this doesn\'t help try to start the service manually (see exact command in the '
                  '<TESTING_OUT_STUFF>/run.log) and see if it succeeds.')

    @property
    def port(self):
        return self._port

    @staticmethod
    def port_count():
        return 1

    @property
    def srcrwr(self):
        return f'BASS:{socket.gethostname()}:{self.port}'
