import os.path
import socket
import subprocess
import sys
import time

import pytest
from yatest import common
from yatest.common import network


def try_connect(port):
    s = socket.socket()
    try:
        s.connect(('127.0.0.1', port))
        return True
    except socket.error as e:
        return False


def wait_for_port(port, n_attempts=10, sleep_internal=0.1):
    connected = False
    attempts = 0
    while not connected and attempts < n_attempts:
        connected = try_connect(port)
        if connected:
            return
        else:
            time.sleep(sleep_internal)
    raise RuntimeError(f'Falied to read port in {n_attempts * sleep_internal} seconds')


class GranetServerInstance:

    def __init__(self, binary_path, config_path, port, cwd):
        self.binary_path = binary_path
        self.config_path = config_path
        self.port = port
        process_args = [
            self.binary_path,
            '--config', self.config_path,
            '--port', str(self.port)
        ]
        self.process = subprocess.Popen(
            process_args,
            cwd=cwd,
            start_new_session=True,
            stdout=sys.stdout,
            stderr=sys.stderr,
        )
        wait_for_port(self.port)

    def stop(self):
        self.process.terminate()
        self.process.wait()

    @property
    def url(self):
        return 'http://127.0.0.1:' + str(self.port)


@pytest.fixture
def granet_server():
    cwd = common.work_path()
    binary_path = common.binary_path('alice/paskills/granet_server/server/server')
    config_path = os.path.join(
        common.source_path('alice/paskills/granet_server/config'),
        'localhost.pb.txt',
    )
    with network.PortManager() as pm:
        port = pm.get_port()
    granet = GranetServerInstance(binary_path, config_path, port, cwd)
    yield granet
    granet.stop()


@pytest.fixture
def compile_grammar_url(granet_server):
    return granet_server.url + '/granet/compile'
