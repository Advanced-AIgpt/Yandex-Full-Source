import time
import logging
import ssl
import os

import tornado.web
import tornado.ioloop
import tornado.httpserver

import alice.uniproxy.library.testing
from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPRequest, HTTPResponse, HTTPError


logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)12s: %(name)-16s %(message)s')


CONFIG_TEST_SERVER_PORT = 9042
CONFIG_TEST_SERVER_SECURE_PORT = 9043
CONFIG_TEST_SERVER_INVALID_PORT = 55042
CONFIG_TEST_CLIENT_POOL_SIZE = 4
CONFIG_TEST_CLIENT_QUEUE_SIZE = 100

CONFIG_TEST_CA_CERT = os.path.join(os.path.dirname(__file__), 'cert', 'uniproxy_ca.crt')
CONFIG_TEST_SERVER_CERTIFICATE = os.path.join(os.path.dirname(__file__), 'cert', 'uniproxy.crt')
CONFIG_TEST_SERVER_KEY = os.path.join(os.path.dirname(__file__), 'cert', 'uniproxy.key')


# ====================================================================================================================
class TimeoutHandler(tornado.web.RequestHandler):
    request_counter = 0

    @tornado.gen.coroutine
    def get(self):
        TimeoutHandler.request_counter += 1
        try:
            timeout_string = self.get_argument('ms', '50')
            timeout_value = float(timeout_string)
            timeout_seconds = timeout_value / 1000

            yield tornado.gen.sleep(timeout_seconds)

            self.finish({
                'code': 200,
            })
        except Exception as ex:
            self.set_status(400)
            self.finish({
                'code': 400,
                'error': str(ex)
            })


# ====================================================================================================================
class StatusCodeHandler(tornado.web.RequestHandler):
    @tornado.gen.coroutine
    def get(self):
        status_string = self.get_argument('status', '200')
        try:
            status_value = int(status_string)

            self.set_status(status_value)
            self.finish(status_string)

        except Exception as ex:
            self.set_status(500)
            self.finish(('REQUESTED %s\nERROR: %s\n' % (status_string, str(ex))).encode('utf-8'))


# ====================================================================================================================
class ChunkedResponseHandler(tornado.web.RequestHandler):
    @tornado.gen.coroutine
    def _reply(self, period=0.05):
        self.write('first line\n')
        self.flush()
        yield tornado.gen.sleep(period)
        self.write('seconds line\n')
        self.flush()
        yield tornado.gen.sleep(period)
        self.write('third line\n')
        self.flush()
        yield tornado.gen.sleep(period)
        self.finish('end of response')

    @tornado.gen.coroutine
    def get(self):
        yield self._reply()

    @tornado.gen.coroutine
    def post(self):
        yield self._reply()


# ====================================================================================================================
class ChunkedRequestHandler(tornado.web.RequestHandler):
    def post(self):
        pass


# ====================================================================================================================
class RetriesHandler(tornado.web.RequestHandler):
    COUNTER = 0

    def get(self):
        RetriesHandler.COUNTER += 1
        if RetriesHandler.COUNTER % 4 == 0:
            self.finish('ok')
        else:
            self.set_status(500)
            self.finish('500')


# ====================================================================================================================
class SimpleHandler(tornado.web.RequestHandler):
    def _reply_text(self):
        text = ''
        text += 'Method: %s\n' % (self.request.method, )
        text += 'Uri:    %s\n' % (self.request.uri, )
        text += 'Path:   %s\n' % (self.request.path, )
        text += 'Query:  %s\n' % (self.request.query, )
        for header, value in self.request.headers.items():
            text += 'Header(%s): %s\n' % (header, value, )
        self.set_header('Content-Type', 'text/plain;encoding=utf-8')
        self.finish(text)

    # ----------------------------------------------------------------------------------------------------------------
    def _reply_text_big(self):
        text = ''
        text += 'Method: %s\n' % (self.request.method, )
        text += 'Uri:    %s\n' % (self.request.uri, )
        text += 'Path:   %s\n' % (self.request.path, )
        text += 'Query:  %s\n' % (self.request.query, )
        for header, value in self.request.headers.items():
            text += 'Header(%s): %s\n' % (header, value, )

        text = text * 100

        self.finish(text)

    # ----------------------------------------------------------------------------------------------------------------
    def _reply_json(self):
        data = {
            'Method': self.request.method,
            'Uri': self.request.uri,
            'Path': self.request.path,
            'Query': self.request.query,
            'Headers': {}
        }

        for header, value in self.request.headers.items():
            data['Headers'][header] = value

        self.finish(data)

    # ----------------------------------------------------------------------------------------------------------------
    def _reply(self):
        if self._content == 'text':
            self._reply_text()
        elif self._content == 'json':
            self._reply_json()
        elif self._content == 'gzip':
            self._reply_text_big()

    # ----------------------------------------------------------------------------------------------------------------
    def initialize(self, content):
        self._content = content
        if content == 'text':
            self._content_type = 'text/plain'
        elif content == 'json':
            self._content_type = 'application/json'
        elif content == 'gzip':
            self._content_type = 'text/plain'
        else:
            self._content = 'text'
            self._content_type = 'text/plain'

    # ----------------------------------------------------------------------------------------------------------------
    def get(self):
        self._reply()

    # ----------------------------------------------------------------------------------------------------------------
    def post(self):
        self._reply()

    # ----------------------------------------------------------------------------------------------------------------
    def delete(self):
        self._reply()


