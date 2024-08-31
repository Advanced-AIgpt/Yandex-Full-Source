from alice.uniproxy.library.backends_ctxs.smart_home import get_smart_home
from alice.uniproxy.library.backends_common.protohelpers import proto_from_json

from alice.uniproxy.library.auth.mocks import TVMToolClientMock, BlackboxMock
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
from alice.uniproxy.library.utils.hostname import replace_hostname
import alice.uniproxy.library.testing

from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo

import tornado.gen
import tornado.web
import json
import base64

import yatest.common
from yatest.common.network import PortManager

SMART_HOME_EMPTY_DUMP = yatest.common.source_path('alice/uniproxy/library/backends_ctxs/ut/smart_home_empty.dump')
SMART_HOME_DUMP = yatest.common.source_path('alice/uniproxy/library/backends_ctxs/ut/smart_home.dump')

EMPTY_LIST = 'empty_list'
FULL_LIST = 'full_list'

_g_smart_home_mock = None
_g_smart_home_initialized = False
_g_smart_home_initializing = False
_g_smart_home_future = tornado.concurrent.Future()


class FakeSystem:
    def __init__(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs
        self.bb = BlackboxMock()

    @tornado.gen.coroutine
    def get_bb_user_ticket(self):
        res = yield self.bb.ticket4oauth(*self.args, **self.kwargs)
        return res


class SmartHomeApiHandler(tornado.web.RequestHandler):
    list_map = {
        EMPTY_LIST: SMART_HOME_EMPTY_DUMP,
        FULL_LIST: SMART_HOME_DUMP,
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def return_error(self, code, text=None):
        self.set_status(code)
        self.set_header('Content-Type', 'application/json')
        if text:
            self.finish(text)
        else:
            self.finish('response body')

    def return_dump(self, dump_type):
        self.set_status(200)
        self.set_header('Content-Type', 'application/protobuf')
        with open(SmartHomeApiHandler.list_map[dump_type], 'r') as f:
            resp = proto_from_json(TIoTUserInfo, json.loads(f.read()))
            self.finish(resp.SerializeToString())

    def get(self):
        headers = self.request.headers

        if 'X-Ya-Service-Ticket' not in headers:
            return self.return_error(400, 'Malformed ticket')

        if 'X-Device-Id' not in headers:
            return self.return_error(400, 'No device id in header')

        if 'X-RTLog-Token' not in headers:
            return self.return_error(400, 'No rtlog_token in header')

        if 'X-Ya-User-Ticket' not in headers:
            return self.return_error(401, 'Unauthorized')

        if 'valid' not in headers['X-Ya-User-Ticket']:
            return self.return_error(401, 'Unauthorized')

        if 'forbidden' in headers['X-Device-Id']:
            return self.return_error(403, 'forbidden')

        if 'forbidden' in headers['X-Alice4business-Uid']:
            return self.return_error(403, 'forbidden')

        if 'Accept' not in headers or \
           headers['Accept'] != 'application/protobuf':
            return self.return_error(500, 'error')

        return self.return_dump(FULL_LIST)


class SmartHomeServerMock(object):
    def __init__(self, host, port):
        super().__init__()
        self._app = tornado.web.Application([
            (r'/v1.0/user/info', SmartHomeApiHandler),
        ])

        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(port)

    def start(self):
        self._srv.start(1)


@tornado.gen.coroutine
def wait_for_mock():
    log = Logger.get()
    global _g_smart_home_mock, _g_smart_home_initialized, _g_smart_home_initializing

    if _g_smart_home_initialized:
        return True

    if _g_smart_home_initializing:
        yield _g_smart_home_future
        return True

    _g_smart_home_initializing = True

    with PortManager() as pm:
        port = pm.get_port()

        url = replace_hostname(config['smart_home']['url'], 'localhost', port)
        config.set_by_path('smart_home.url', url)

        log.info('starting smart_home mock server at port {}'.format(port))
        _g_smart_home_mock = SmartHomeServerMock('localhost', port)
        _g_smart_home_mock.start()

        yield tornado.gen.sleep(0.2)

        log.info('starting smart_home mock server at port {} done'.format(port))

    _g_smart_home_initialized = True
    _g_smart_home_future.set_result(True)

    TVMToolClientMock.mock_it()
    BlackboxMock.mock_it()
    UniproxyCounter.init()
    UniproxyTimings.init()
    return True


# ====================================================================================================================
DEVICE_ID = 'b3cbc14a-15fa-11ea-b40c-525400123456'


@alice.uniproxy.library.testing.ioloop_run
def test_smart_home():
    yield wait_for_mock()
    args = ['valid-token', '127.0.0.1', 0]
    proto_b64, devices = yield get_smart_home(FakeSystem(*args).get_bb_user_ticket, DEVICE_ID, smarthomeuid='12345')

    proto = TIoTUserInfo()
    proto.ParseFromString(base64.b64decode(proto_b64))
    assert proto.RawUserInfo == ''

    names = []
    for i in [proto.Rooms, proto.Devices, proto.Scenarios, proto.Groups, proto.Colors]:
        for k in i:
            names.append(k.Name)
    assert set(names) == set(['Вечеринка', 'Кабинет', 'Яндекс Станция домашняя'])


@alice.uniproxy.library.testing.ioloop_run
def test_invalid_token():
    yield wait_for_mock()

    args = ['some-bad-token', '127.0.0.1', 0]
    try:
        _, ret = yield get_smart_home(FakeSystem(*args).get_bb_user_ticket, DEVICE_ID)
    except Exception as e:
        assert str(e) == 'smart_home: user_ticket is None, request to IoT without user_ticket is pointless'


@alice.uniproxy.library.testing.ioloop_run
def test_forbidden():
    yield wait_for_mock()

    args = ['valid-token', '127.0.0.1', 0]
    try:
        _, ret = yield get_smart_home(FakeSystem(*args).get_bb_user_ticket, 'forbidden')
    finally:
        assert ret == {}


@alice.uniproxy.library.testing.ioloop_run
def test_alice4business_header():
    yield wait_for_mock()

    args = ['valid-token', '127.0.0.1', 0]
    try:
        _, ret = yield get_smart_home(FakeSystem(*args).get_bb_user_ticket, DEVICE_ID, smarthomeuid='forbidden')
    finally:
        assert ret == {}
