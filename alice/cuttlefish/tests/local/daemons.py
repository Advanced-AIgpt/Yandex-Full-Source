import os
import sys
import logging
import asyncio
from alice.cuttlefish.library.python.apphost_here.utils import run_daemon, read_json, read_text, Daemon, Process


# -------------------------------------------------------------------------------------------------
class CuttlefishDaemon(Daemon):
    NAME = "cuttlefish"
    BIN_PATH = "alice/cuttlefish/bin/cuttlefish/cuttlefish"

    def __init__(self, bin_path, port):
        super().__init__()
        self._bin_path = bin_path
        self._port = port

    async def run(self, workdir="./", **kwargs):
        args = [
            self._bin_path,
            "run",
            "--override",
            f"server.http.port={self._port}",
            "--override",
            f"server.grpc.port={self._port + 1}",
        ]
        await super().run(args=args, workdir=workdir, **kwargs)

    async def _wait_ready(self):
        await asyncio.sleep(1)  # check if stays alive after 1 second

    @property
    def grpc_port(self):
        return None if (self._port is None) else (self._port + 1)

    @property
    def endpoint(self):
        return ("localhost", self.grpc_port)


# -------------------------------------------------------------------------------------------------
class UniproxyDaemon(Daemon):
    NAME = "uniproxy"
    BIN_PATH = "alice/uniproxy/bin/uniproxy/uniproxy"

    def __init__(self, bin_path, port):
        super().__init__()
        self._bin_path = bin_path
        self._port = port

    async def run(self, workdir="./", **kwargs):
        # env = kwargs.pop("env", None)
        # if env is None:
        #     os.environ
        #     env = {}

        # env["UNIPROXY_CUSTOM_ENVIRONMENT_TYPE"] = "local"

        # ydb_token = os.environ.get("YDB_TOKEN")
        # if ydb_token is None:
        #     ydb_token = read_text(os.path.expanduser("~/.ydb/token"))
        # env["YDB_TOKEN"] = ydb_token

        self.logger.warning(
            "This version doesn't need `tvmtool` but will use you SSH key to obtain TVM tickets. "
            "Hence `TVM ssh user` role and running `ssh-agent` are mandatory"
        )
        args = [self._bin_path, "-p", str(self._port), "-n", str(1)]
        await super().run(args=args, workdir=workdir, **kwargs)

    async def _wait_ready(self):
        await asyncio.sleep(1)

    @property
    def endpoint(self):
        return ("localhost", self._port)


# -------------------------------------------------------------------------------------------------
class SubwayDaemon(Daemon):
    NAME = "subway"
    BIN_PATH = "alice/uniproxy/bin/uniproxy-subway/uniproxy-subway"

    def __init__(self, bin_path):
        super().__init__()
        self._bin_path = bin_path
        self._port = 7777

    async def run(self, **kwargs):
        await super().run(args=[self._bin_path, "-p", str(self._port)], **kwargs)

    async def _wait_ready(self):
        await asyncio.sleep(1)

    @property
    def endpoint(self):
        return ("localhost", self._port)


# -------------------------------------------------------------------------------------------------
class TvmtoolDaemon(Daemon):
    NAME = "tvmtool"

    # should be downloaded
    BIN_PATH = "alice/cuttlefish/tests/local/tvmtool"
    CONFIG_PATH = "alice/cuttlefish/tests/local/tvm.config"

    def __init__(self, bin_path, config_path):
        super().__init__()
        self._bin_path = bin_path
        self._config_path = config_path
        self._port = 9090
        self._token = None

    @property
    def token(self):
        return self._token

    async def run(self, **kwargs):
        async with Process("xxd -l 16 -p /dev/urandom", stdout=Process.PIPE, shell=True) as proc:
            async for l in proc.read_stdout():
                self._token = l.strip()

        self.logger.debug(f"Token: {self._token}")
        env = kwargs.pop("env", None)
        if env is None:
            env = {}
        env["QLOUD_TVM_TOKEN"] = self._token

        await super().run(
            args=[self._bin_path, "-v", "--port", str(self._port), "-c", self._config_path], env=env, **kwargs
        )

    async def _wait_ready(self):
        await asyncio.sleep(1)

    @property
    def endpoint(self):
        return ("localhost", self._port)
