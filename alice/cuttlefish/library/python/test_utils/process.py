import logging
import asyncio


async def read_until(reader, separator=b"\n", timeout=None, encoding=None):
    eof = False
    while not eof:
        try:
            chunk = await asyncio.wait_for(reader.readuntil(separator), timeout=timeout)
        except asyncio.IncompleteReadError as err:
            chunk = err.partial
            eof = True
        if not chunk:
            break
        if encoding is not None:
            chunk = chunk.decode(encoding)
        yield chunk


async def lines(reader):
    while True:
        l = await reader.readline()
        if not l:
            return
        yield l


class ToLogger:
    def __init__(self, logger, prefix="", level=logging.INFO, encoding="utf-8"):
        self._logger = logger
        self._prefix = prefix
        self._level = level
        self._encoding = encoding

    async def run(self, reader):
        if callable(self._logger):
            self._logger = self._logger()
        if callable(self._prefix):
            self._prefix = self._prefix()
        async for l in lines(reader):
            if self._encoding is not None:
                l = l.decode(self._encoding, errors="ignore").strip()
            self._logger.log(self._level, f"{self._prefix}{l}")

    def __str__(self):
        name = "unknown" if callable(self._logger) else self._logger.name
        return f"to logger ({name}, level={logging.getLevelName(logging.INFO)})"


class ToFObj:
    def __init__(self, fobj):
        self._fobj = fobj

    async def run(self, reader):
        async for l in lines(reader):
            self._fobj.write(l)

    def __str__(self):
        return f"to file-like object ({self._fobj})"


class ToFile:
    def __init__(self, fname):
        self._fname = fname

    async def run(self, reader):
        with open(self._fname, "wb") as f:
            async for l in lines(reader):
                f.write(l)

    def __str__(self):
        return f"to file ({self._fname})"


class ToDevnull:
    async def run(self, reader):
        while await reader.read(1024):
            pass

    def __str__(self):
        return "to devnull"


# -------------------------------------------------------------------------------------------------
class OutputHandler:
    _names = {None: "no redirect", asyncio.subprocess.DEVNULL: "devnull", asyncio.subprocess.PIPE: "pipe"}

    def __init__(self, mode):
        self.mode = mode
        self._task = None

    def __str__(self):
        name = self._names.get(self.mode)
        return name or str(self.mode)

    def get_create_arg(self):
        if self.mode in (None, asyncio.subprocess.DEVNULL):
            return self.mode
        return asyncio.subprocess.PIPE

    def on_started(self, out):
        if self.mode in (None, asyncio.subprocess.DEVNULL, asyncio.subprocess.PIPE):
            return
        self._task = asyncio.create_task(self.mode.run(out))

    def on_terminate(self, out):
        if self.mode == asyncio.subprocess.PIPE:
            self._task = asyncio.create_task(ToDevnull().run(out))

    def redirect(self, mode, out):
        if mode is None:
            raise RuntimeError(f"unable to cancel redirection to {self}")
        if self.mode in (None, asyncio.subprocess.DEVNULL):
            raise RuntimeError(f"unable to change redirection from {self}")

        if self._task is not None:
            self._task.cancel()
            self._task = None

        self.mode = ToDevnull() if (mode is asyncio.subprocess.DEVNULL) else mode
        if mode is not asyncio.subprocess.PIPE:
            self._task = asyncio.create_task(self.mode.run(out))

    async def wait(self):
        if self._task is not None:
            await self._task


