from alice.uniproxy.library.backends_common.protostream import ProtoStream
from alice.uniproxy.library.settings import config

from tornado.tcpserver import TCPServer, IOStream, IOLoop
from tornado import httputil
from tornado.concurrent import Future
from tornado.queues import Queue
import tornado.gen


async def _read_request_line(stream):
    data = await stream.read_until(b"\r\n")
    return httputil.parse_request_start_line(data.decode("utf-8").strip())


async def _read_request_headers(stream):
    data = await stream.read_until(b"\r\n\r\n")
    return httputil.HTTPHeaders.parse(data.decode("utf-8").strip())


class AsyncProtoStream(ProtoStream):
    async def read_protobuf(self, protos, timeout=None):
        fut = Future()
        super().read_protobuf(protos, lambda x: fut.set_result(x) if fut.running() else None)
        if timeout is not None:
            return await tornado.gen.with_timeout(timeout, fut)
        return await fut

    async def send_protobuf(self, proto, timeout=None):
        fut = Future()
        super().send_protobuf(proto, lambda x: fut.set_result(x) if fut.running() else None)
        if timeout is not None:
            return await tornado.gen.with_timeout(timeout, fut)
        return await fut


class ProtoServer(TCPServer):
    class __Stop:
        pass

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.__incoming = Queue()
        self.__protostreams = []  # needed to close all streams on 'stop()'
        self.port = None

    def stop(self):
        super().stop()
        self.__incoming.put_nowait(self.__Stop)
        for ps in self.__protostreams:
            ps.close()

    def listen(self, port=None, address=""):
        """ Server listens either on given port or looks for a free one (>50000)
        """
        if port is not None:
            super().listen(port, address)
            self.port = port
            return
        for port in range(50000, 0x10000):
            try:
                super().listen(port, address)
                self.port = port
                return
            except OSError:
                pass

    async def handle_proto_stream(self, accept=True, timeout=None) -> AsyncProtoStream:
        """ Accepts or rejects new incoming protobuf stream and returns it
        It returns None if the server's been stopped or 'accept=False'
        """

        s = await self.__incoming.get(timeout)
        self.__incoming.task_done()
        if s == self.__Stop:
            return

        method, path, version = await _read_request_line(s)
        headers = await _read_request_headers(s)
        if not accept:
            await s.write("HTTP/1.1 404 Rejected\r\n\r\n".encode("utf-8"))
            s.close()
            return

        await s.write("HTTP/1.1 101 OK\r\n\r\n".encode("utf-8"))

        ps = AsyncProtoStream(host="", port=0, uri=path)
        ps._connected = True
        ps.stream = s

        self.__protostreams.append(ps)
        return ps

    async def handle_stream(self, stream: IOStream, address):
        self.__incoming.put_nowait(stream)
