from tornado.httputil import HTTPHeaders
from tornado.tcpclient import TCPClient
from collections import namedtuple


HTTPResponse = namedtuple("HTTPResponse", ("code", "reason", "headers", "body"))


async def _read_http_headers(stream):
    header_lines = await stream.read_until(b"\r\n\r\n")
    headers = HTTPHeaders()
    for line in header_lines.decode("utf-8").strip().split("\r\n"):
        headers.parse_line(line)
    return headers


async def _write_chunked_body(stream, body_gen):
    async for chunk in body_gen():
        data = f"{len(chunk):X}".encode("utf-8") + b"\r\n" + chunk + b"\r\n"
        await stream.write(data)
    await stream.write(b"0\r\n\r\n")


async def write_http_request(stream, host, url, method="GET", headers={}, body=b""):
    data = (
        f"{method} {url} HTTP/1.1\r\n" +
        "\r\n".join(f"{key}: {value}" for key, value in headers.items() if value is not None) +
        "\r\n\r\n"
    ).encode("utf-8")

    if not callable(body):
        await stream.write(data + body)
    else:
        await stream.write(data)
        await _write_chunked_body(stream, body)


async def read_http_response(stream):
    first_line = await stream.read_until(b"\r\n")
    _, code, reason = first_line.decode("utf-8").strip().split(" ", maxsplit=2)
    headers = await _read_http_headers(stream)
    body = await stream.read_bytes(int(headers["Content-Length"]))  # doesn't support chunked mode for now
    return HTTPResponse(code=int(code), reason=reason, headers=headers, body=body)


async def http_request(host, port, url, method="GET", headers=None, body=b""):
    """
    Why?
    1. Ability to avoid default HTTP headers added by Tornado AsyncHTTPClient
    2. Support chunked-encoding in requests
    """
    stream = await TCPClient().connect(host=host, port=port)
    try:
        # default HTTP headers may be skipped by setting None in `headers` parameter
        if headers is None:
            headers = {}
        headers.setdefault("Host", host)
        if not callable(body):
            headers.setdefault("Content-Length", len(body))
        else:
            headers.setdefault("Transfer-Encoding", "chunked")

        try:
            await write_http_request(stream, host, url, method, headers, body)
        except:
            pass

        return await read_http_response(stream)
    finally:
        stream.close()
