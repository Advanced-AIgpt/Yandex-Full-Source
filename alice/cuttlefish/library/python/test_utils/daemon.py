from .process import Process
import os
import logging
import asyncio
import time
from contextlib import asynccontextmanager


class Daemon:
    NAME = "anoynmous"

    def _prepare_output(self, out, workdir, suf):
        if out is None:  # output is redirected to a file by default
            return Process.FILE(os.path.join(workdir, f"{self.NAME}.{suf}"))
        if isinstance(out, str):
            return Process.FILE(out)
        return out

    def __init__(self):
        self.logger = logging.getLogger(f"daemon.{self.NAME}")
        self._proc = None

    def __repr__(self):
        return f"Daemon({self.NAME})"

    def create_task(self, *args, **kwargs):
        return asyncio.create_task(self.run(*args, **kwargs), name=self.NAME)

    async def wait_ready(self, timeout=None):
        if timeout is None:
            timeout = 5

        t0 = time.monotonic()
        deadline = t0 + timeout

        # wait for process started
        self.logger.debug("wait for started...")
        while self._proc is None:
            if time.monotonic() < deadline:
                await asyncio.sleep(0.1)
            else:
                raise RuntimeError(f"daemon {self.NAME} could not start")

        try:
            t = deadline - time.monotonic()
            self.logger.debug(f"wait for ready (up to {t:.3f} sec)...")
            res = await asyncio.wait_for(self._wait_ready(), timeout=t)

            if not self._proc.is_running:
                raise RuntimeError(f"daemon {self.NAME} died too fast")

            self.logger.debug(f"got ready in {time.monotonic() - t0:.3f} sec")
            return res
        except:
            if not self._proc.is_running:
                raise RuntimeError(f"daemon {self.NAME} died too fast")
            raise

    async def run(self, args, workdir="./", shell=False, stdout=None, stderr=None, env=None):
        if self._proc is not None:
            raise RuntimeError(f"daemon {self.NAME} is running already")

        proc = Process(
            args=args,
            workdir=workdir,
            stdout=self._prepare_output(stdout, workdir, "out"),
            stderr=self._prepare_output(stderr, workdir, "err"),
            logger=self.logger,
            shell=shell,
            env=env,
        )

        try:
            async with proc:
                self._proc = proc
                await proc.wait()
        except asyncio.CancelledError:
            pass  # normal termination
        except Exception as err:
            self.logger.error(f"crashed: {err}")
            raise err

    @asynccontextmanager
    async def running(self, *args, ready_timeout=None, **kwargs):
        """Async context manager to start&stop a daemon"""

        task = self.create_task(*args, **kwargs)

        ready_exc = None
        try:
            await self.wait_ready(timeout=ready_timeout)
        except Exception as exc:
            ready_exc = exc

        if ready_exc is not None:
            task.cancel()
            await task  # it's expected to throw a cause exception

            self.logger.debug(f"no exception from task - raise auxiliary exception {ready_exc}")
            raise ready_exc  # otherwise throw original exception

        try:
            yield self
        except Exception as err:
            self.logger.warning(f"terminate due to exception in enveloped scope: {err}")
            raise
        finally:
            try:
                self.logger.debug("cancel task...")
                task.cancel()
                await task
            except Exception as err:
                self.logger.warning(f"finished with exception: {err}")
            self.logger.debug("finished")

    async def _wait_ready(self):
        # to be redefined in sublclasses
        await asyncio.sleep(1)


# deprecated
@asynccontextmanager
async def run_daemon(d, *args, ready_timeout=None, **kwargs):
    async with d.running(*args, ready_timeout=ready_timeout, **kwargs) as d:
        yield d
