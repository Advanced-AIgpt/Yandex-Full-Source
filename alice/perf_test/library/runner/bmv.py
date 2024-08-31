import logging
import os.path
import requests
import signal
import subprocess
import sys

logger = logging.getLogger(__name__)


class AliceServer:
    def __init__(self, port, vins_package_path, log_dir, run_all, url_requester=None):
        self.port = port
        if run_all:
            args = [
                os.path.join(vins_package_path, 'launch', 'launch'),
                'run',
                '-p', str(self.port),
                '-a',
                '-w', vins_package_path,
                '-l', log_dir
            ]
        else:
            args = [
                os.path.join(vins_package_path, 'launch', 'launch'),
                'megamind_only',
                '-p', str(self.port),
                '-w', vins_package_path,
                '-l', log_dir,
            ]

        with open(os.path.join(log_dir, 'launch'), 'w+') as file_out:
            self._io = subprocess.Popen(args, stdout=file_out, stderr=file_out)
        if url_requester:
            resp = url_requester(self.make_url('ping'))
            if resp != 'pong':
                if self._io.poll():
                    raise Exception('Unable to start megamind server: %s', self._io.communicate()[1])
                raise Exception('Unable to start megamind server (consider to raise ping/pong tries): %s', resp)

    def stop(self):
        self._io.send_signal(signal.SIGINT)
        try:
            self._io.wait(30)
        except subprocess.TimeoutExpired:
            logger.error('Unable to shutdown megamind server: %s', resp.content)

    def make_url(self, path):
        return 'http://localhost:{}/{}'.format(self.port, path)

    def get_address(self):
        return 'localhost:{}'.format(self.port)
