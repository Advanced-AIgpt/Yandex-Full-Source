import os
import json
import asyncio
import yatest.common
from alice.cuttlefish.library.python.test_utils import Process, Daemon


class EventLogDump:
    DEFAULT_BIN_PATH = "voicetech/tools/evlogdump/evlogdump"

    def __init__(self, evlog_path, bin_path=None):
        if bin_path is None:
            bin_path = yatest.common.binary_path(self.DEFAULT_BIN_PATH)

        self.bin_path = bin_path
        self.evlog_path = evlog_path

    async def get_json(self, start_time=None):
        args = [self.bin_path, "-r", "-j"]
        if start_time is not None:
            args += ["-s", str(start_time)]
        args.append(self.evlog_path)

        async with Process(args, stdout=Process.PIPE) as p:
            async for record in p.read_stdout():
                if not record:
                    continue
                yield json.loads(record)

    async def get_json_all(self, *args, **kwargs):
        return [r async for r in self.get_json(*args, **kwargs)]

    async def dump_text(self, fname, start_time=None, end_time=None):
        args = [self.bin_path, "-r"]
        if start_time is not None:
            args += ["-s", str(start_time)]
        if end_time is not None:
            args += ["-e", str(end_time)]
        args.append(self.evlog_path)

        async with Process(args, stdout=Process.FILE(fname)) as p:
            await p.wait(timeout=20)


class CuttlefishDaemon(Daemon):
    NAME = "cuttlefish"
    BIN_PATH = "alice/cuttlefish/bin/cuttlefish/cuttlefish"
    BACKEND_NAMES = ["VOICE__CUTTLEFISH", "VOICE__CUTTLEFISH_BIDIRECTIONAL", "VOICE__CUTTLEFISH_MM"]

    def __init__(self, pm):
        super().__init__()
        self._pm = pm
        self._port = None
        self._evlog_dump = None

    async def run(self, workdir="./", megamind_url=None, **kwargs):
        self._port = self._pm.get_port_range(None, 2)
        self._evlog_dump = EventLogDump(evlog_path=os.path.join(workdir, "cuttlefish.evlog"))

        args = [
            yatest.common.binary_path(self.BIN_PATH),
            "-V",
            f"server.http.port={self._port}",
            "-V",
            f"server.grpc.port={self._port + 1}",
            "-V",
            "server.lock_memory=false",
        ]

        if megamind_url is not None:
            args += ["-V", f"megamind.default_url={megamind_url}"]

        await super().run(args=args, workdir=workdir, **kwargs)

    async def _wait_ready(self):
        await asyncio.sleep(1)  # check if stays alive after 1 second

    @property
    def grpc_port(self):
        return None if (self._port is None) else (self._port + 1)

    @property
    def endpoint(self):
        return ("localhost", self.grpc_port)

    @property
    def evlog_dump(self):
        return self._evlog_dump
