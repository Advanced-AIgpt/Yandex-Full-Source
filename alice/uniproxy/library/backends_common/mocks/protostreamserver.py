import tornado
from tornado import httputil
from tornado.tcpserver import TCPServer


# ------------------------------------------------------------------------------------------------
class ProtoStreamServerMock(TCPServer):
    def __init__(self, host, port):
        super().__init__()
        self._upgraded = False
        self._port = port

    async def upgrade(self, start_line, headers):
        assert start_line.method == 'GET'
        assert headers["Connection"] == "Upgrade"
        assert headers["Upgrade"] == "protobuf"
        assert headers["User-Agent"] == "KeepAliveClient"
        assert len(headers["Host"])

        headers = await self.on_upgrade(start_line, headers)
        resp = "HTTP/1.1 101 Upgraded\r\n"
        if headers:
            for k, v in headers.items():
                resp += k + ": " + v + "\r\n"
        resp += "\r\n"
        return resp.encode()

    async def on_upgrade(self, start_line, headers):
        raise NotImplementedError

    async def on_proto(self, proto):
        raise NotImplementedError

    def start(self):
        super().listen(self._port)

    async def handle_stream(self, stream, address):
        while True:
            try:
                if not self._upgraded:
                    data = await stream.read_until(b'\r\n')
                    start_line = httputil.parse_request_start_line(data.decode("utf-8").strip())

                    data = await stream.read_until(b'\r\n\r\n')
                    headers = httputil.HTTPHeaders.parse(data.decode("utf-8").strip())

                    data = await self.upgrade(start_line, headers)
                    await stream.write(data)
                    self._upgraded = True
                    continue

                hex_len = await stream.read_until(b'\r\n')
                hlen = int(hex_len.decode("utf-8").strip(), 16)
                proto = await stream.read_bytes(hlen)
                ret = await self.on_proto(proto)
                if ret:
                    await stream.write(("%X\r\n" % (len(ret))).encode())
                    await stream.write(ret)
            except tornado.iostream.StreamClosedError:
                break
