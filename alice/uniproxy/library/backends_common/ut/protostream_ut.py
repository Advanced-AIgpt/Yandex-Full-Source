from alice.uniproxy.library.backends_common.protostream import ProtoStream
import common
import tornado.gen
import tornado.concurrent


class FakeProtoType:
    def __init__(self, data=b"1234567890"):
        self.data = data

    def ParseFromString(self, message):
        self.data = message

    def SerializeToString(self):
        return self.data


class ProtoStreamWrap(ProtoStream):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._established = tornado.concurrent.Future()

    async def established(self):
        return await self._established

    def process(self):  # invoked on successfull handshake
        self._established.set_result(True)

    def mark_as_failed(self):  # invoked on stream's failure
        self._established.set_result(False)


@common.run_async
async def test_simple_connect_and_handshake():
    with common.QueuedTcpServer() as srv:
        protostream = ProtoStreamWrap(host="localhost", port=srv.port, uri="/some/path")
        protostream.connect()

        server_stream = await srv.pop_stream()

        request = await common.read_http_request(server_stream)
        assert request.method == "GET"
        assert request.path == "/some/path"
        assert request.headers["User-Agent"] == "KeepAliveClient"
        assert request.headers["Upgrade"] == "protobuf"
        assert request.headers["Connection"] == "Upgrade"

        await server_stream.write(b"HTTP/1.1 101 OK\r\n\r\n")

        assert (await protostream.established())


@common.run_async
async def test_send_receive_and_close():
    with common.QueuedTcpServer() as srv:
        protostream = ProtoStreamWrap(host="localhost", port=srv.port, uri="/some/path")
        protostream.connect()

        server_stream = await srv.pop_stream()

        # handshake
        await common.read_http_request(server_stream)
        await server_stream.write(b"HTTP/1.1 101 OK\r\n\r\n")
        await protostream.established()

        # client sends to server
        fut_result = tornado.concurrent.Future()
        protostream.send_protobuf(FakeProtoType(b"1234567890"), lambda success: fut_result.set_result(success))
        assert b"0xa\r\n" == await server_stream.read_until(b"\r\n")
        assert b"1234567890" == await server_stream.read_bytes(10)
        assert (await fut_result)

        # client receives from server
        fut_result = tornado.concurrent.Future()
        protostream.read_protobuf(FakeProtoType, lambda result: fut_result.set_result(result))
        await server_stream.write(b"0x10\r\n0123456789ABCDEF")
        assert b"0123456789ABCDEF" == (await fut_result).data

        # client closes
        protostream.close()
        assert protostream.is_closed()
        assert b"" == await server_stream.read_until_close()


@common.run_async
async def test_bad_handshake():
    with common.QueuedTcpServer() as srv:
        protostream = ProtoStreamWrap(host="localhost", port=srv.port, uri="/some/path")
        protostream.connect()

        server_stream = await srv.pop_stream()
        await common.read_http_request(server_stream)
        await server_stream.write(b"HTTP/1.1 200 OK\r\n\r\n")  # '101 OK' is needed

        assert not (await protostream.established())
