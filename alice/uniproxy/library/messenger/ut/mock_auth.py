import logging
import json
import urllib

import tornado.concurrent
import tornado.httpserver
import tornado.gen
import tornado.web

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.messenger.auth import update_global_yamb_config
from alice.uniproxy.library.messenger.auth import update_global_fanout_config
from alice.uniproxy.library.auth.mocks import BlackboxMock

from yatest.common.network import PortManager


# ====================================================================================================================
class MetaApiHandler(tornado.web.RequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def parse_request_body(self):
        pass

    def return_error(self, code, text):
        pass

    def return_guid(self, guid):
        pass

    def params_required(self):
        return True

    def post(self):
        headers = self.request.headers

        request_body = self.parse_request_body()
        if 'method' not in request_body:
            return self.return_error(400, 'no method')

        if 'params' not in request_body and self.params_required():
            return self.return_error(400, 'no params')

        request_method = request_body['method']
        request_params = request_body['params'] if self.params_required() else {}

        if request_method not in ['oauth', 'session_id']:
            return self.return_error(400, 'invalid method')

        if len(request_params) > 0:
            return self.return_error(400, 'expected empty params')

        if 'X-Request-Id' not in headers:
            return self.return_error(400, 'expected request id')

        if 'X-User_ip' not in headers:
            return self.return_error(400, 'expeceted user ip')

        if 'X-Ya-Service-Ticket' not in headers:
            return self.return_error(403, 'expeceted service ticket')

        if request_method == 'oauth':
            return self.check_oauth(headers)
        else:
            return self.check_session(headers)

        return self.return_error(500, 'unexpected behavior')

    # ----------------------------------------------------------------------------------------------------------------
    def check_oauth(self, headers):
        if 'X-Yamb-Token-Type' in headers:
            self.check_yambauth(headers)
        elif 'X-Ya-User-Ticket' in headers:
            self.check_ticket(headers)

    # ----------------------------------------------------------------------------------------------------------------
    def check_ticket(self, headers):
        user_ticket = headers.get('X-Ya-User-Ticket', '')
        is_team = True if headers.get('X-Yamb-Is-Team', '') == 'True' else False

        if 'valid' in user_ticket:
            if 'team' in user_ticket:
                if is_team:
                    return self.return_guid('team-guid')
                else:
                    return self.return_error(403, 'invalid blackbox environment')
            else:
                if is_team:
                    return self.return_error(403, 'invalid blackbox environment')
                else:
                    return self.return_guid('public-guid')
        else:
            return self.return_error(403, 'invalid token')

    # ----------------------------------------------------------------------------------------------------------------
    def check_yambauth(self, headers):
        token_type = headers.get('X-Yamb-Token-Type', '').lower()
        token_value = headers.get('X-Yamb-Token', '').lower()
        if token_type != 'yambauth':
            return self.return_error(403, 'invalid-token-type')

        if 'valid' in token_value:
            return self.return_guid('yamb-guid')
        else:
            return self.return_error(403, 'invalid-token')

    # ----------------------------------------------------------------------------------------------------------------
    def check_session(self, headers):
        cookie = headers.get('X-Yamb-Auth-Cookie')
        icookie = headers.get('X-Ya-I')

        if cookie is None and icookie is None:
            return self.return_error(400, 'expected X-Yamb-Auth-Cookie OR X-Ya-I')

        if icookie is not None and 'valid' in icookie:
            return self.return_guid('yamb-guid')

        if cookie is not None and 'valid' in cookie:
            return self.return_guid('yamb-guid')

        self.return_error(403, 'invalid-cookie')


# ====================================================================================================================
class YambMetaApiHandler(MetaApiHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    # ----------------------------------------------------------------------------------------------------------------
    def parse_request_body(self):
        request = dict(urllib.parse.parse_qsl(self.request.body.decode('utf-8')))

        if 'request' not in request:
            return self.return_error(400, 'invalid body')

        request_body = json.loads(request['request'])
        return request_body

    # ----------------------------------------------------------------------------------------------------------------
    def return_error(self, code, text):
        self.set_status(code)
        self.set_header('Content-Type', 'application/json')
        self.finish(json.dumps({
            'data': {
                'code': str(code),
                'text': text,
                'guid': 'none',
            }
        }))

    # ----------------------------------------------------------------------------------------------------------------
    def return_guid(self, guid='valid-guid'):
        self.set_status(200)
        self.finish(json.dumps({
            'data': {
                'guid': guid,
            }
        }))


# ====================================================================================================================
class FanoutMetaApiHandler(MetaApiHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    # ----------------------------------------------------------------------------------------------------------------
    def parse_request_body(self):
        request_body = json.loads(self.request.body.decode('utf-8'))
        return request_body

    # ----------------------------------------------------------------------------------------------------------------
    def params_required(self):
        return False

    # ----------------------------------------------------------------------------------------------------------------
    def return_error(self, code, text):
        self.set_status(code)
        self.set_header('Content-Type', 'application/json')
        self.finish(json.dumps({
            'data': {
                'code': str(code),
                'text': text,
                'guid': 'none',
            }
        }))

    # ----------------------------------------------------------------------------------------------------------------
    def return_guid(self, guid='valid-guid'):
        self.set_status(200)
        self.finish(json.dumps({
            'guid': guid,
        }))


# ====================================================================================================================
class AuthServerMock(object):
    def __init__(self, host, port, handler):
        super().__init__()
        self._app = tornado.web.Application([
            (r'/meta_api/', handler),
        ])

        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(port)

    def start(self):
        self._srv.start(1)


# ====================================================================================================================
class YambServerMock(AuthServerMock):
    def __init__(self, host, port):
        super().__init__(host, port, YambMetaApiHandler)


# ====================================================================================================================
class FanoutServerMock(AuthServerMock):
    def __init__(self, host, port):
        super().__init__(host, port, FanoutMetaApiHandler)


_g_yamb_mock = None
_g_yamb_initialized = False
_g_yamb_initializing = False
_g_yamb_future = tornado.concurrent.Future()

_g_fanout_mock = None
_g_fanout_initialized = False
_g_fanout_initializing = False
_g_fanout_future = tornado.concurrent.Future()


@tornado.gen.coroutine
def wait_for_yamb_mock():
    global _g_yamb_mock, _g_yamb_initialized, _g_yamb_initializing

    if _g_yamb_initialized:
        return True

    if _g_yamb_initializing:
        yield _g_yamb_future
        return True

    _g_yamb_initializing = True

    with PortManager() as pm:
        config.set_by_path('yamb.host', 'localhost')
        config.set_by_path('yamb.port', pm.get_port())
        config.set_by_path('yamb.secure', False)
        config.set_by_path('yamb.pool_size', 10)
        config.set_by_path('yamb.disable_tvm', True)

        update_global_yamb_config()

        logging.info('starting yamb mock server at port {}'.format(config['yamb']['port']))
        _g_yamb_mock = YambServerMock(config['yamb']['host'], config['yamb']['port'])
        _g_yamb_mock.start()

        yield tornado.gen.sleep(0.2)

        logging.info('starting yamb mock server at port {} done'.format(config['yamb']['port']))

    _g_yamb_initialized = True
    _g_yamb_future.set_result(True)

    BlackboxMock.make_global()
    return True


@tornado.gen.coroutine
def wait_for_fanout_mock():
    global _g_fanout_mock, _g_fanout_initialized, _g_fanout_initializing

    if _g_fanout_initialized:
        return True

    if _g_fanout_initializing:
        yield _g_fanout_future
        return True

    _g_fanout_initializing = True

    with PortManager() as pm:
        config.set_by_path('messenger.host', 'localhost')
        config.set_by_path('messenger.port', pm.get_port())
        config.set_by_path('messenger.secure', False)
        config.set_by_path('messenger.pool.size', 10)
        config.set_by_path('messenger.disable_tvm', True)

        update_global_fanout_config()

        _g_fanout_mock = FanoutServerMock(config['messenger']['host'], config['messenger']['port'])
        _g_fanout_mock.start()

    yield tornado.gen.sleep(0.2)

    _g_fanout_initialized = True

    BlackboxMock.make_global()
    return True
