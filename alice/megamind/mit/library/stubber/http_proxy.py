import falcon
import logging
import requests
import retry
import socket
import threading
from wsgiref import simple_server

from apphost.python.client.client import Client as ApphostClient
from apphost.lib.proto_answers.http_pb2 import THttpRequest, THttpResponse, THeader


logger = logging.getLogger(__name__)


class PingHandler:
    PATH = '/ping'
    PONG = 'pong'

    def on_get(self, req, resp):
        logger.info('Ping handler is called')
        resp.status = falcon.HTTP_OK
        resp.text = PingHandler.PONG


def _wait_for_server_ready(port):
    ping_url = f'http://[::1]:{port}{PingHandler.PATH}'

    def ping():
        logger.info(f'Pinging HttpProxyService at {ping_url}')
        resp = requests.get(ping_url)
        assert resp.status_code == 200 and resp.text == PingHandler.PONG

    retry.retry_call(ping, delay=0.5, tries=60)


def _handle_exception(req, resp, exception, params):
    logger.error(f'Got exception while handling http request: {exception}')
    raise falcon.errors.HTTPInternalServerError()


def _convert_falcon_request_to_http_request(falcon_request, base_uri):
    http_request = THttpRequest()
    http_request.Method = THttpRequest.EMethod.Value(falcon_request.method.capitalize())
    http_request.Scheme = THttpRequest.EScheme.Value(falcon_request.scheme.capitalize())
    http_request.Path = falcon_request.relative_uri[len(base_uri):]
    for header_name, header_value in falcon_request.headers.items():
        http_request.Headers.append(THeader(Name=header_name, Value=header_value))
    http_request.Content = falcon_request.bounded_stream.read()
    if falcon_request.access_route:
        http_request.RemoteIP = falcon_request.access_route[0]
    return http_request


def _convert_apphost_response_to_falcon_response(http_response, falcon_response):
    falcon_response.status = falcon.util.code_to_http_status(http_response.StatusCode)
    falcon_response.data = http_response.Content
    for header in http_response.Headers:
        falcon_response.append_header(header.Name, header.Value)


class WSGIServerV6(simple_server.WSGIServer):
    address_family = socket.AddressFamily.AF_INET6


class QuietWSGIRequestHandler(simple_server.WSGIRequestHandler):
    def log_message(self, format, *args):
        pass


class HttpProxyService(object):
    def __init__(self, port, apphost_port):
        self._falcon_app = falcon.App()
        self._server = simple_server.make_server(
            '', port, self._falcon_app,
            server_class=WSGIServerV6, handler_class=QuietWSGIRequestHandler
        )
        self._thread = threading.Thread(name='HttpProxy', daemon=True, target=self._run_server)
        self._apphost_client = ApphostClient('localhost', apphost_port)

        self._falcon_app.add_error_handler(Exception, _handle_exception)
        self._falcon_app.add_route(PingHandler.PATH, PingHandler())

    def __enter__(self):
        logger.info(f'Starting HttpProxyService at port {self.port}')
        self._thread.start()
        _wait_for_server_ready(self.port)
        logger.info(f'Started HttpProxyService at port {self.port}')
        return self

    def __exit__(self, _type, _value, _traceback):
        logger.info("Stopping HttpProxyService")
        self._server.shutdown()
        self._thread.join(300)
        logger.info("Stopped HttpProxyService")

    def add_apphost_handle(self, apphost_path):
        def handler(request, response):
            logger.info(f'Handling http request for {apphost_path}')
            apphost_response = self._apphost_client.request(
                path=apphost_path,
                http_request=_convert_falcon_request_to_http_request(request, apphost_path)
            )
            http_response = apphost_response.get_only_type_item('http_response', THttpResponse)
            logger.debug(f'Received http response from apphost: {http_response}')
            _convert_apphost_response_to_falcon_response(http_response, response)
            logger.info(f'Handled http request for {apphost_path}')

        self._falcon_app.add_sink(handler, prefix=apphost_path)

    @property
    def port(self):
        return self._server.server_address[1]

    def _run_server(self):
        self._server.serve_forever()
