import os
import yatest

from alice.hollywood.library.python.testing.it_grpc.mock_server import MockServicer
from alice.hollywood.library.python.testing.it_grpc.wrappers import GraphResponse

import apphost.lib.proto_answers.http_pb2 as http_protos


class HttpRequestMockServicer(MockServicer):

    REQUEST_ITEM = 'http_request'
    RESPONSE_ITEM = 'http_response'

    def _assert_request(self, req):
        http_request_item = req.get_only_item(
            self.REQUEST_ITEM,
            http_protos.THttpRequest
        )
        assert http_request_item.Path == self.HTTP_REQUEST_PATH
        assert http_request_item.Content == self.HTTP_REQUEST_CONTENT

    def _prepare_response(self, req):
        item = http_protos.THttpResponse()
        item.StatusCode = 200
        item.Content = self.HTTP_RESPONSE_CONTENT

        resp = GraphResponse(req)
        resp.add_item(item, self.RESPONSE_ITEM)
        return resp

    def on_request(self, req):
        self._assert_request(req)
        return self._prepare_response(req)


def load_from_data(filename):
    path = yatest.common.source_path('alice/hollywood/library/scenarios/music/it_grpc/data')
    with open(os.path.join(path, filename)) as f:
        return f.read().encode()
