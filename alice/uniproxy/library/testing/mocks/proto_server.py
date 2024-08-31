from .server import QueuedTcpServer, read_http_request
from alice.uniproxy.library.backends_common.protostream import ProtoStream
from tornado.concurrent import Future


class AsyncProtoStream(ProtoStream):
    async def read_protobuf(self, protos):
        fut = Future()
        super().read_protobuf(protos, lambda x: fut.set_result(x) if fut.running() else None)
        return await fut

    async def send_protobuf(self, proto):
        fut = Future()
        super().send_protobuf(proto, lambda x: fut.set_result(x) if fut.running() else None)
        return await fut


class ProtoServer(QueuedTcpServer):
    async def pop_proto_stream(self, accept=True, timeout=None) -> AsyncProtoStream:
        tcp_stream = await self.pop_tcp_stream(timeout=timeout)
        if tcp_stream is None:
            return None

        # handshake
        http_request = await read_http_request(tcp_stream)
        if not accept:
            tcp_stream.write(b"HTTP/1.1 503 Service Unavailable\r\n\r\n")
            return None
        await tcp_stream.write(b"HTTP/1.1 101 OK\r\n\r\n")

        proto_stream = AsyncProtoStream(host="", port=0, uri=http_request.path)
        proto_stream.headers = http_request.headers
        proto_stream._connected = True
        proto_stream.stream = tcp_stream
        return proto_stream
