import tornado.gen
import tornado.ioloop
import weakref

from alice.uniproxy.library.async_http_client.http_request import HTTPResponse


class FakeStream:
    def __init__(self, url, on_result, method, on_error, body, headers, connect_timeout, connect_retries, request_timeout):
        self.result_callback = weakref.WeakMethod(on_result)
        tornado.ioloop.IOLoop.current().spawn_callback(self.return_result)

    @tornado.gen.coroutine
    def return_result(self):
        yield tornado.gen.sleep(0.0)
        response = HTTPResponse(code=200, body='{"response":{}}'.encode('utf-8'))
        self.result_callback()(response)

    def close(self):
        pass
