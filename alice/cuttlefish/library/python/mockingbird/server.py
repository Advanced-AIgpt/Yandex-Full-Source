import random
import tornado.web
import tornado.ioloop
import logging
import asyncio
import inspect
from collections import deque
import time
from .callback_handler import create_callback_http_handler
from alice.cuttlefish.library.python.apphost_grpc_servant import AppHostServant, AppHostHandler


def listen(app, port=None):
    MAX_PORT = 0xFFFF

    if port is None:
        port = random.randint(30000, 33000)

    while port < MAX_PORT:
        try:
            return (app.listen(port), port)
        except OSError:
            port += 1

    raise RuntimeError("could not find free port")


async def await_if_needed(func, *args, **kwargs):
    if inspect.iscoroutinefunction(func):
        return await func(*args, **kwargs)
    return func(*args, **kwargs)


# -------------------------------------------------------------------------------------------------
class HttpBackend:
    @staticmethod
    def default_callback(name, request):
        return {"code": 404, "reason": "NotFound"}

    def __init__(self, name, callback, port=None):
        if callback is None:
            callback = self.default_callback

        self.name = name
        self.port = port
        self.callback = callback
        self._history = deque()
        self.handler = create_callback_http_handler(self.name, self)

    async def __call__(self, request):
        start_time = time.monotonic()
        response = await await_if_needed(self.callback, name=self.name, request=request)
        self._history.append({"time": start_time, "request": request, "response": response})
        return response


# -------------------------------------------------------------------------------------------------
class AppHostGrpcBackend:
    @staticmethod
    async def default_callback(name, request):
        request.flush()

    def __init__(self, name, paths, callback, port):
        if callback is None:
            callback = self.default_callback

        self.name = name
        self.port = port
        self.callback = callback
        self._history = deque()
        self.handlers = {
            p: AppHostHandler(path=p, logger=logging.getLogger(f"mockingbird.ah-grpc.{name}.{p}")) for p in paths
        }

    async def __call__(self, request):
        start_time = time.monotonic()
        await await_if_needed(self.callback, name=self.name, request=request)
        self._history.append({"time": start_time, "request": request})


# -------------------------------------------------------------------------------------------------
class HttpServant:
    def __init__(self, backend):
        self.backend = backend
        self.logger = logging.getLogger(f"mockingbird.http.{backend.name}")
        self._srv = None

    def __str__(self):
        return f"HttpServant({self.name}, endpoint={self.endpoint})"

    @property
    def name(self):
        return self.backend.name

    @property
    def endpoint(self):
        return ("localhost", self.backend.port)

    async def start(self, port=None):
        if port is None:
            port = self.backend.port

        app = tornado.web.Application([(r"(.*)", self.backend.handler, dict(logger=self.logger))])

        self._srv, self.backend.port = listen(app, port)
        self.logger.info(f"is listening on {self.endpoint}")

    async def stop(self):
        if self._srv is None:
            raise RuntimeError("not started")

        self.logger.debug("stopping...")
        self._srv.stop()
        await self._srv.close_all_connections()
        self._srv = None
        self.logger.info("stopped")


# -------------------------------------------------------------------------------------------------
class AppHostGrpcServant(AppHostServant):
    def __init__(self, backend):
        self.backend = backend
        self.logger = logging.getLogger(f"mockingbird.ah-grpc.{backend.name}")
        self._task = None
        super().__init__(handlers=self.backend.handlers, port=self.backend.port, logger=self.logger)

    def __str__(self):
        return f"AppHostGrpcServant({self.name}, endpoint={self.endpoint})"

    @property
    def name(self):
        return self.backend.name

    @property
    def endpoint(self):
        return ("localhost", self.grpc_port)

    async def process(self):
        req_tasks = {}

        async def do_process(request):
            self.logger.debug(f"Start processing {request}")
            try:
                async with request:
                    await self.backend(request)
                self.logger.debug(f"Processing of {request} sucessfully completed")
            except:
                self.logger.exception(f"Processing of {request} failed with an exception")
            finally:
                req_tasks.pop(request.id)

        try:
            while True:
                req = await self.get_request()
                req_tasks[req.id] = asyncio.create_task(do_process(req))
        except asyncio.CancelledError:
            self.logger.debug("Processing has been cancelled")

        for task in [t for t in req_tasks.values()]:
            task.cancel()
            try:
                await task
            except asyncio.CancelledError:
                pass
            except:
                self.logger.exception("Request processing failed with an exception")

    async def start(self):
        await super().start()
        self._task = asyncio.create_task(self.process())

    async def stop(self):
        self.logger.debug("Cancel the task...")
        self._task.cancel()
        try:
            await self._task
        except asyncio.CancelledError:
            pass
        finally:
            self.logger.info("Processing task has finished")

        await super().stop()


# -------------------------------------------------------------------------------------------------
class MockServer:
    async def __aenter__(self):
        await self.start()
        return self

    async def __aexit__(self, *args, **kwargs):
        await self.stop()

    def __init__(self, backends):
        self.logger = logging.getLogger("mockingbird.httpserver")

        self._servants = {}
        for b in backends:
            if isinstance(b, HttpBackend):
                servant = HttpServant(b)
            elif isinstance(b, AppHostGrpcBackend):
                servant = AppHostGrpcServant(b)
            else:
                raise RuntimeError("unknown type of backend")

            self._servants[b.name] = servant

    def endpoint_for(self, name):
        return self._servants[name].endpoint

    @property
    def servants(self):
        return self._servants.values()

    async def start(self):
        for s in self._servants.values():
            await s.start()

    async def stop(self):
        for s in self._servants.values():
            await s.stop()
