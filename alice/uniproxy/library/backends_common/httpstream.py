import time
import tornado.gen

from tornado.httpclient import AsyncHTTPClient, HTTPRequest
from tornado.ioloop import IOLoop
from tornado.iostream import StreamClosedError
from urllib.parse import urlsplit

from alice.uniproxy.library.async_http_client import HTTPError
from alice.uniproxy.library.async_http_client.http_connection import AsyncHTTPConnection
from alice.uniproxy.library.async_http_client.http_request import HTTPRequest as AsyncHTTPRequest
from alice.uniproxy.library.logging import Logger


class HttpStream(object):
    def __del__(self):
        self.close()

    def close(self):
        if self.client:
            self.client.close()
            self.client = None

    def error_callback(self, result):
        self._log.error("error_callback:", result.error, result.body)

    def __init__(self, url, on_result, on_chunk=None, on_error=None, method='GET', body=None, **kwars):
        self._log = Logger.get('.backends.httpstream')
        self.create_client(url)
        self.on_result = on_result
        self.on_error = on_error if on_error else self.error_callback

        request = HTTPRequest(url=url, method=method, **kwars)
        if callable(body):
            request.body_producer = body
        else:
            request.body = body
        if on_chunk is not None:
            request.streaming_callback = on_chunk
        self.client.fetch(request, self.process_result)

    def create_client(self, url):  # separate method for mocking self.client
        self.client = AsyncHTTPClient(force_instance=True)

    def process_result(self, result):
        if result.error:
            self.on_error(result)
            self.close()
        else:
            self.on_result(result)
            self.close()


class AsyncHttpStream(object):
    def __init__(
        self,
        url,
        on_result,
        on_error=None,
        method='GET',
        body=None,  # bytes
        headers=None,
        connect_timeout=None,
        connect_retries=0,
        request_timeout=None,
    ):
        self._log = Logger.get('.backends.asynchttpstream')
        self.url = url
        self.method = method
        self.body = body
        self.headers = headers
        self.on_result = on_result
        self.on_error = on_error if on_error else self.default_error_callback
        self.connect_timeout = connect_timeout
        self.connect_retries = connect_retries
        self.request_timeout = request_timeout

        purl = urlsplit(url)
        secure = purl.scheme.lower() == 'https'
        port = purl.port
        if port is None:
            if secure:
                port = 443
            else:
                port = 80
        self.connection = AsyncHTTPConnection(
            purl.hostname,
            port,
            secure=secure,
            connect_timeout=1,
        )
        # spawn worker coroutine
        IOLoop.current().spawn_callback(self.worker)

    @tornado.gen.coroutine
    def worker(self):
        start = time.monotonic()
        request_timeout = self.request_timeout
        if request_timeout is None:
            request_timeout = 24 * 60 * 60  # 1day
        connect_timeout = self.connect_timeout
        if connect_timeout is None:
            connect_timeout = request_timeout
        connect_timeout = min(connect_timeout, request_timeout)
        begin_process_result = False
        request = None
        try:
            request = AsyncHTTPRequest(
                self.url,
                method=self.method,
                headers=self.headers,
                body=self.body,
                request_timeout=request_timeout,
            )

            while True:  # loop connect retries
                connected = False
                err = None
                try:
                    if self.connection is None:
                        self.connect_retries = 0
                        raise Exception('connection canceled')

                    connected = yield self.connection.connect(timeout=connect_timeout, noexcept=False)
                except StreamClosedError as exc:
                    if exc.real_error:
                        err = 'connection failed [SCE]: {} ({})'.format(str(exc), str(exc.real_error))
                    else:
                        err = 'connection failed [SCE]: {}'.format(str(exc))
                except Exception as exc:
                    err = 'connection failed: {}'.format(str(exc))
                if connected:
                    break  # SUCCESS

                if self.connect_retries:
                    used_time = time.monotonic() - start
                    if request_timeout > used_time:
                        connect_timeout = min(connect_timeout, request_timeout - used_time)
                        self.connect_retries -= 1
                        continue
                if err is None:
                    err = 'connection timedout'
                self.on_error(HTTPError(599, err.encode('utf-8'), request))
                return

            if not self.connection:  # race with concurrent call self.close()
                raise Exception('connection canceled')

            # calc connection duration, request_timeout -= duration
            used_time = time.monotonic() - start
            if request_timeout <= used_time:
                raise Exception('connection timedout [2]')

            request_timeout -= used_time
            request.request_timeout = request_timeout

            request.prepare()  # <<< utils HTTPRequest guts sticking out here
            response = yield self.connection.fetch(request)
            begin_process_result = True
            self.process_result(response)
        except HTTPError as exc:
            self.on_error(exc)
        except Exception as exc:
            if begin_process_result:
                self._log.exception('{} got exception from callback: "{}"'.format(self.__class__.__name__, str(exc)))
            else:
                self.on_error(HTTPError(599, str(exc).encode('utf-8'), request))
        finally:
            self.close()

    def close(self):
        if self.connection:
            self.connection.disconnect()
            self.connection = None

    def default_error_callback(self, result):
        self._log.error('{} default_error_callback: {}'.format(self.__class__.__name__, str(result)))

    def process_result(self, result):
        if result.code >= 200 and result.code < 300:
            self.on_result(result)
            self.close()
        else:
            self.on_error(result)
            self.close()
