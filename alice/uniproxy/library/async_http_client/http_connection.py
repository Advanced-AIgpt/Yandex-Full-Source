import gzip
import time
import os
import sys

import tornado.iostream
import tornado.tcpclient
import tornado.gen

from alice.uniproxy.library.async_http_client.http_request import HTTPRequest
from alice.uniproxy.library.async_http_client.http_request import HTTPResponse
from alice.uniproxy.library.async_http_client.http_request import HTTPError
from alice.uniproxy.library.logging import Logger


LOG_ALL_HTTP = (os.environ.get("UNIPROXY_LOG_ALL_HTTP") == "YES")
if LOG_ALL_HTTP:
    print("Overall HTTP logging is enabled!", file=sys.stderr, flush=True)
else:
    print("Overall HTTP logging is disabled", file=sys.stderr, flush=True)


# ====================================================================================================================
class AsyncHTTPConnection(object):
    def __init__(self, host, port, secure=False, ca=None, conn_num=0, connect_timeout=1, retry_on_timeout=True):
        super(AsyncHTTPConnection, self).__init__()
        self._log = Logger.get('.httpconn')
        self._host = host
        self._port = port
        self._tcp_client = None
        self._stream = None
        self._connected = False
        self._secure = secure
        self._ca_certificate = ca
        self._conn_num = conn_num
        self._connect_timeout = connect_timeout
        self._retry_on_timeout = retry_on_timeout

    # ----------------------------------------------------------------------------------------------------------------
    def stream(self) -> tornado.iostream.IOStream:
        return self._stream

    def checked_stream(self) -> tornado.iostream.IOStream:
        if self._stream is None:
            raise HTTPError(HTTPError.CODE_CONNECTION_CANCELED, b'connection canceled')

        return self._stream

    @tornado.gen.coroutine
    def fetch(self, request: HTTPRequest):
        response = HTTPResponse(0, None, request)

        retries = request._retries
        self._log.debug('will retry this request for {} times'.format(retries))
        done = False
        while not done:
            retries -= 1

            try:
                if self._stream and self._stream.closed():
                    self._connected = False

                if not self._connected:
                    self._log.info('reconnecting to {}:{} ({})...'.format(self._host, self._port, self._conn_num))
                    yield self.reconnect(timeout=self._connect_timeout)

                if not self._connected:
                    raise HTTPError(
                        HTTPError.CODE_CONNECT_TIMEOUT,
                        ('timeout while connecting to %s:%d (%d)' % (
                            self._host,
                            self._port,
                            self._conn_num
                        )).encode('utf-8')
                    )

                response.request.set_sending()
                yield self._send_data(request)
                response.request.set_sent()
                yield self._read_headers(response, request.request_timeout)
                yield self._read_body(response, request.request_timeout)
                response.request.set_received()

                status = response.code // 100
                done = done or (status == 2)
                done = done or (status == 4)

                self._log.debug('response.code = {}'.format(response.code))
                if done:
                    if status != 2:
                        response.request.set_failed()
            except (TimeoutError, tornado.gen.TimeoutError):
                response.request.set_failed()
                self._log.error('{}:{} ({}) request {} timed out'.format(
                    self._host, self._port, self._conn_num, request.make_logable()))
                self.disconnect()
                if retries <= 0 or not self._retry_on_timeout:
                    raise HTTPError(HTTPError.CODE_REQUEST_TIMEOUT, b'request timed out', response.request)
            except IOError:
                if self._connected:
                    self._log.error('{}:{} ({}) connection dropped'.format(self._host, self._port, self._conn_num))
                    self.disconnect()
                    if request._retries == 1 and retries == 0:
                        self._log.info('{}:{} ({}) reconnect and try again'.format(
                            self._host, self._port, self._conn_num))
                        continue
                response.request.set_failed()
                if retries <= 0:
                    raise HTTPError(HTTPError.CODE_CONNECTION_DROPPED, b'connection dropped', request)
            except HTTPError as ex:
                if ex.request is None:
                    ex.request = request
                raise ex
            except Exception as ex:
                response.request.set_failed()
                self._log.exception(ex)

            done = done or (retries <= 0)

            if not done:
                self._log.debug('{}:{} will retry for {} times more'.format(self._host, self._port, retries))

        response.request.set_ready()

        if LOG_ALL_HTTP:
            self._log.debug(
                "\n-- URL: ------------------------------------------------------------------------\n" +
                ("https" if self._secure else "http") + f"://{self._host}:{self._port}" +
                "\n-- REQUEST: --------------------------------------------------------------------\n" +
                response.request._data.decode("utf-8") +
                "\n-- RESPONSE: -------------------------------------------------------------------\n" +
                response.serialize() +
                "\n--------------------------------------------------------------------------------\n"
            )
        return response

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _send_data(self, request: HTTPRequest, timeout=None):
        yield self._timeout_func(timeout, self.checked_stream().write(request._data))

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _read_headers(self, response: HTTPResponse, timeout=None):
        data, _ = yield self._timeout_func(timeout, self.stream().read_until(b'\r\n\r\n'))

        lines = data.decode('utf-8').split('\r\n')
        if len(lines) < 3:
            self._log.error('response contains invalid headers section: {}'.format(data))
            raise HTTPError(0, b'empty headers in response', response.request)

        for line in lines[1:]:
            r = [s.strip() for s in line.split(':', 1)]
            if len(r) != 2:
                continue

            name, value = r
            name = name.lower()

            if name == 'content-length':
                response.length = int(value)
                response.headers[name] = response.length
            elif name == 'transfer-encoding':
                if value.lower() == 'chunked':
                    response.chunked = True
            elif name == 'content-encoding':
                value_lower = value.lower()
                if value_lower == 'gzip':
                    response.gzipped = True
                elif value_lower == 'identity':
                    response.gzipped = False
            else:
                response.headers[name] = value

        http_status_line = lines[0]
        http_status_splitted = http_status_line.split()
        if len(http_status_splitted) < 3:
            self._log.error('response contains invalid first line: "{}"'.format(http_status_line))
            raise HTTPError(0, b'invalid response headers', response.request)

        try:
            response.code = int(http_status_splitted[1].strip())
        except Exception:
            self._log.error('response contains non-int http status code: "{}"'.format(http_status_splitted[1]))
            raise HTTPError(0, b'invalid response status code', response.request)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _read_body(self, response: HTTPResponse, timeout=None):
        if response.chunked:
            yield self._read_chunked_body(response, timeout)
        elif response.length > 0:
            yield self._read_simple_body(response, timeout)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _read_chunked_body(self, response: HTTPResponse, timeout=None):
        self._log.debug('reading chunked body')

        if response.body is None:
            response.body = b''

        done = False
        while not done:
            chunk_size_string, timeout = yield self._timeout_func(timeout, self.checked_stream().read_until(b'\r\n'))

            try:
                chunk_size = int(chunk_size_string.strip(), 16)
            except Exception as ex:
                self._log.error('invalid chunk size: "{}"'.format(chunk_size))
                raise ex

            if chunk_size == 0:
                self._log.debug('read last chunk')
                yield self._timeout_func(timeout, self.checked_stream().read_until(b'\r\n'))

                done = True
            else:
                data, timeout = yield self._timeout_func(timeout, self.checked_stream().read_bytes(chunk_size))
                _, timeout = yield self._timeout_func(timeout, self.checked_stream().read_bytes(2))
                self._log.debug('read chunk size={} data={}'.format(chunk_size, data))
                response.body += data

        if response.gzipped:
            response.body = gzip.decompress(response.body)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _read_simple_body(self, response: HTTPResponse, timeout=None):
        response.body, _ = yield self._timeout_func(timeout, self.checked_stream().read_bytes(response.length))

        if response.gzipped:
            response.body = gzip.decompress(response.body)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def reconnect(self, timeout=0, period=0):
        if self._connected and self._stream and not self._stream.closed():
            raise tornado.gen.Return(True)

        self.disconnect()
        started = time.time()
        current = started

        if period < 1.0:
            period = 1.0

        # ............................................................................................................
        self._log.info('{}:{} reconnecting...'.format(self._host, self._port))
        while not self._connected and (current - started < timeout or timeout == 0):
            self._connected = yield self.connect()
            current = time.time()
            if not self._connected:
                yield tornado.gen.sleep(period)

        # ............................................................................................................
        raise tornado.gen.Return(self._connected)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def connect(self, timeout=None, noexcept=True):
        if self._connected and self._stream and not self._stream.closed():
            self._log.debug('connection has already been established')
            raise tornado.gen.Return(True)

        try:
            self._log.debug('connecting...')
            if self._secure:
                if self._ca_certificate:
                    ssl_options = {
                        'ca_certs': self._ca_certificate,
                    }
                else:
                    ssl_options = {}
            else:
                ssl_options = None

            self._tcp_client = tornado.tcpclient.TCPClient()

            self._stream, timeout = yield self._timeout_func(
                timeout,
                self._tcp_client.connect(self._host, self._port, ssl_options=ssl_options)
            )
            self._connected = True
            self._log.info('connected to {}:{} ({})'.format(self._host, self._port, self._conn_num))
        except (tornado.gen.TimeoutError, TimeoutError):
            self._connected = False
        except Exception as ex:
            if not noexcept:
                raise
            self._log.error(ex)

        raise tornado.gen.Return(self._connected)

    # ----------------------------------------------------------------------------------------------------------------
    def disconnect(self):
        self._connected = False
        if not self._stream:
            return

        if not self._stream.closed():
            self._stream.close()
            self._log.info("stream closed")

        self._tcp_client.close()
        self._log.info("tcp_client closed")

        self._stream = None
        self._tcp_client = None

    @tornado.gen.coroutine
    def _timeout_func(self, timeout, future):
        if timeout is not None:
            start = time.monotonic()
            ret = yield tornado.gen.with_timeout(
                time.time() + timeout,
                future,
                quiet_exceptions=(tornado.iostream.StreamClosedError,),  # ignore closing stream error after timeout event
            )

            end = time.monotonic()
            return ret, timeout - (end - start)
        else:
            ret = yield future
            return ret, None
