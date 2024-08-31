import asyncio
import logging
import signal
import socket

import alice.library.python.utils as utils
import alice.hollywood.library.config.config_pb2 as hw_config
import alice.megamind.library.config.protos.config_pb2 as mm_config
from alice.library.python.utils.arcadia import arcadia_path
from alice.library.python.utils.network import wait_port
from library.python.vault_client.instances import Production as VaultClient


def _get_tvm_secret(secret_id='sec-01cnbk6vvm6mfrhdyzamjhm4cm'):
    try:
        return VaultClient().get_version(secret_id)['value']['TVM2_SECRET']
    except Exception as e:
        raise Exception(
            f'Failed to get secret from Vault, reason: {e}.\n'
            f'Check whether you have access to secret https://yav.yandex-team.ru/secret/{secret_id}\n'
            f'Most likely you are not a part of abc:bassdevelopers group') from e


class AppHost:
    def __init__(self, port):
        self._port = port

    async def run(self):
        binary = arcadia_path('apphost/tools/app_host_launcher/app_host_launcher')
        if not binary.is_file():
            raise OSError(f'Cannot find {binary.name}. Please run `ya make -A -DAP/DALL`')

        cmd = [
            str(binary),
            'setup',
            '--nora',
            '--port', str(self.port),
            '--tvm-id', '2000860',
            '--force-yes',
            '--install-path', 'local_app_host_dir/',
            'arcadia',
            '--vertical', 'ALICE',
            '--target-ctype', 'test',
        ]
        env = {'UNISTAT_DAEMON_PORT': str(self.unistat_daemon_port)}
        env.update({'TVM_SECRET': _get_tvm_secret()})
        try:
            logging.info(f'Starting {AppHost.__name__}: {cmd}')
            self._process = await asyncio.create_subprocess_exec(*cmd, env=env)
            wait_port(AppHost.__name__, self.port, timeout_seconds=120)
            await self._process.wait()
        except asyncio.CancelledError:
            self._process.send_signal(signal.SIGINT)
            await self._process.wait()

    @property
    def port(self):
        return self._port + 1

    @property
    def grpc_port(self):
        return self.port + 1

    @property
    def http_adapter_port(self):
        return self.port + 4

    @property
    def unistat_daemon_port(self):
        return self.port + 7

    @property
    def url(self):
        return f'http://{socket.gethostname()}:{self.http_adapter_port}/speechkit/app/pa/'


class Megamind:
    _dir = arcadia_path('alice', 'megamind')

    def __init__(self, port=None, config=None):
        self._config_path = config or self._dir/'configs'/'dev'/'megamind.pb.txt'
        self._config = utils.load_file(self._config_path, mm_config.TConfig())
        self._port = port or self._config.AppHost.HttpPort

    async def run(self):
        binary = self._dir/'scripts'/'run'/'run'
        if not binary.is_file():
            raise OSError(f'Cannot find {binary.name}. Please run `ya make -A -DMM/DALL`')

        cmd = [
            str(binary),
            '-c', str(self._config_path),
            '-p', str(self.port),
            '--logs', 'logs',
            '--run-unified-agent',
        ]
        try:
            logging.info(f'Starting {Megamind.__name__}: {cmd}')
            self._process = await asyncio.create_subprocess_exec(*cmd)
            wait_port(Megamind.__name__, self.port, timeout_seconds=120)
            await self._process.wait()
        except asyncio.CancelledError:
            self._process.send_signal(signal.SIGINT)
            await self._process.wait()

    @property
    def port(self):
        return self._port

    @property
    def grpc_port(self):
        return self.port + self._config.AppHost.GrpcPortOffset

    @property
    def srcrwr(self):
        return f'MEGAMIND_ALIAS:{socket.gethostname()}:{self.grpc_port}'


class Hollywood:
    _dir = arcadia_path('alice', 'hollywood')

    def __init__(self, shard, scenarios=[], port=None, config=None):
        self._shard = shard
        self._scenarios = scenarios
        shard_type = 'test' if self._scenarios else 'dev'
        self._config_path = config or self._dir/'shards'/self._shard/shard_type/'hollywood.pb.txt'
        self._config = utils.load_file(self._config_path, hw_config.TConfig())
        self._port = port or self._config.AppHostConfig.Port

    async def run(self):
        binary = self._dir/'scripts'/'run'/'run-hollywood-bin'
        if not binary.is_file():
            raise OSError(f'Cannot find {binary.name}. Please run `ya make -A -DHW/DALL`')

        cmd = [
            str(binary),
            '-c', str(self._config_path),
            '--app-host-config-port', str(self.port),
            '--logs', 'logs',
            '--run-unified-agent',
        ]
        for scenario in self._scenarios:
            cmd.extend(['--scenarios', scenario])
        try:
            logging.info(f'Starting {Hollywood.__name__}: {cmd}')
            self._process = await asyncio.create_subprocess_exec(*cmd)
            wait_port(Hollywood.__name__, self.port, timeout_seconds=120)
            await self._process.wait()
        except asyncio.CancelledError:
            self._process.send_signal(signal.SIGINT)
            await self._process.wait()

    @property
    def port(self):
        return self._port

    @property
    def grpc_port(self):
        return self.port + 1

    @property
    def srcrwr(self):
        return f'HOLLYWOOD_{self._shard.upper()}:{socket.gethostname()}:{self.grpc_port}'
