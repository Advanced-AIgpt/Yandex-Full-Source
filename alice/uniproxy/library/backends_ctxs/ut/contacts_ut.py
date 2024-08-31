from alice.uniproxy.library.backends_ctxs.contacts import Contacts, get_contacts
from alice.uniproxy.library.async_http_client import HTTPError
from alice.uniproxy.library.auth.mocks import TVMToolClientMock
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter
from alice.uniproxy.library.utils.hostname import replace_hostname
import alice.uniproxy.library.testing

import json
import tornado.gen
import tornado.web

import yatest.common
from yatest.common.network import PortManager

CONTACTS_EMPTY_DUMP = yatest.common.source_path('alice/uniproxy/library/backends_ctxs/ut/contacts_list_empty.dump')
CONTACTS_DUMP = yatest.common.source_path('alice/uniproxy/library/backends_ctxs/ut/contacts_list.dump')

EMPTY_LIST = 'empty_list'
FULL_LIST = 'full_list'
TIMEOUT = 'timeout'

_g_contact_mock = None
_g_contact_initialized = False
_g_contact_initializing = False
_g_contact_future = tornado.concurrent.Future()


class ContactApiHandler(tornado.web.RequestHandler):
    list_map = {
        EMPTY_LIST: CONTACTS_EMPTY_DUMP,
        FULL_LIST: CONTACTS_DUMP,
        TIMEOUT: CONTACTS_EMPTY_DUMP,
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def parse_request_body(self):
        request_body = json.loads(self.request.body.decode('utf-8'))
        return request_body

    def return_error(self, code, text=None):
        self.set_status(code)
        self.set_header('Content-Type', 'application/json')
        if text:
            self.finish(text)
        else:
            with open(ContactApiHandler.list_map[EMPTY_LIST], 'r') as f:
                self.finish(f.read())

    @tornado.gen.coroutine
    def return_dump(self, dump_type):
        self.set_status(200)
        self.set_header('Content-Type', 'application/json')
        if dump_type == TIMEOUT:
            yield tornado.gen.sleep(config['contacts']['request_timeout']+1)
        with open(ContactApiHandler.list_map[dump_type], 'r') as f:
            self.finish(f.read())

    async def post(self):
        headers = self.request.headers

        if 'X-Device-Id' not in headers:
            return self.return_error(400, 'expeceted device id')

        if 'X-Ya-Service-Ticket' not in headers:
            return self.return_error(400, 'Malformed ticket')

        request_body = self.parse_request_body()

        if 'method' not in request_body:
            return self.return_error(400, 'invalid body')

        if 'params' not in request_body:
            return self.return_error(400, 'invalid body')

        if request_body['method'] != 'list_contacts_alice':
            return self.return_error(400, 'invalid method')

        if not isinstance(request_body['params'].get('uid'), int):
            return self.return_error(400, 'uid must be int')

        return await self.return_dump(headers['X-Device-Id'])


class ContactServerMock(object):
    def __init__(self, host, port):
        super().__init__()
        self._app = tornado.web.Application([
            (r'/meta_api/', ContactApiHandler),
        ])

        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(port)

    def start(self):
        self._srv.start(1)


@tornado.gen.coroutine
def wait_for_mock():
    log = Logger.get()
    global _g_contact_mock, _g_contact_initialized, _g_contact_initializing

    if _g_contact_initialized:
        return True

    if _g_contact_initializing:
        yield _g_contact_future
        return True

    _g_contact_initializing = True

    with PortManager() as pm:
        port = pm.get_port()
        url = replace_hostname(config['contacts']['url'], 'localhost', port)
        config.set_by_path('contacts.url', url)

        log.info('starting contact mock server at port {}'.format(port))
        _g_contact_mock = ContactServerMock('localhost', port)
        _g_contact_mock.start()

        yield tornado.gen.sleep(0.2)

        log.info('starting contact mock server at port {} done'.format(port))

    _g_contact_initialized = True
    _g_contact_future.set_result(True)

    TVMToolClientMock.mock_it()
    UniproxyCounter.init()
    return True


# ====================================================================================================================
def test_create_object():
    cs = Contacts()
    assert cs.is_undefined()


@alice.uniproxy.library.testing.ioloop_run
def test_missed_x_device_id():
    yield wait_for_mock()

    contacts = yield get_contacts(None, '42')
    assert contacts is None


@alice.uniproxy.library.testing.ioloop_run
def test_missed_uid():
    yield wait_for_mock()

    contacts = yield get_contacts('device_id', None)
    assert contacts is None


@alice.uniproxy.library.testing.ioloop_run
def test_empty_list():
    yield wait_for_mock()

    contacts = yield get_contacts(EMPTY_LIST, 42)
    assert contacts == []


@alice.uniproxy.library.testing.ioloop_run
def test_uid_is_not_int():
    yield wait_for_mock()

    contacts = yield get_contacts(FULL_LIST, '42')
    assert contacts == ["Петя Киреев", "Костя Казанова", "Сергей Бондарьков"]


@alice.uniproxy.library.testing.ioloop_run
def test_timeout():
    yield wait_for_mock()

    try:
        yield get_contacts(TIMEOUT, '42')
    except HTTPError as err:
        assert err.code == 599


@alice.uniproxy.library.testing.ioloop_run
def test_normal_list():
    yield wait_for_mock()

    contacts = yield get_contacts(FULL_LIST, 42)
    assert contacts == ["Петя Киреев", "Костя Казанова", "Сергей Бондарьков"]
