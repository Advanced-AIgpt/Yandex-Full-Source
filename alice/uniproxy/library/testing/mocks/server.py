import yatest.common.network
from tornado.tcpserver import TCPServer, IOStream
from tornado import httputil
from tornado.queues import Queue
from tornado.gen import TimeoutError
import datetime


async def read_request_line(stream: IOStream):
    data = await stream.read_until(b"\r\n")
    return httputil.parse_request_start_line(data.decode("utf-8").strip())


async def read_request_headers(stream: IOStream):
    data = await stream.read_until(b"\r\n\r\n")
    return httputil.HTTPHeaders.parse(data.decode("utf-8").strip())


async def read_http_request(stream: IOStream) -> httputil.HTTPServerRequest:
    start_line = await read_request_line(stream)
    headers = await read_request_headers(stream)
    return httputil.HTTPServerRequest(start_line=start_line, headers=headers)


async def write_http_response(stream: IOStream, version="1.1", code=200, reason="OK", headers={}, body=""):
    response = (
        f"HTTP/{version} {code} {reason}\r\n" +
        "\r\n".join(f"{k}: {v}" for k, v in headers.items()) +
        "\r\n\r\n" +
        body
    )
    await stream.write(response.encode("utf-8"))


class QueuedTcpServer(TCPServer):
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
        self.address = None

    def stop(self):
        self.__runing = False
        super().stop()
        self.__incoming.put_nowait(None)
        self.__port_manager.release_port(self.port)

    def listen(self, address="", port_manager=None):
        self.__port_manager = port_manager or yatest.common.network.PortManager()
        self.port = self.__port_manager.get_port()
        self.address = address or "localhost"
        super().listen(self.port, address)
        self.__runing = True

    @property
    def endpoint(self):
        return f"{self.address}:{self.port}"

    async def pop_tcp_stream(self, accept=True, timeout=None) -> IOStream:
        if not self.__runing:
            return None

        if timeout is not None and not isinstance(timeout, datetime.timedelta):
            timeout = datetime.timedelta(seconds=timeout)

        try:
            stream = await self.__incoming.get(timeout)
            self.__incoming.task_done()
            return stream
        except TimeoutError as err:
            return None

    async def handle_stream(self, stream: IOStream, address):
        self.__incoming.put_nowait(stream)
