import os
import json
from .utils import deepupdate, intrusive_fetch, Daemon, Process


class AppHostEventLogDump:
    def __init__(self, evlogdump_bin_path, evlog_path):
        self.evlogdump_bin_path = evlogdump_bin_path
        self.evlog_path = evlog_path

    async def get_json(self, start_time=None):
        args = [self.evlogdump_bin_path, "-r", "--dump-mode", "Json"]
        if start_time is not None:
            args += ["-s", str(start_time)]
        args.append(self.evlog_path)

        async with Process(args, stdout=Process.PIPE) as p:
            async for record in p.read_stdout():
                if not record:
                    continue
                yield json.loads(record)

    async def dump_text(self, fname, start_time=None, end_time=None):
        args = [self.evlogdump_bin_path, "-r"]
        if start_time is not None:
            args += ["-s", str(start_time)]
        if end_time is not None:
            args += ["-e", str(end_time)]
        args.append(self.evlog_path)

        async with Process(args, stdout=Process.FILE(fname)) as p:
            await p.wait(timeout=20)

    async def get_json_all(self, **kwargs):
        return [r async for r in self.get_json(**kwargs)]


class AppHostDaemon(Daemon):
    NAME = "apphost"

    @staticmethod
    def patch_config_for_local_run(cfg, workdir="./", tvm_id=None):
        deepupdate(
            cfg,
            {
                # don't need any particular branch
                "stable_branch_mapping_path": None,
                "require_stable_branch": False,
                # don't need any particular graphs
                "stable_graph_list_path": None,
                "require_stable_graphs": None,
                # logs
                "log": os.path.join(workdir, "apphost-eventlog"),
                "dump_input_probability": 1,
                "enable_logging_edge_expressions": True,
                # graphs & backends
                "conf_dir": os.path.join(workdir, "graphs_patched"),
                "fallback_conf_dir": os.path.join(workdir, "horizon-data/graphs"),
                "backends_path": os.path.join(workdir, "backends_patched"),
                "fallback_backends_path": os.path.join(workdir, "horizon-data/backends"),
                # TVM
                "tvm_config": None if (tvm_id is None) else {"self_id": tvm_id},
                # Magic option from prod
                # The behavior of the conditions on the edges changes significantly
                "enable_edge_synchronization": True,
                "root_certificates_path": None,
            },
        )
        return cfg

    def __init__(self, apphost_bin_path):
        super().__init__()
        self.apphost_bin_path = apphost_bin_path
        self._port = None

    async def run(self, config_path, port, **kwargs):
        args = [self.apphost_bin_path, "--config", config_path, "-p", str(port), "--no-mlock"]

        workdir = kwargs.get("workdir", "./")
        try:
            os.makedirs(os.path.join(workdir, ".update_from_fs_tmp_dir"))
        except FileExistsError:
            # directory already exists
            pass
        with open(config_path) as f:
            cfg = json.load(f)
        try:
            os.makedirs(os.path.join(workdir, cfg['backends_path']))
        except FileExistsError:
            # directory already exists
            pass
        try:
            os.makedirs(os.path.join(workdir, cfg['conf_dir']))
        except FileExistsError:
            # directory already exists
            pass
        self._port = port
        await super().run(args=args, **kwargs)

    @property
    def http_url(self):
        if self._proc is None:
            return None
        return f"http://localhost:{self._port}"

    @property
    def grpc_endpoint(self):
        if self._proc is None:
            return None
        return ("localhost", self._port + 1)

    @property
    def ping_url(self):
        if self._proc is None:
            return None
        return f"{self.http_url}/admin?action=ping"

    async def _wait_ready(self, timeout=15):
        await intrusive_fetch(self.ping_url, timeout=timeout, logger=self.logger)
