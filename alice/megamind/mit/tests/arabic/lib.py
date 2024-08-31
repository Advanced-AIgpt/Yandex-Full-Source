import json

from urllib.parse import parse_qsl, urlparse

from apphost.lib.proto_answers.http_pb2 import THttpRequest, THttpResponse


REQUEST_PHRASE = 'رقم عشوائي من 1 إلى 1000'  # случайное число от 1 до 1000
TRANSLATED_PHRASE = 'случайное число от 1 до 1000'


def not_visited_node_checker(ctx):
    assert False, "This node should not be visited"


def generate_polyglot_handler():
    def handler(ctx):
        request = THttpRequest()
        request.ParseFromString(ctx.get_protobuf_item(b'http_request'))

        cgi_params = parse_qsl(urlparse(request.Path).query)
        assert ('lang', 'ar-ru') in cgi_params
        assert ('srv', 'alice') in cgi_params
        assert ('text', REQUEST_PHRASE) in cgi_params

        response_content = {'text': [TRANSLATED_PHRASE]}

        response = THttpResponse()
        response.StatusCode = 200
        response.Content = json.dumps(response_content, ensure_ascii=False).encode('utf-8')

        ctx.add_protobuf_item(b'http_response', response)

    return handler
