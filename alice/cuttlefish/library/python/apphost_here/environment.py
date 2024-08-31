import os
import asyncio
import logging
from contextlib import asynccontextmanager
from .apphost_daemon import AppHostDaemon, AppHostEventLogDump
from .utils import write_json, ensure_dir


DEFAULT_APPHOST_CONFIG = {
    "threads": 2,
    "grpc_threads": 1,
    "executor_threads": 2,
    "total_quota": 400,
    "group_quotas": {"": 1},
    "source_codecs": ["lz4"],
    "conf_dir": "patched_graphs/",
    "fallback_conf_dir": "horizon-data/graphs/",
    "backends_path": "backends-after-its.json",
    "fallback_backends_path": "horizon-data/backends",
    "stable_branch_mapping_path": "stable_branch",
    "protocol_options": {"post/ConnectTimeout": "25ms"},
    "update_from_fs_config": {},
    "debug_info_version": 1,
    "golovan_output_zero_values": False,
    "session_dump_count": 10,
    "session_dump_period": "1h",
    "installation": "VOICE",
    "background_threads": 2,
    "grpc_max_channels_to_backend": 4,
    "enable_profile_log": True,
    "golovan_cache_update_duration": "2500",
}


class AppHostEnvironment:
    class MOCK_NO:
        pass

    class MOCK_TRANSPARENT:
        pass

    class MOCK_LOCALHOST:
        pass

    DEFAULT_CONFIGURATION = "ctype=testing;geo=sas"
    DEFAULT_AGENT_BIN_PATH = "apphost/daemons/horizon/agent/horizon-agent"
    DEFAULT_APPHOST_BIN_PATH = "apphost/daemons/app_host/app_host"
    DEFAULT_APPHOST_EVLOGDUMP_BIN_PATH = "apphost/tools/event_log_dump/event_log_dump"

    def __init__(self, local_arcadia_path, apphost_bin_path=None, apphost_evlogdump_bin_path=None, apphost_config=None):
        self.logger = logging.getLogger("env")

        if apphost_bin_path is None:
            apphost_bin_path = os.path.join(local_arcadia_path, self.DEFAULT_APPHOST_BIN_PATH)
        if apphost_evlogdump_bin_path is None:
            apphost_evlogdump_bin_path = os.path.join(local_arcadia_path, self.DEFAULT_APPHOST_EVLOGDUMP_BIN_PATH)
        if apphost_config is None:
            apphost_config = DEFAULT_APPHOST_CONFIG

        self._apphost_config = apphost_config
        self._apphostd = AppHostDaemon(apphost_bin_path)
        self._apphost_evlogdump_bin_path = apphost_evlogdump_bin_path

        self._tasks = []

        self._ready_fut = None
        self._apphost_evlog_path = None  # will be initialized in `run`

    @property
    def apphostd(self):
        return self._apphostd

    @property
    def evlog_dump(self):
        if self._apphost_evlog_path is None:
            raise RuntimeError("not running")
        return AppHostEventLogDump(
            evlogdump_bin_path=self._apphost_evlogdump_bin_path, evlog_path=self._apphost_evlog_path
        )

    async def _wait_all(self):
        failure = False

        async def do_wait(how):
            self.logger.debug(f"wait for {len(self._tasks)} tasks...")
            done, self._tasks = await asyncio.wait(self._tasks, return_when=how)
            for task in done:
                try:
                    await task
                except asyncio.CancelledError:
                    pass
                except:
                    nonlocal failure
                    failure = True
                    self.logger.exception(f"{task} failed")

        if not self._tasks:
            return

        await do_wait(asyncio.FIRST_EXCEPTION)
        self.stop()  # at least one task has failed - stop the whole environment
        while self._tasks:
            await do_wait(asyncio.FIRST_EXCEPTION)

        self.logger.debug("all tasks are completed")
        if failure:
            raise RuntimeError("failed")

    async def run(self, apphost_port, horizon_data, workdir="./", tvm_id=None, env=None):
        self._ready_fut = asyncio.Future()

        workdir = os.path.abspath(workdir)
        ensure_dir(workdir)

        apphost_config_path = os.path.join(workdir, "app_host.json")
        failure = False

        try:
            self.logger.info(f"Use existing horizon-data (from {horizon_data.path})")
            horizon_data.save_to(os.path.join(workdir, "horizon-data"))

            # make config
            cfg = self._apphostd.patch_config_for_local_run(self._apphost_config, workdir=workdir, tvm_id=tvm_id)
            write_json(cfg, apphost_config_path)

            # run AppHost...
            self._tasks.append(
                self._apphostd.create_task(config_path=apphost_config_path, port=apphost_port, workdir=workdir, env=env)
            )
            await self._apphostd.wait_ready(timeout=30)

            self._apphost_evlog_path = "{}-{}".format(cfg["log"], apphost_port)

            self.logger.info("READY")
            self._ready_fut.set_result(True)
        except Exception as err:
            failure = True
            self.logger.exception("failed")
            self._ready_fut.set_exception(err)
            self.stop()

        await self._wait_all()
        if failure:
            raise RuntimeError("failed")

    def stop(self):
        self.logger.debug("stop all tasks")
        for t in self._tasks:
            t.cancel()

    async def wait_ready(self):
        while self._ready_fut is None:
            await asyncio.sleep(0.1)
        return await self._ready_fut

    @asynccontextmanager
    async def running(self, *args, **kwargs):
        task = asyncio.create_task(self.run(*args, **kwargs))
        await self.wait_ready()
        try:
            yield self
        except:
            self.logger.warning("terminate due to exception in enveloped scope")
            raise
        finally:
            self.stop()
            await task
