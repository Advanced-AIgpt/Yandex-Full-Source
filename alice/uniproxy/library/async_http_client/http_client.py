import tornado.gen
import tornado.locks
import tornado.util
import tornado.queues

from urllib.parse import urlsplit


from alice.uniproxy.library.async_http_client.http_request import HTTPRequest
from alice.uniproxy.library.async_http_client.http_request import HTTPResponse
from alice.uniproxy.library.async_http_client.http_request import HTTPError

from alice.uniproxy.library.async_http_client.http_connection import AsyncHTTPConnection

from alice.uniproxy.library.async_http_client.settings import HTTP_CLIENT_DEFAULT_POOL_SIZE
from alice.uniproxy.library.async_http_client.settings import HTTP_CLIENT_DEFAULT_QUEUE_SIZE

from alice.uniproxy.library.logging import Logger


# ====================================================================================================================
class _Request(object):
    def __init__(self, http_request):
        self.request = http_request
        self.response = tornado.concurrent.Future()

    # ----------------------------------------------------------------------------------------------------------------
    def set_result(self, result: HTTPResponse):
        self.response.set_result(result)

    # ----------------------------------------------------------------------------------------------------------------
    def set_exception(self, ex):
        self.response.set_exception(ex)


# ====================================================================================================================
class QueuedHTTPClient(object):
    __clients = {}

    @classmethod
    def get_client(
        cls,
        host,
        port,
        secure=False,
        ca=None,
        **kwargs
    ):
        args = host, port, secure, ca

        client = cls.__clients.get(args)
        if client:
            return client

        cls.__clients[args] = client = cls(
            host=host,
            port=port,
            secure=secure,
            ca=ca,
            **kwargs
        )
        return client

    @classmethod
    def get_client_by_url(cls, url, ca=None, **kwargs):
        host, port, secure = cls._parse_url(url)
        return cls.get_client(host, port, secure, ca, **kwargs)

    def _parse_url(url):
        parsed = urlsplit(url)
        secure = parsed.scheme == 'https'
        port = parsed.port
        if port is None:
            port = 443 if secure else 80

        return parsed.hostname, port, secure

    def __init__(
        self,
        host,
        port,
        pool_size=HTTP_CLIENT_DEFAULT_POOL_SIZE,
        queue_size=HTTP_CLIENT_DEFAULT_QUEUE_SIZE,
        secure=False,
        ca=None,
        retry_on_timeout=True,
        wait_if_queue_is_full=True,
        connect_timeout=1,
    ):
        super(QueuedHTTPClient, self).__init__()
        self._log = Logger.get('.httpclient')
        self._host = host
        self._port = port
        self._pool_size = pool_size
        self._queue = tornado.queues.Queue(queue_size)
        self._ca_certificate = ca
        self._secure = secure
        self._workers = []
        self._retry_on_timeout = retry_on_timeout
        self._wait_if_queue_is_full = wait_if_queue_is_full
        self._connect_timeout = connect_timeout
        self.start_workers()
        self.free_workers = self._pool_size

    @tornado.gen.coroutine
    def _worker_main(self, index, completed):
        try:
            client = AsyncHTTPConnection(
                host=self._host,
                port=self._port,
                secure=self._secure,
                ca=self._ca_certificate,
                conn_num=index,
                retry_on_timeout=self._retry_on_timeout,
                connect_timeout=self._connect_timeout,
            )
            yield client.connect()
            while True:
                try:
                    req = yield self._queue.get()
                    self.free_workers -= 1
                    if req == 'stop':
                        self._log.debug('worker {} stopping'.format(index))
                        break
                    self._log.debug('worker {} processing request'.format(index))
                except Exception as ex:
                    self._log.exception(ex)
                    continue

                try:
                    req.request.set_sending()
                    response = yield client.fetch(req.request)
                    if response.code // 100 != 2:
                        raise HTTPError(response.code, response.body, req.request)
                    self._log.debug('{} <= {}'.format(response, req.request.make_logable()))
                    req.response.set_result(response)
                except HTTPError as ex:
                    self._log.debug(ex)
                    req.response.set_exception(ex)
                except Exception as ex:
                    req.response.set_exception(ex)
                finally:
                    self.free_workers += 1
                    self._queue.task_done()
            client.disconnect()
        finally:
            completed.set_result(None)

    def start_workers(self):
        for i in range(0, self._pool_size):
            f = tornado.concurrent.Future()
            tornado.ioloop.IOLoop.current().spawn_callback(self._worker_main, i + 1, f)
            self._workers.append(f)

    @tornado.gen.coroutine
    def stop(self):
        self._log.debug('[{0}:{1}] stopping'.format(self._host, self._port))
        for _ in self._workers:
            self._queue.put("stop")
        for f in self._workers:
            yield f
        self._log.debug('[{0}:{1}] stopped'.format(self._host, self._port))

    def _try_put_request_to_queue(self, request: _Request):
        if self._wait_if_queue_is_full:
            self._queue.put(request)
        else:
            try:
                self._queue.put_nowait(request)
            except tornado.queues.QueueFull:
                raise HTTPError(
                    code=429,
                    body="Too Many Requests (Queue of QueuedHTTPClient is full)",
                    request=request.request,
                )

    @tornado.gen.coroutine
    def send_request(self, req: HTTPRequest):
        request = _Request(req)
        self._try_put_request_to_queue(request)
        req.set_queued()
        return (yield request.response)

    @tornado.gen.coroutine
    def fetch(self, request, retries=None, raise_error=True, **kwargs):
        if not isinstance(request, HTTPRequest):
            request = HTTPRequest(query=request, **kwargs)
        else:
            if kwargs:
                raise ValueError("kwargs can't be used if request is an HTTPRequest object")

        request.prepare()
        if retries is not None:
            request.set_retries(retries)

        try:
            response = yield self.send_request(request)
        except HTTPError as e:
            request.set_failed()
            if raise_error:
                raise
            response = HTTPResponse(
                code=e.code,
                body=e.body,
                request=e.request
            )

        return response