# ====================================================================================================================
class HttpServer:
    def __init__(self, port=CONFIG_TEST_SERVER_PORT, secure=False):
        self._app = tornado.web.Application([
            (r'/timeout', TimeoutHandler),
            (r'/chunked_response', ChunkedResponseHandler),
            (r'/chunked_request', ChunkedRequestHandler),
            (r'/status', StatusCodeHandler),
            (r'/text/.*', SimpleHandler, dict(content='text')),
            (r'/json/.*', SimpleHandler, dict(content='json')),
            (r'/gzip/.*', SimpleHandler, dict(content='gzip')),
            (r'/retries', RetriesHandler),
        ], compress_response=True)

        if secure:
            ssl_context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
            ssl_context.load_cert_chain(CONFIG_TEST_SERVER_CERTIFICATE, CONFIG_TEST_SERVER_KEY)
        else:
            ssl_context = None

        self._srv = tornado.httpserver.HTTPServer(self._app, ssl_options=ssl_context)
        self._srv.bind(port)

    def start(self):
        self._srv.start(1)

    def stop(self):
        self._srv.stop()

    def __enter__(self, *args, **kwargs):
        self.start()
        return self

    def __exit__(self, *args, **kwargs):
        self.stop()


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_status_200_ok():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT)
        yield tornado.gen.sleep(0.03)

        response = yield client.fetch(HTTPRequest(
            '/status'
        ))
        if response.code != 200:
            print(response.body)
        assert response.code == 200

    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_free_workers():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT, pool_size=1, queue_size=1)
        yield tornado.gen.sleep(0.03)

        assert client.free_workers == 1

        response_future = client.fetch(HTTPRequest(
            '/timeout'
        ))

        yield tornado.gen.sleep(0.02)

        assert client.free_workers == 0

        response = yield response_future

        assert client.free_workers == 1
        assert response.code == 200

    yield tornado.gen.sleep(0.05)


@tornado.gen.coroutine
def _test_error_status_impl(code):
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT)
        yield tornado.gen.sleep(0.03)

        error = None
        try:
            response = yield client.fetch(HTTPRequest(
                '/status?status=%s' % code
            ))
            print(response)
        except HTTPError as err:
            error = err

        assert error is not None
        assert error.code == code

    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_status_400_error():
    yield _test_error_status_impl(400)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_status_404_error():
    yield _test_error_status_impl(404)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_status_500_error():
    yield _test_error_status_impl(500)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_status_503_error():
    yield _test_error_status_impl(503)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_status_504_error():
    yield _test_error_status_impl(504)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_request_timeout():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT)
        yield tornado.gen.sleep(0.03)

        error = None
        try:
            response = yield client.fetch(HTTPRequest(
                '/timeout',
                request_timeout=0.035
            ))
        except HTTPError as e:
            error = e

        assert error
        assert error.code == HTTPError.CODE_REQUEST_TIMEOUT
    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_retries_on_request_timeout():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT)
        yield tornado.gen.sleep(0.03)

        rc = TimeoutHandler.request_counter
        error = None
        try:
            response = yield client.fetch(HTTPRequest('/timeout', request_timeout=0.035, retries=4))
        except HTTPError as e:
            error = e
        assert error
        assert error.code == HTTPError.CODE_REQUEST_TIMEOUT
        assert TimeoutHandler.request_counter == rc + 4

    yield tornado.gen.sleep(0.05)

# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_no_retries_on_request_timeout():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT, retry_on_timeout=False)
        yield tornado.gen.sleep(0.03)

        # check that retries aren't made on timeout
        rc = TimeoutHandler.request_counter
        error = None
        try:
            response = yield client.fetch(HTTPRequest('/timeout', request_timeout=0.035, retries=4))
        except HTTPError as e:
            error = e
        assert error
        assert error.code == HTTPError.CODE_REQUEST_TIMEOUT
        assert TimeoutHandler.request_counter == rc + 1

        # check that retries are made on server errors
        error = None
        try:
            response = yield client.fetch(HTTPRequest('/retries', request_timeout=0.035, retries=4))
        except HTTPError as e:
            error = e
        assert error is None
        assert response.code == 200
        assert response.text() == 'ok'

    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_chunked_response():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT)
        yield tornado.gen.sleep(0.03)

        error = None
        try:
            response = yield client.fetch(HTTPRequest(
                '/chunked_response'
            ))
        except HTTPError as e:
            error = e

        assert error is None
        assert response.code == 200

        text = response.text()
        chunks = text.split('\n')
        assert len(chunks) == 4
        assert chunks[0] == 'first line'
        assert chunks[-1] == 'end of response'

    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_request():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT)
        yield tornado.gen.sleep(0.03)

        error = None
        try:
            response = yield client.fetch(HTTPRequest(
                '/text/',
                method='POST'
            ))
        except HTTPError as e:
            error = e

        assert error is None
        assert response.code == 200

        assert len(response.text()) > 20
        assert 'POST' in response.text()

    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_post_request_json():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT)
        yield tornado.gen.sleep(0.03)

        error = None
        try:
            response = yield client.fetch(HTTPRequest(
                '/json/',
                method='POST'
            ))
        except HTTPError as e:
            error = e

        assert error is None
        assert response.code == 200

        assert len(response.text()) > 20
        assert response.json()['Method'] == 'POST'

    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_delete_request():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT)
        yield tornado.gen.sleep(0.03)

        error = None
        try:
            response = yield client.fetch(HTTPRequest(
                '/text/',
                method='DELETE'
            ))
        except HTTPError as e:
            error = e

        assert error is None
        assert response.code == 200

        assert len(response.text()) > 20
        assert 'DELETE' in response.text()

    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_delete_request_json():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT)
        yield tornado.gen.sleep(0.03)

        error = None
        try:
            response = yield client.fetch(HTTPRequest(
                '/json/',
                method='DELETE'
            ))
        except HTTPError as e:
            error = e

        assert error is None
        assert response.code == 200

        assert len(response.text()) > 20
        assert response.json()['Method'] == 'DELETE'

    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_retries():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT)
        yield tornado.gen.sleep(0.03)

        error = None
        try:
            response = yield client.fetch(HTTPRequest(
                '/retries',
                retries=5
            ))
        except HTTPError as e:
            error = e

        assert error is None
        assert response.code == 200

        assert response.text() == 'ok'

    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_encoding_gzip():
    with HttpServer():
        client = QueuedHTTPClient(host='localhost', port=CONFIG_TEST_SERVER_PORT)
        yield tornado.gen.sleep(0.03)

        error = None
        try:
            response = yield client.fetch(HTTPRequest(
                '/gzip/',
                retries=1,
                headers={
                    'Accept-Encoding': 'gzip;q=0.9,identity;q=0.1',
                }
            ))
        except HTTPError as e:
            error = e

        assert error is None
        assert response.code == 200

        assert 'gzip' in response.text()


        try:
            response = yield client.fetch(HTTPRequest(
                '/text/',
                retries=1,
                headers={
                    'Accept-Encoding': 'gzip,identity;q=0',
                }
            ))
        except HTTPError as e:
            error = e

        assert error is None
        assert response.code == 200

        assert 'text' in response.text()

        try:
            response = yield client.fetch(HTTPRequest(
                '/gzip/',
                retries=1,
                headers={
                    'Accept-Encoding': 'identity',
                }
            ))
        except HTTPError as e:
            error = e
        assert error is None
        assert response.code == 200
        assert 'gzip' in response.text()

        try:
            response = yield client.fetch(HTTPRequest(
                '/text/',
                retries=1,
                headers={
                    'Accept-Encoding': 'gzip',
                }
            ))
        except HTTPError as e:
            error = e
        assert error is None
        assert response.code == 200
        assert 'text' in response.text()


    yield tornado.gen.sleep(0.05)

# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_https():
    with HttpServer(port=CONFIG_TEST_SERVER_SECURE_PORT, secure=True):
        client = QueuedHTTPClient(
            host='localhost',
            port=CONFIG_TEST_SERVER_SECURE_PORT,
            secure=True,
            ca=CONFIG_TEST_CA_CERT
        )
        yield tornado.gen.sleep(0.03)

        error = None
        try:
            response = yield client.fetch(HTTPRequest(
                '/retries',
                retries=5,
                request_timeout=0.03
            ))
        except HTTPError as e:
            error = e

        assert error is None
        assert response.code == 200

        assert response.text() == 'ok'

    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_https_ignore_ca():
    with HttpServer(port=CONFIG_TEST_SERVER_SECURE_PORT, secure=True):
        client = QueuedHTTPClient(
            host='localhost',
            port=CONFIG_TEST_SERVER_SECURE_PORT,
            secure=True
        )
        yield tornado.gen.sleep(0.03)

        error = None
        try:
            response = yield client.fetch(HTTPRequest(
                '/retries',
                retries=5,
                request_timeout=0.03
            ))
        except HTTPError as e:
            error = e

        assert error is None
        assert response.code == 200

        assert response.text() == 'ok'

    yield tornado.gen.sleep(0.05)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_http_no_service():
    client = QueuedHTTPClient(
        host='localhost',
        port=CONFIG_TEST_SERVER_INVALID_PORT,
        pool_size=1,
        queue_size=5
    )
    yield tornado.gen.sleep(0.03)

    error = None
    try:
        response = yield client.fetch(HTTPRequest(
            '/retries',
            retries=1,
            request_timeout=0.03
        ))
    except HTTPError as e:
        error = e

    assert error.code == HTTPError.CODE_CONNECT_TIMEOUT
