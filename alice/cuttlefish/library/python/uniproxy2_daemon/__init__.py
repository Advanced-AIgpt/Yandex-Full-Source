import os
import json
import asyncio
import shutil
import yaml
from contextlib import asynccontextmanager
import tornado.httpclient
from alice.cuttlefish.library.python.test_utils import deepupdate, unistat_diff, match
from alice.cuttlefish.library.python.test_utils_with_tornado import wait_ping
from alice.cuttlefish.library.python.apphost_here.utils import Daemon, Process, read_json, write_json, ensure_dir


def _make_settings_config(settings, fname=None):
    res = yaml.safe_dump(settings)
    if fname is not None:
        with open(fname, "w") as f:
            f.write(res)
    return res


# -------------------------------------------------------------------------------------------------
class Uniproxy2BuiltinEventLogReader:
    def __init__(self, bin_path, evlog_path):
        self._bin_path = bin_path
        self._evlog_path = evlog_path

    async def get_json(self, start_time=None, end_time=None):
        args = [self._bin_path, "evlogdump", "-r", "-j"]
        if start_time is not None:
            args += ["-s", str(start_time)]
        if end_time is not None:
            args += ["-e", str(end_time)]
        args.append(self._evlog_path)

        async with Process(args, stdout=Process.PIPE) as p:
            async for record in p.read_stdout():
                if not record:
                    continue
                yield json.loads(record)

    async def get_json_all(self, *args, **kwargs):
        return [r async for r in self.get_json(*args, **kwargs)]

    async def dump_text(self, fname, start_time=None, end_time=None):
        args = [self._bin_path, "evlogdump", "-r"]
        if start_time is not None:
            args += ["-s", str(start_time)]
        if end_time is not None:
            args += ["-e", str(end_time)]
        args.append(self._evlog_path)

        async with Process(args, stdout=Process.FILE(fname)) as p:
            await p.wait(timeout=20)


# -------------------------------------------------------------------------------------------------
class Uniproxy2Daemon(Daemon):
    NAME = "uniproxy2"
    BIN_PATH = "voicetech/uniproxy2/uniproxy2"
    CONFIG_PATH = "alice/uniproxy/configs/prod/configs/uniproxy2.json"

    def __init__(self, bin_path, config_path, port, patcher_config_path=None):
        super().__init__()
        self._bin_path = bin_path
        self._cfg_path = config_path
        self._port = port

        self._workdir = None
        self._patcher_config_path = patcher_config_path

        self.logger.debug(f"bin_path='{self._bin_path}' " f"cfg_path='{self._cfg_path}' " f"port={self._port}")

    @property
    def stdout(self):
        return os.path.join(self._workdir, "uniproxy2.out")

    @property
    def stderr(self):
        return os.path.join(self._workdir, "uniproxy2.err")

    @property
    def eventlog_path(self):
        return os.path.join(self._workdir, "uniproxy2.evlog")

    @property
    def evlog_dump(self):
        return Uniproxy2BuiltinEventLogReader(self._bin_path, self.eventlog_path)

    @property
    def rtlog_path(self):
        return os.path.join(self._workdir, "uniproxy2.rtlog")

    @property
    def config_path(self):
        return os.path.join(self._workdir, "uniproxy2.json")

    @property
    def patcher_config_path(self):
        return self._patcher_config_path

    @property
    def host(self):
        return "localhost"

    @property
    def port(self):
        return self._port

    @property
    def endpoint(self):
        return (self.host, self.port)

    @property
    def http_url(self):
        return f"http://{self.host}:{self.port}"

    @property
    def ws_url(self):
        host, port = self.endpoint
        return f"ws://{self.host}:{self.port}/uni.ws"

    def ws_connect(self, cgi=None, headers=None):
        url = self.ws_url
        if cgi is not None:
            url += "?" + cgi
        req = tornado.httpclient.HTTPRequest(url, headers=headers)
        return tornado.websocket.websocket_connect(req)

    def _patch_config(self, cfg, patch=None):
        default_patch = {
            "Server": {"Http": {"Port": self._port}},
            "Logger": {
                "EventLog": {"Filename": self.eventlog_path},
                "RtLog": {"file": self.rtlog_path},
            },
            "Uniproxy2": {"LockMemory": False, "SettingsPatcherDynamicCfg": self.patcher_config_path},
        }

        if patch is None:
            patch = default_patch
        else:
            patch = deepupdate(default_patch, patch)

        patch = deepupdate(
            default_patch,
            {
                "MatrixAmbassador": {
                    "CommunicationGap": "20s",
                },
            },
        )

        self.logger.debug(f"Config will be patched with: {patch}")
        return deepupdate(cfg, patch)

    async def run(self, workdir, config_patch=None):
        self._workdir = workdir
        if self._patcher_config_path is None:
            self._patcher_config_path = os.path.join(self._workdir, "controls/patcher_dynamic_cfg.yaml")

        ensure_dir(os.path.dirname(self._patcher_config_path))

        cfg = read_json(self._cfg_path)
        self._patch_config(cfg=cfg, patch=config_patch)
        write_json(cfg, self.config_path)
        self.logger.debug(f"Config was written to '{self.config_path}'")

        return await super().run(
            args=[self._bin_path, "--config", self.config_path],
            workdir=self._workdir,
            stdout=self.stdout,
            stderr=self.stderr,
        )

    async def _wait_ready(self):
        await wait_ping(f"{self.http_url}/ping")

    async def _get(self, url):
        resp = await tornado.httpclient.AsyncHTTPClient().fetch(f"{self.http_url}{url}")
        body = resp.body.decode("utf-8")
        self.logger.debug(f"GET {url} responded: {body}")
        return body

    async def _get_json(self, url):
        return json.loads(await self._get(url))

    async def get_unistat(self):
        signals = await self._get_json("/unistat")
        res = {}
        for s in signals:
            res[s[0]] = s[1]
        return res

    @asynccontextmanager
    async def check_unistat_delta(self, expected):
        before = await self.get_unistat()
        try:
            yield
        finally:
            diff = unistat_diff(before, await self.get_unistat())
            assert match(diff, expected)

    async def update_settings_patch(self, settings):
        # this logic is similar to production (ITS)
        tmp_file = f"{self.patcher_config_path}-tmp"
        yaml = _make_settings_config(settings, fname=tmp_file)  # serialize dict or list into YAML
        shutil.move(tmp_file, self.patcher_config_path)
        await asyncio.sleep(0.1)
        return yaml

    @asynccontextmanager
    async def settings_patch(self, name=None, surface=None, **kwargs):
        """Context manager that applies and rollback settings patch (ITS-like way)"""

        prev_config = None
        if os.path.exists(self.patcher_config_path):
            with open(self.patcher_config_path, "r") as f:
                prev_config = f.read()

        async with self.check_unistat_delta({"settings_patcher_update_ok_summ": 1}):
            cfg = {
                "name": name if (name is not None) else "some patch",
                "surface": surface if (surface is not None) else "all",
                "settings": [{"name": k, "value": v} for k, v in kwargs.items()],
            }
            await self.update_settings_patch([cfg])

        try:
            yield self
        finally:
            if prev_config is None:
                os.remove(self.patcher_config_path)
            else:
                with open(self.patcher_config_path, "w") as f:
                    f.write(prev_config)
            await asyncio.sleep(0.1)
