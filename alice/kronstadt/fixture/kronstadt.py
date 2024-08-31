import logging
import os
import socket
import tarfile
import requests
import sys
import time

from alice.library.python.utils.network import wait_port
from yatest import common as yc


logger = logging.getLogger(__name__)


def get_scenario(data):
    assert data is None or isinstance(data, (str)), 'Scenarios must be given by str or None'
    return data if isinstance(data, str) else data or ''


class Kronstadt:
    def __init__(self, http_port, grpc_port, scenarios=None, wait_port=True):
        self._http_port = http_port
        self._grpc_port = grpc_port
        self._scenario = get_scenario(scenarios)
        self._wait_port = wait_port
        # todo: change me later - sick thing - cannot provide needed sdk on pytest
        with tarfile.open(yc.binary_path(f'alice/kronstadt/scenarios/{self._scenario}/it2/shard/jdk.tar')) as jdk_tar_file:
            try:
                jdk_tar_file.extractall('/tmp/jdk')
            except Exception as error:
                logger.info(f'Error while extracting jdk tar: {error}')
            finally:
                jdk_tar_file.close()
        logger.info(f'Starting Kronstadt shard on http_port: {http_port}; grpc_port: {grpc_port}')

    def __enter__(self):
        os.environ['JAVA_HOME'] = '/tmp/jdk'
        binary_path = yc.binary_path(f'alice/kronstadt/scenarios/{self._scenario}/it2/shard/run.sh')

        cmd_params = [
            binary_path,
            '-XX:+HeapDumpOnOutOfMemoryError',
            '-Dfile.encoding=UTF-8',
            '-XX:+PrintCommandLineFlags',
            '-XX:+AlwaysActAsServerClassMachine',
            '-XX:+UseShenandoahGC',
            '-XX:MaxGCPauseMillis=10',
            '-XX:ActiveProcessorCount=2',
            '-XX:+DisableExplicitGC',
            '-Xms2G',
            '-Xmx2G',
            '--add-opens=java.base/java.lang=ALL-UNNAMED',
            f'-Dserver.port={str(self.http_port)}',
            f'-Dapphost.port={str(self.grpc_port)}',
            '-Dtvm.mode=disabled',
            '-Dspring.profiles.active=dev',
            '-Dlogging.config=classpath:log4j2-dev.xml',
            '-Dtests.it2=true'
        ]

        if yc.get_param('kronstadt_debug', False):
            print("Kronstadt is started in debug mode. Connect with debugger to proceed.", file=sys.stderr)
            cmd_params.append('-agentlib:jdwp=transport=dt_socket,server=y,suspend=y,address=*:5005')

        if yc.get_param('kronstadt_additional_params'):
            params = yc.get_param('kronstadt_additional_params')
            print(f'Kronstadt additional parameters provided: {params}', file=sys.stderr)
            cmd_params.append(params)

        cmd_params.append('ru.yandex.alice.kronstadt.runner.KronstadtApplication')

        self._process = yc.process.execute(
            cmd_params,
            wait=False,
            cwd=yc.binary_path(f'alice/kronstadt/scenarios/{self._scenario}/it2/shard'),
            stderr=yc.output_path('kronstadt.err'),
            stdout=yc.output_path('kronstadt.out'),
        )
        if self._wait_port:
            self.wait_port()
        return self

    def __exit__(self, type, value, traceback):
        try:
            logger.info(f'Executing graceful shutdown script for port {self.http_port}')
            url = f'http://localhost:{self.http_port}/actuator/shutdown'
            resp = requests.post(url)
            assert resp.status_code == 200
            self._process.wait(timeout=100)
        except:
            try:
                self._process.terminate()
                self._process.wait(timeout=10)
            finally:
                raise RuntimeError('Graceful shutdown failed')

    def wait_port(self):
        wait_port('KronstadtServer', self.http_port, timeout_seconds=300, sleep_between_attempts_seconds=1)
        self.wait_healthcheck('KronstadtServer')

    def wait_healthcheck(self, service_name, timeout_seconds=30, sleep_between_attempts_seconds=0.5, message_if_failure=None):
        start_time = time.time()
        logger.info(f'Waiting for {service_name} healthcheck {self.http_port}...')
        url = f'http://localhost:{self.http_port}/actuator/health/readiness'
        while True:
            elapsed_seconds = time.time() - start_time
            if elapsed_seconds > timeout_seconds:
                if not message_if_failure:
                    message_if_failure = f'Look for errors in the {service_name} log.'
                raise Exception(f'Service {service_name} can''t pass health check. {message_if_failure}')
            resp = requests.get(url)
            if resp.status_code == 200:
                break

    @property
    def http_port(self):
        return self._http_port

    @property
    def grpc_port(self):
        return self._grpc_port

    @property
    def port(self):
        return self.grpc_port

    @property
    def name(self):
        return 'Kronstadt'

    @property
    def srcrwr(self):
        # todo: move to general apphost backend
        return f'KRONSTADT_ALL:{socket.gethostname()}:{self.port}'
