import alice.uniproxy.library.testing
import tornado.gen
import tornado.web
from tornado.ioloop import IOLoop
from tornado.concurrent import Future

import json

from alice.uniproxy.library.personal_data import PersonalDataHelper, DATASYNC_ADDRESSES_PREFIX, DATASYNC_KV_PREFIX, DATASYNC_SETTINGS_PREFIX
from alice.uniproxy.library.auth.mocks import TVMToolClientMock, BlackboxMock
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter
from alice.uniproxy.library.utils.hostname import replace_hostname
from alice.uniproxy.library.responses_storage import ResponsesStorage

from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadResponse
from apphost.lib.proto_answers.http_pb2 import THttpResponse

from rtlog import null_logger

from yatest.common.network import PortManager

_g_personal_data_mock = None
_g_personal_data_initialized = False
_g_personal_data_initializing = False
_g_personal_data_future = tornado.concurrent.Future()

SETTINGS_ID = 'id'


class PersonalDataApiHandler(tornado.web.RequestHandler):

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
        self.set_header('Content-Type', 'application/json')
        self.finish(dump_type)

    def post(self):
        # headers = self.request.headers
        Logger.get().warn('send response')

        body = json.loads(self.request.body)
        resp = {
            'items': [
                {'body': '{"items": [{"id": "' + SETTINGS_ID + '", "' + DATASYNC_SETTINGS_PREFIX + '": "value"}]}'} for i in range(len(body['items']))
            ],
        }
        return self.return_dump(json.dumps(resp))


class PersonalDataServerMock(object):
    def __init__(self, host, port):
        super().__init__()
        self._app = tornado.web.Application([
            (r'/v1/batch/request', PersonalDataApiHandler),
        ])

        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(port)

    def start(self):
        self._srv.start(1)


@tornado.gen.coroutine
def wait_for_mock():
    Logger.init('personal_data_tests', True)
    log = Logger.get()
    global _g_personal_data_mock, _g_personal_data_initialized, _g_personal_data_initializing

    if _g_personal_data_initialized:
        return True

    if _g_personal_data_initializing:
        yield _g_personal_data_future
        return True

    _g_personal_data_initializing = True

    with PortManager() as pm:
        port = pm.get_port()

        url = replace_hostname(config['data_sync']['url'], 'localhost', port)
        config.set_by_path('data_sync.url', url)

        log.info('starting personal_data mock server at port {}'.format(port))
        _g_personal_data_mock = PersonalDataServerMock('localhost', port)
        _g_personal_data_mock.start()

        yield tornado.gen.sleep(0.2)

        log.info('starting personal_data mock server at port {} done'.format(port))

    _g_personal_data_initialized = True
    _g_personal_data_future.set_result(True)

    TVMToolClientMock.mock_it()
    BlackboxMock.mock_it()
    UniproxyCounter.init()
    return True


class FakeUnisystem:
    def __init__(self, oauth):
        self.oauth_token = oauth
        self.session_id = 'session-id'
        self.client_ip = '127.0.0.1'
        self.client_port = 80
        self.responses_storage = ResponsesStorage()

    def get_oauth_token(self):
        return self.oauth_token


# -------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_personal_data_settings():
    yield wait_for_mock()
    system = FakeUnisystem('valid')
    req_info = {}
    pdata = PersonalDataHelper(system, req_info, null_logger())

    resp, _ = yield pdata.get_personal_data(only_settings=True)
    assert resp[SETTINGS_ID][DATASYNC_SETTINGS_PREFIX] == 'value'


# -------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_personal_data_store_responses():
    yield wait_for_mock()
    system = FakeUnisystem('valid')
    req_info = {
        'request': {
            'experiments': {
                'context_load_diff': '1',
            },
        },
        'header': {
            'request_id': 'my-lovely-request-id',
        },
        'uuid': 'my-lovely-uuid',
    }
    pdata = PersonalDataHelper(system, req_info, null_logger())

    resp, _ = yield pdata.get_personal_data()

    # check responses_storage
    storage = pdata.responses_storage.load(reqid='my-lovely-request-id')
    assert set(storage) == {'datasync_uuid', 'datasync', 'datasync_device_id'}


# -------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_personal_data_from_contextload_future():
    context_load_response = TContextLoadResponse(
        DatasyncResponse=THttpResponse(
            StatusCode=200,
            Content=json.dumps({
                "items": [
                    {
                        "body": json.dumps({
                            "items": [
                                {"address_id": "home", "value": "my home address"},
                                {"address_id": "work", "value": "my work address"}
                            ]
                        })
                    },
                    {
                        "body": json.dumps({
                            "items": [
                                {"id": "X", "value": "my X value"},
                                {"id": "Y", "value": "my Y value"}
                            ]
                        })
                    }
                ]
            }).encode("utf-8")
        ),
        DatasyncDeviceIdResponse=THttpResponse(
            StatusCode=404,
            Content=b"Not Found"
        ),
        DatasyncUuidResponse=THttpResponse(
            StatusCode=200,
            Content=json.dumps({
                "items": [
                    {
                        "body": json.dumps({
                            "items": [
                                {"address_id": "home", "value": "my another home address"},
                            ]
                        })
                    },
                    {
                        "body": json.dumps({
                            "items": [
                                {"id": "Z", "value": "my Z value"}
                            ]
                        })
                    }
                ]
            }).encode("utf-8")
        )
    )

    system = FakeUnisystem('valid')
    req_info = {
        'request': {
            'experiments': {
                'context_load_diff': '1',
            },
        },
        'header': {
            'request_id': 'my-lovely-request-id',
        },
        'uuid': 'my-lovely-uuid',
    }

    fut = Future()
    pdata = PersonalDataHelper(system, req_info, null_logger(), personal_data_response_future=fut)

    IOLoop.current().call_later(0.5, fut.set_result, context_load_response)
    resp, _ = yield pdata.get_personal_data()

    assert resp == {
        f"{DATASYNC_ADDRESSES_PREFIX}/home": {"address_id": "home", "value": "my home address"},
        f"{DATASYNC_ADDRESSES_PREFIX}/work": {"address_id": "work", "value": "my work address"},
        f"{DATASYNC_KV_PREFIX}/X": "my X value",
        f"{DATASYNC_KV_PREFIX}/Y": "my Y value",
        f"{DATASYNC_KV_PREFIX}/Z": "my Z value"
    }
