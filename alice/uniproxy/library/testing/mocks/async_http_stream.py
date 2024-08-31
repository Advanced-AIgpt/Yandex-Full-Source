from alice.uniproxy.library.async_http_client.http_request import HTTPResponse
import json
import tornado.gen
import weakref


class FakeAsyncHttpStream:
    REQUESTS = []

    def __init__(self, url, on_result, method, on_error, body, headers, connect_timeout, connect_retries, request_timeout):
        self.body = body
        self.result_callback = weakref.WeakMethod(on_result)
        FakeAsyncHttpStream.REQUESTS.append(self)

    @tornado.gen.coroutine
    def return_result(self, result=None):
        yield tornado.gen.sleep(0.0)
        if result is None:
            body = '{"response":{}}'.encode('utf-8')
        else:
            body = json.dumps(result).encode('utf-8')
        response = HTTPResponse(code=200, body=body)
        self.result_callback()(response)

    def close(self):
        pass
