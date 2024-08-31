from apphost.api import service
from functools import partial
import logging
import datetime
import asyncio
from collections import deque


def ensure_bytes(val):
    return val.encode("utf-8") if isinstance(val, str) else val


# -------------------------------------------------------------------------------------------------
class AppHostRequest:
    async def __aenter__(self):
        return self

    async def __aexit__(self, *args, **kargs):
        self.flush()

    def __init__(self, handler, ioloop, context):
        self._ioloop = ioloop
        self._handler = handler
        self._id = str(context.request_id)
        self.context = context

        self._handler._req_in_prog.add(self)

    def __repr__(self):
        return f"AppHostRequest(path='{self.path}', ID={self.id})"

    @property
    def handler(self):
        return self._handler

    @property
    def path(self):
        return self._handler.path

    @property
    def id(self):
        return self._id

    def flush(self):
        if self.context is not None:
            self.context.flush()
            del self.context
            self.context = None

            self._handler._req_in_prog.discard(self)

    def intermediate_flush(self):
        self.context.intermediate_flush()

    def get_item(self, item_type, proto_cls):
        item_type = ensure_bytes(item_type)
        raw = self.context.get_protobuf_item(item_type, mask=b"i")
        dst = proto_cls()
        dst.ParseFromString(raw)
        return dst

    def get_items(self, item_type, proto_cls):
        for raw in self.context.get_protobuf_items(item_type, mask=b"i"):
            dst = proto_cls()
            dst.ParseFromString(raw)
            yield dst

    def get_json_item(self, item_type):
        return self.context.get_item(item_type, mask=b"i")

    def add_item(self, item_type, item):
        item_type = ensure_bytes(item_type)
        self.context.add_protobuf_item(item_type, item)

    async def next_input(self, timeout=None):
        fut = self._ioloop.create_future()

        def on_next_input(has_new_input):
            def in_loop():
                if not fut.cancelled():
                    fut.set_result(has_new_input)

            self._ioloop.call_soon_threadsafe(in_loop)

        self.context.next_input(on_next_input)

        got_new_input = bool(await asyncio.wait_for(fut, timeout))
        return got_new_input


# -------------------------------------------------------------------------------------------------
class AppHostHandler:
    def __init__(self, path, logger=None):
        self.logger = logging.getLogger(f"ah.handler.{path}") if (logger is None) else logger
        self.path = path
        self._req_in_prog = set()
        self._req_queue = asyncio.Queue()

    def flush_all(self):
        while self._req_in_prog:
            req = self._req_in_prog.pop()
            self.logger.warning(f"Auto-flushed {req}")
            req.flush()

    async def get_new_request(self, timeout=None):
        if (timeout is not None) and (timeout < 0):
            raise asyncio.TimeoutError()
        req = await asyncio.wait_for(self._req_queue.get(), timeout=timeout)
        self._req_queue.task_done()
        return req


# -------------------------------------------------------------------------------------------------
class AppHostServant:
    STREAMING_SESSION_TIMEOUT = datetime.timedelta(minutes=10)

    async def __aenter__(self):
        await self.start()
        return self

    async def __aexit__(self, *args, **kwargs):
        self.stop()

    def __init__(self, handlers, port, logger=None, max_queue_size=50):
        if isinstance(handlers, dict):
            handlers = [h for h in handlers.values()]

        self.logger = logging.getLogger("ah.servant") if (logger is None) else logger
        self._handlers = {h.path: h for h in handlers}
        self._ah_loop = service.Loop()
        self._req_queue = asyncio.Queue()
        self._port = port
        self._req_queue2 = deque()

        self._max_queue_size = max_queue_size

    @property
    def http_port(self):
        return self._port

    @property
    def grpc_port(self):
        return self._port + 1

    async def get_request(self, timeout=None, path_filter=None):
        if path_filter is None:
            path_filter = lambda _: True

        coros = [h.get_new_request() for path, h in self._handlers.items() if path_filter(path)]
        done, pending = await asyncio.wait(coros, timeout=timeout, return_when=asyncio.FIRST_COMPLETED)
        for t in pending:
            t.cancel()
        if not done:
            raise asyncio.TimeoutError()
        return await done.pop()

    async def get_request_of(self, path, timeout=None):
        handler = self._handlers.get(path)
        if handler is None:
            raise RuntimeError(f"No handler for path '{path}'")
        return await handler.get_new_request(timeout=timeout)

    async def start(self):
        ioloop = asyncio.get_running_loop()
        timeout_microseconds = self.STREAMING_SESSION_TIMEOUT.total_seconds() * 1000000

        def on_request(context, handler):
            req = AppHostRequest(handler, ioloop, context)
            ioloop.call_soon_threadsafe(handler._req_queue.put_nowait, req)
            self.logger.debug(f"New request: {req}")

        self._ah_loop.enable_grpc(self.grpc_port, False, timeout_microseconds)

        for handler in self._handlers.values():
            self.logger.info(f"Add handler on '{handler.path}'")
            self._ah_loop.add(self.http_port, partial(on_request, handler=handler), path=handler.path.encode())

        self._ah_loop.set_max_queue_size(self._max_queue_size)
        self._ah_loop.start()
        self.logger.info(
            f"Started: http_port={self.http_port} grpc_port={self.grpc_port} " f"max_queue_size={self._max_queue_size}"
        )

    async def stop(self):
        for handler in self._handlers.values():
            handler.flush_all()
        self._ah_loop.stop()
        self.logger.info("Stopped")
