import json
import time
import tornado.ioloop
import tornado.gen
import tornado.httpserver
import tornado.web
import alice.uniproxy.library.testing
from alice.uniproxy.library.auth import tvm2
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger
from urllib.parse import parse_qs


class Tvm2RequestHandler(tornado.web.RequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._log = Logger.get('unit_test')

    def get(self):
        self._log.debug('GET')
        parse_qs
        self.set_status(200)
        self.write(json.dumps({}))

    def post(self):
        request = parse_qs(self.request.body.decode('utf-8'))
        self._log.debug('POST: {}'.format(request))
        self.set_status(200)
        # return fake ticket
        self.write(json.dumps({
            request['dst'][0]: {
                'ticket': 'mock-ticket'
            }
        }))


class Tvm2ServerMock(object):
    def __init__(self, host, port, handler):
        super().__init__()
        self.port = port
        self._app = tornado.web.Application([
            (r'/.*', handler),
        ])

        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.listen(port)

    def stop(self):
        self._srv.stop()


@alice.uniproxy.library.testing.ioloop_run
def _test_mocked_tvm2_server(monkeypatch):
    Logger.init('unitest', True)
    logger = Logger.get('unit_test')
    tvm2_server_mock = None
    try:
        if config.get('qloud_tvm_token'):
            # for QLOUD env with hardcoded tvm url use cache mocking
            monkeypatch.setattr(tvm2, 'TICKETS_CACHE', {
                config['music']['service_id']: {
                    'ts': int(time.time()),
                    'ticket': 'mock-ticket',
                }
            })
        else:
            # for direct usage tvm servers
            for port in (9877, 9878, 10018, 11111, 11112):
                try:
                    tvm2_server_mock = Tvm2ServerMock('localhost', port, Tvm2RequestHandler)
                    logger.debug('Tvm2ServerMock bind port {}'.format(port))
                    break
                except Exception as exc:
                    logger.warning('can not bind port {} for Tvm2ServerMock: {}'.format(port, str(exc)))

            if not tvm2_server_mock:
                raise Exception('can not run Tvm2ServerMock')

            monkeypatch.setattr(tvm2, 'TVM_API_URL', 'http://localhost:{}'.format(tvm2_server_mock.port))
            monkeypatch.setattr(tvm2, 'TVM_API_HOST', 'localhost')
            monkeypatch.setattr(tvm2, 'TVM_API_PORT', tvm2_server_mock.port)
            monkeypatch.setattr(tvm2, 'TVM_API_SECURE', False)

        ticket = yield tvm2.service_ticket_for_music()
        logger.debug('got tiket={}'.format(ticket))
        assert ticket == 'mock-ticket'
    finally:
        if tvm2_server_mock:
            tvm2_server_mock.stop()


def test_mocked_tvm2_server(monkeypatch):
    _test_mocked_tvm2_server(monkeypatch)
