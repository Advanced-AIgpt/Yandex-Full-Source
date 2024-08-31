import yatest.common.network
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_counter import GlobalCounter

from tornado.tcpserver import TCPServer, IOStream
from tornado import httputil
from tornado.queues import Queue


__initialized = False

if not __initialized:
    Logger.init('uniproxy', True)
    GlobalCounter.init()
    __initialized = True


async def read_request_line(stream: IOStream):
    data = await stream.read_until(b"\r\n")
    return httputil.parse_request_start_line(data.decode("utf-8").strip())


async def read_request_headers(stream: IOStream):
    data = await stream.read_until(b"\r\n\r\n")
    return httputil.HTTPHeaders.parse(data.decode("utf-8").strip())


async def read_http_request(stream: IOStream, with_body=False) -> httputil.HTTPServerRequest:
    start_line = await read_request_line(stream)
    headers = await read_request_headers(stream)
    body = None
    if with_body:
        body = await stream.read_bytes(int(headers["Content-Length"]))
    return httputil.HTTPServerRequest(start_line=start_line, headers=headers, body=body)


def run_async(func):
    import tornado.ioloop

    def test_async_wrap():
        tornado.ioloop.IOLoop.current().run_sync(func)
    return test_async_wrap


class QueuedTcpServer(TCPServer):
    class __Stop:
        pass

    def __enter__(self):
        self.listen()
        return self

    def __exit__(self, *args, **kwargs):
        self.stop()

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.__incoming = Queue()
        self.__port_manager = None
        self.__runing = False
        self.port = None

    def stop(self):
        self.__runing = False

        super().stop()
        self.__incoming.put_nowait(self.__Stop)
        self.__port_manager.release_port(self.port)

    def listen(self, address="", port_manager=None):
        self.__port_manager = port_manager or yatest.common.network.PortManager()
        self.port = self.__port_manager.get_port()
        super().listen(self.port, address)

        self.__runing = True

    async def pop_stream(self, accept=True, timeout=None) -> IOStream:
        if not self.__runing:
            return None

        stream = await self.__incoming.get(timeout)
        self.__incoming.task_done()
        if stream == self.__Stop:
            return None

        return stream

    async def handle_stream(self, stream: IOStream, address):
        self.__incoming.put_nowait(stream)


class ConfigPatch:
    @staticmethod
    def config_patches(patch):
        for path, val in patch.items():
            if isinstance(val, dict):
                for child_path, child_val in ConfigPatch.config_patches(val):
                    yield path + "." + child_path, child_val
            else:
                yield path, val

    @staticmethod
    def swap_config_value(path, new_value):
        old_value = config
        for i in path.split("."):
            old_value = old_value[i]
        config.set_by_path(path, new_value)
        return old_value

    def __enter__(self):
        self.apply()
        return self

    def __exit__(self, *args, **kwargs):
        self.restore()

    def __init__(self, patch={}):
        self.patch = patch
        self.__old_values = None

    def apply(self):
        if self.__old_values is not None:
            raise SystemError("Config patch already applied")
        self.__old_values = {}
        for path, val in ConfigPatch.config_patches(self.patch):
            old_value = self.swap_config_value(path, val)
            self.__old_values[path] = old_value

    def restore(self):
        if self.__old_values is None:
            raise SystemError("Config patch wasn't applied")
        for path, val in self.__old_values.items():
            config.set_by_path(path, val)