# -------------------------------------------------------------------------------------------------
class Process:
    DEVNULL = asyncio.subprocess.DEVNULL
    PIPE = asyncio.subprocess.PIPE
    LOG = ToLogger
    FILE = ToFile
    FOBJ = ToFObj

    async def __aenter__(self):
        await self._start()
        return self

    async def __aexit__(self, *_):
        await self.stop()

    def __init__(self, args, workdir=None, stdout=LOG, stderr=LOG, logger=None, shell=False, env=None):
        if shell:
            self.name = "shell." + args.split(" ", maxsplit=1)[0].rsplit("/", maxsplit=1)[-1]
            self._args = [args]
            self._creator = asyncio.create_subprocess_shell
        else:
            self.name = args[0].rsplit("/", maxsplit=1)[-1]
            self._args = args
            self._creator = asyncio.create_subprocess_exec

        self.logger = logger or logging.getLogger(f"process.{self.name}")
        self._workdir = workdir
        self._proc = None
        self._wait_task = None

        if stdout is self.LOG:
            stdout = self._default_log_redirection("out")
        if stderr is self.LOG:
            stderr = self._default_log_redirection("err")

        self._stdout = OutputHandler(stdout)
        self._stderr = OutputHandler(stderr)
        self._env = env

    def _default_log_redirection(self, name):
        return ToLogger(logging.getLogger(f"{self.logger.name}.{name}"), prefix=lambda: f"[PID={self.pid}] ")

    def __str__(self):
        if self._proc is None:
            return f"Process ({self.name}, not started)"
        rc = self._proc.returncode
        if rc is None:
            return f"Process ({self.name}, PID={self._proc.pid}, running)"
        return f"Process ({self.name}, PID={self._proc.pid}, rc={rc})"

    async def _start(self):
        self.logger.debug(f"Starting process (args={self._args}, workdir='{self._workdir})'...")
        self._proc = await self._creator(
            *self._args,
            cwd=self._workdir,
            stdin=asyncio.subprocess.DEVNULL,
            stdout=self._stdout.get_create_arg(),
            stderr=self._stderr.get_create_arg(),
            env=self._env,
        )
        self._stdout.on_started(self._proc.stdout)
        self._stderr.on_started(self._proc.stderr)
        self.logger.info(
            f"{self} started ("
            f"args={self._args}, workdir='{self._workdir}', stdout='{self._stdout}', stderr='{self._stderr}'"
            f", env(var names)={[i for i in self._env.keys()] if self._env else None}"
            ")"
        )
        self._wait_task = asyncio.create_task(self._wait())

    async def _wait(self):
        retcode = self._proc.returncode
        if retcode is None:
            retcode = await self._proc.wait()
            self.logger.info(f"{self} finished with code={retcode}")
        return retcode

    async def _stop(self, timeout=10):
        self._proc.terminate()
        self._stdout.on_terminate(self._proc.stdout)
        self._stderr.on_terminate(self._proc.stderr)
        self.logger.info(f"{self} terminating...")

        retcode = await self.wait(noexcept=True, timeout=timeout)
        if retcode is not None:
            return retcode

        self.logger.warning(f"{self} could not terminate, try to kill it...")

        self._proc.kill()
        self.logger.warning(f"{self} killing...")
        retcode = await self.wait(noexcept=True, timeout=timeout)
        if retcode is not None:
            return retcode

        self.logger.error(f"{self} could not kill")
        return None

    @property
    def pid(self):
        return None if (self._proc is None) else self._proc.pid

    @property
    def returncode(self):
        return None if (self._proc is None) else self._proc.returncode

    @property
    def is_running(self):
        return (self._proc is not None) and (self._proc.returncode is None)

    async def start(self):
        await self._start()

    async def stop(self):
        if self.returncode is not None:
            return self.returncode
        try:
            return await self._stop()
        except ProcessLookupError:
            pass

    async def wait(self, noexcept=False, timeout=None):
        self.logger.debug(f"{self} is waited (timeout={timeout})...")
        try:
            retcode = await asyncio.wait_for(asyncio.shield(self._wait_task), timeout=timeout)
        except asyncio.TimeoutError:
            self.logger.warning(f"{self} hasn't finished in {timeout} seconds")
            if noexcept:
                return None
            raise

        if noexcept or retcode == 0:
            return retcode

        raise RuntimeError(f"{self} failed with code={retcode}")

    async def read_stdout(self, separator=b"\n", timeout=None, encoding="utf-8"):
        if self._stdout.mode is not self.PIPE:
            raise RuntimeError(f"can't read from output {self._stdout}")
        async for line in read_until(self._proc.stdout, separator=separator, timeout=timeout, encoding=encoding):
            yield line

    async def read_stderr(self, separator=b"\n", timeout=None, encoding="utf-8"):
        if self._stderr.mode is not self.PIPE:
            raise RuntimeError(f"can't read from output {self._stdout}")
        async for line in read_until(self._proc.stderr, separator=separator, timeout=timeout, encoding=encoding):
            yield line

    def redirect_stdout(self, dst=LOG):
        if dst is self.LOG:
            dst = self._default_log_redirection("out")
        self._stdout.redirect(dst, self._proc.stdout)

    def redirect_stderr(self, dst=LOG):
        if dst is self.LOG:
            dst = self._default_log_redirection("err")
        self._stderr.redirect(dst, self._proc.stderr)
