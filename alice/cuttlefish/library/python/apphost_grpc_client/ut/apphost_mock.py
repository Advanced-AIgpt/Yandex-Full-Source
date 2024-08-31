from apphost.api import service
from functools import partial
import tornado.httpclient
import yatest.common.network
import logging
import time
import asyncio


logger = logging.getLogger("AppHostMock")


def ensure_bytes(val):
    return val.encode("utf-8") if isinstance(val, str) else val


def fix_path(path):
    if path.startswith("/"):
        return path
    else:
        return f"/{path}"


# -------------------------------------------------------------------------------------------------
class AppHostMockRequest:
    async def __aenter__(self):
        return self

    async def __aexit__(self, *args, **kargs):
        self.flush()

    def __init__(self, loop, context, path):
        self.loop = loop
        self.context = context
        self.path = fix_path(path)

    def finish(self):
        self.flush()

    def flush(self):
        if self.context is not None:
            self.context.flush()
            del self.context
            self.context = None

    @property
    def id(self):
        return str(self.context.request_id)

    # def get_raw_protobuf_items(self, item_type):
    #     item_type = ensure_bytes(item_type)
    #     for raw in self.context.get_protobuf_items(item_type, mask=b"i"):
    #         yield raw

    def get_item(self, item_type, proto_cls):
        item_type = ensure_bytes(item_type)
        raw = self.context.get_protobuf_item(item_type, mask=b"i")
        dst = proto_cls()
        dst.ParseFromString(raw)
        return dst

    def get_items(self, item_type, proto_cls):
        item_type = ensure_bytes(item_type)
        for raw in self.context.get_protobuf_items(item_type, mask=b"i"):
            dst = proto_cls()
            dst.ParseFromString(raw)
            yield dst

    def get_json_item(self, item_type):
        item_type = ensure_bytes(item_type)
        return self.context.get_item(item_type, mask=b"i")

    def add_item(self, item_type, item):
        item_type = ensure_bytes(item_type)
        self.context.add_protobuf_item(item_type, item)

    def intermediate_flush(self):
        self.context.intermediate_flush()

    async def next_input(self, timeout=3):
        fut = self.loop.create_future()

        def on_next_input(has_new_input):
            def in_loop():
                if not fut.cancelled():
                    fut.set_result(has_new_input)

            self.loop.call_soon_threadsafe(in_loop)

        self.context.next_input(on_next_input)

        got_new_input = bool(await asyncio.wait_for(fut, timeout))
        logger.info(f"Got next input in stream RID={self.id}, new_input={got_new_input}")

        return got_new_input


# -------------------------------------------------------------------------------------------------
class AppHostMock:
    async def __aenter__(self):
        await self.start()
        return self

    async def __aexit__(self, *args, **kwargs):
        self.stop()

    def __init__(self, *handlers, pm=None):
        if pm is None:
            pm = yatest.common.network.PortManager()

        self._handlers = [fix_path(handler) for handler in handlers]
        self._pm = pm
        self._loop = service.Loop()
        self._req_queue = asyncio.Queue()
        self._req_in_prog = {}
        self.port = None

    @property
    def grpc_port(self):
        return self.port + 1

    @property
    def grpc_endpoint(self):
        return ("localhost", self.grpc_port)

    async def get_request(self, timeout=1, noexcept=False):
        try:
            req = await asyncio.wait_for(self._req_queue.get(), timeout=timeout)
            self._req_queue.task_done()
            return req
        except asyncio.TimeoutError:
            if not noexcept:
                raise

    async def start(self):
        loop = asyncio.get_running_loop()
        self.port = self._pm.get_port_range(None, 2)

        def handler(context, path):
            logger.info(f"New request: RID={context.request_id}, path='{path}'")
            req = AppHostMockRequest(loop, context, path)
            self._req_in_prog[context.request_id] = req
            loop.call_soon_threadsafe(self._req_queue.put_nowait, req)

        def add_path(path):
            self._loop.add(self.port, partial(handler, path=path), path=str.encode(path))

        logger.info(f"Listen on {self.port} and {self.grpc_port} for gRPC")
        self._loop.enable_grpc(self.grpc_port, False, 60 * 1000 * 1000)
        # for path in ["/synchronize_state_2", "/example_base64_streaming", "/context_load", "/store_audio"]:
        for path in self._handlers:
            add_path(path)
        self._loop.start()
        await self.wait_ready()

    def stop(self):
        for reqid, req in self._req_in_prog.items():
            req.flush()
        self._req_in_prog = {}

        self._loop.stop()
        logger.info("Just stopped")

        self._pm.release_port(self.port)

    async def wait_ready(self, timeout=10):
        ping_url = f"http://localhost:{self.port}/admin?action=ping"
        deadline = time.monotonic() + timeout
        while True:
            try:
                resp = await tornado.httpclient.AsyncHTTPClient().fetch(ping_url)
                if resp.code == 200:
                    logger.info("Ping is OK")
                    return
            except Exception:
                if time.monotonic() > deadline:
                    raise
                await asyncio.sleep(0.1)
