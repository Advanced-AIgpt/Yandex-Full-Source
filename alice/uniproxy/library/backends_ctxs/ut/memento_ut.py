import base64
from alice.uniproxy.library.backends_ctxs.memento import Memento

from alice.uniproxy.library.auth.mocks import TVMToolClientMock, BlackboxMock
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
from alice.uniproxy.library.utils.hostname import replace_hostname
import alice.uniproxy.library.testing

from alice.memento.proto.api_pb2 import TReqGetAllObjects, TRespGetAllObjects, TUserConfigs, TReqChangeUserObjects
from alice.uniproxy.library.global_counter import GlobalCounter

import tornado.gen
import tornado.web

from yatest.common.network import PortManager


_g_memento_mock = None
_g_memento_initialized = False
_g_memento_initializing = False
_g_memento_future = tornado.concurrent.Future()


class FakeSystem:
    def __init__(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs
        self.bb = BlackboxMock()

    @tornado.gen.coroutine
    def get_bb_user_ticket(self):
        res = yield self.bb.ticket4oauth(*self.args, **self.kwargs)
        return res


def simple_memento_get_response():
    rsp = TRespGetAllObjects()
    rsp.UserConfigs.CopyFrom(TUserConfigs())  # just some default non-empty values.
    return rsp


class MementoGetApiHandler(tornado.web.RequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def return_error(self, code, text=None):
        self.set_status(code)
        self.set_header('Content-Type', 'application/json')
        if text:
            self.finish(text)
        else:
            self.finish('response body')

    def return_dump(self):
        self.set_status(200)
        self.finish(simple_memento_get_response().SerializeToString())

    def post(self):
        headers = self.request.headers

        if 'X-Ya-Service-Ticket' not in headers:
            return self.return_error(400, 'Malformed ticket')

        if 'X-RTLog-Token' not in headers:
            return self.return_error(400, 'No rtlog_token in header')

        if 'X-Ya-User-Ticket' not in headers:
            return self.return_error(403, 'Unauthorized')

        if 'valid' not in headers['X-Ya-User-Ticket']:
            return self.return_error(403, 'Unauthorized')

        request = TReqGetAllObjects()
        request.ParseFromString(self.request.body)

        return self.return_dump()


class MementoUpdateApiHandler(tornado.web.RequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def return_error(self, code, text=None):
        self.set_status(code)
        self.set_header('Content-Type', 'application/json')
        if text:
            self.finish(text)
        else:
            self.finish('response body')

    def return_dump(self):
        self.set_status(200)
        self.finish("{}")

    def post(self):
        headers = self.request.headers

        if 'X-Ya-Service-Ticket' not in headers:
            return self.return_error(400, 'Malformed ticket')

        if 'X-RTLog-Token' not in headers:
            return self.return_error(400, 'No rtlog_token in header')

        if 'X-Ya-User-Ticket' not in headers:
            return self.return_error(403, 'Unauthorized')

        if 'valid' not in headers['X-Ya-User-Ticket']:
            return self.return_error(403, 'Unauthorized')

        request = TReqChangeUserObjects()
        request.ParseFromString(self.request.body)

        if request.ScenarioData['scenario'].value != 'data'.encode('utf-8'):
            return self.return_error(500, 'unknown data')

        return self.return_dump()


class MementoServerMock(object):
    def __init__(self, host, port):
        super().__init__()
        self._app = tornado.web.Application([
            (r'/memento/get_all_objects', MementoGetApiHandler),
            (r'/memento/update_objects', MementoUpdateApiHandler),
        ])

        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(port)

    def start(self):
        self._srv.start(1)


@tornado.gen.coroutine
def wait_for_mock():
    log = Logger.get()
    global _g_memento_mock, _g_memento_initialized, _g_memento_initializing

    if _g_memento_initialized:
        return True

    if _g_memento_initializing:
        yield _g_memento_future
        return True

    _g_memento_initializing = True

    with PortManager() as pm:
        port = pm.get_port()

        url = replace_hostname(config['memento']['host'], 'localhost', port)
        config.set_by_path('memento.host', url)

        log.info('starting memento mock server at port {}'.format(port))
        _g_memento_mock = MementoServerMock('localhost', port)
        _g_memento_mock.start()

        yield tornado.gen.sleep(0.2)

        log.info('starting memento mock server at port {} done'.format(port))

    _g_memento_initialized = True
    _g_memento_future.set_result(True)

    TVMToolClientMock.mock_it()
    BlackboxMock.mock_it()
    UniproxyCounter.init()
    UniproxyTimings.init()
    return True


# ====================================================================================================================

@alice.uniproxy.library.testing.ioloop_run
def test_memento():
    yield wait_for_mock()
    args = ['valid-token', '127.0.0.1', 0]
    memento = yield Memento(FakeSystem(*args).get_bb_user_ticket).get('device_id')
    assert GlobalCounter.MEMENTO_GET_REQUEST_OK_SUMM.value() == 1
    assert memento == base64.b64encode(simple_memento_get_response().SerializeToString()).decode('utf-8')


@alice.uniproxy.library.testing.ioloop_run
def test_invalid_token():
    yield wait_for_mock()
    args = ['some-bad-token', '127.0.0.1', 0]

    try:
        ret = yield Memento(FakeSystem(*args).get_bb_user_ticket).get('device_id')
    finally:
        assert ret is None


@alice.uniproxy.library.testing.ioloop_run
def test_update_memento():
    yield wait_for_mock()
    req = TReqChangeUserObjects()

    args = ['valid-token', '127.0.0.1', 0]
    req.ScenarioData['scenario'].value = 'data'.encode('utf-8')
    memento = Memento(FakeSystem(*args).get_bb_user_ticket)
    yield memento.update(base64.b64encode(req.SerializeToString()))
    assert GlobalCounter.MEMENTO_UPDATE_REQUEST_OK_SUMM.value() == 1
