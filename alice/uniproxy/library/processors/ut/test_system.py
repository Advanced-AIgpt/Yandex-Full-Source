import logging
import tornado
from rtlog import null_logger

import alice.uniproxy.library.personal_data as pdata
from alice.uniproxy.library.processors import create_event_processor
import alice.uniproxy.library.processors.system as system
from alice.uniproxy.library.processors.system import SynchronizeState
import alice.uniproxy.library.testing
import common
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.auth.mocks import BlackboxMock
from alice.uniproxy.library.async_http_client import HTTPResponse, HTTPError


logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)12s: %(name)-16s %(message)s')


def test_create_system_processors():
    proc = create_event_processor(common.FakeSystem(), common.FakeEvent("System", "SynchronizeState"))
    assert proc.event_type == "system.synchronizestate"
    assert isinstance(proc, SynchronizeState)

    proc = create_event_processor(common.FakeSystem(), common.FakeEvent("System", "UserInactivityReport"))
    assert proc.event_type == "system.userinactivityreport"
    assert isinstance(proc, system.UserInactivityReport)

    proc = create_event_processor(common.FakeSystem(), common.FakeEvent("System", "ResetUserInactivity"))
    assert proc.event_type == "system.resetuserinactivity"
    assert isinstance(proc, system.ResetUserInactivity)

    proc = create_event_processor(common.FakeSystem(), common.FakeEvent("System", "ExceptionEncountered"))
    assert proc.event_type == "system.exceptionencountered"
    assert isinstance(proc, system.ExceptionEncountered)

    proc = create_event_processor(common.FakeSystem(), common.FakeEvent("System", "EchoRequest"))
    assert proc.event_type == "system.echorequest"
    assert isinstance(proc, system.EchoRequest)


def test_device_model():
    event = common.FakeEvent("System", "SynchronizeState")
    system = common.FakeSystem()
    proc = create_event_processor(system, event)
    event.payload = {
        'vins': {
            'application': {
                'device_model': 'Station',
            },
        },
    }

    proc.vins_app_data(event)
    assert system.device_model == 'station'

    proc.payload_with_session_data['request'] = {'experiments': {'use_yandexstation_instead_of_station': '1'}}
    proc.vins_app_data(event)
    assert system.device_model == 'yandexstation'


class FakeSystem:
    def __init__(self, uid='100500', error=False):
        self.uid = uid
        self.puid = uid
        self.do_not_use_user_logs = None
        self.bb_uid4oauth_error = error
        self.session_id = 1
        self.client_ip = 'ip'
        self.client_port = 0
        self.logger = common.FakeLogger()
        self.suspended = 0
        self.suspend_future = None
        self.auth_token = None
        self.use_balancing_hint = True
        self.use_spotter_glue = True
        self.use_laas = True
        self.use_datasync = True
        self.use_personal_cards = True

    def suspend_message_processing(self, inc, *args):
        if inc:
            self.suspended += 1
        else:
            self.suspended -= 1
        return

    @tornado.gen.coroutine
    def set_oauth_token(self, *args):
        yield tornado.gen.sleep(0)

    def get_oauth_token(self):
        return 'valid'

    def vins_app_data(self, *args):
        return

    def sync_state_complete(self, *args):
        return

    def on_close_event_processor(self, *args, **kwargs):
        pass

    def wait_initialization(self, message_id=None):
        pass


class FakeAsyncHttpClient:
    def __init__(self, error, resp):
        self.error = error
        self.resp = bytes('{"items": [{"body": "{\\"items\\": [' + resp + ']}"}]}', encoding='utf-8')

    @tornado.gen.coroutine
    def fetch(self, *args, **kwargs):
        yield tornado.gen.sleep(0.0)
        if self.error:
            raise HTTPError(code=500, body=self.resp)
        return HTTPResponse(200, self.resp)


@tornado.gen.coroutine
def helper(monkeypatch, uid, bb_error, ds_error, ds_response):
    test_system = FakeSystem(uid, bb_error)
    proc = SynchronizeState(system=test_system, rt_log=null_logger(), init_message_id=None)
    proc.event = common.FakeEvent('namespace', 'name', 'message_id')
    BlackboxMock.mock_it()

    def create_fake_clients(self):
        self.bb_client = BlackboxMock()

    def _mock_get_client(url):
        return FakeAsyncHttpClient(ds_error, ds_response)

    monkeypatch.setattr(pdata.PersonalDataHelper, 'create_clients', create_fake_clients)
    monkeypatch.setattr(pdata, '_get_client', _mock_get_client)

    def get_fake_headers(self):
        return []

    monkeypatch.setattr(pdata.PersonalDataHelper, '_get_common_headers', get_fake_headers)
    yield proc.try_get_personal_settings()
    return test_system.do_not_use_user_logs


def add_monkeypatch(foo):
    def bar(monkeypatch):
        return foo(monkeypatch)
    return bar


@add_monkeypatch
@alice.uniproxy.library.testing.ioloop_run
def test_simple(monkeypatch):
    Logger.init("test_system", is_debug=True)

    # uid, no errors
    assert not (yield helper(monkeypatch, '100500', False, False, '{}'))

    assert not (yield helper(monkeypatch, '100500', False, False, '{\\"address_id\\": \\"some_id\\", \\"do_not_use_user_logs\\": false}'))

    assert (yield helper(monkeypatch, '100500', False, False, '{\\"address_id\\": \\"some_id\\", \\"do_not_use_user_logs\\": true}'))

    # no uid, no errors
    assert not (yield helper(monkeypatch, None, False, False, '{}'))

    assert not (yield helper(monkeypatch, None, False, False, '{\\"some_id\\": {\\"do_not_use_user_logs\\": false}}'))

    assert not (yield helper(monkeypatch, None, False, False, '{\\"some_id\\": {\\"do_not_use_user_logs\\": true}}'))

    # no uid + BB error
    assert (yield helper(monkeypatch, None, True, False, '{}'))

    assert (yield helper(monkeypatch, None, True, False, '{\\"some_id\\": {\\"do_not_use_user_logs\\": false}}'))

    assert (yield helper(monkeypatch, None, True, False, '{\\"some_id\\": {\\"do_not_use_user_logs\\": true}}'))

    # uid + data_sync error
    assert (yield helper(monkeypatch, '100500', False, True, '{}'))


@alice.uniproxy.library.testing.ioloop_run
def test_check_mssngr_suspended():
    system = FakeSystem('42', False)

    def check(*args):
        pass

    system.update_messenger_guid = check
    assert system.suspended == 0

    proc = SynchronizeState(system=system, rt_log=null_logger(), init_message_id=None)
    event = common.FakeEvent("System", "SynchronizeState", payload={'Messenger': {}})
    proc.event = event

    assert system.suspended == 0

    system.suspend_message_processing(True, "SynchronizeState")
    BlackboxMock.mock_it()
    yield proc.process_event_coro(event, 'valid', '42')

    assert system.suspended == 0
