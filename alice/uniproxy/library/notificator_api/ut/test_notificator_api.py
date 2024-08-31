import alice.uniproxy.library.testing
import tornado.gen
import tornado.web

from alice.protos.api.matrix.delivery_pb2 import TDelivery
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.notificator_api import NotificatorApi
from alice.uniproxy.library.settings import config

from yatest.common.network import PortManager

_g_notificator_api_mock = None
_g_notificator_api_initialized = False
_g_notificator_api_initializing = False
_g_notificator_api_future = tornado.concurrent.Future()
_g_notificator_api_requests = {}


class NotificatorApiHandler(tornado.web.RequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def post(self):
        Logger.get().info(f'handling post request {self.request.body}')

        global _g_notificator_api_requests
        _g_notificator_api_requests[self.request.path].set_result(self.request)

        self.set_status(200)
        self.set_header('Content-Type', 'application/json')
        self.finish('{"result": "Operation success"}')


class NotificatorApiServerMock(object):
    def __init__(self, host, port):
        super().__init__()

        paths = [
            r'/delivery/push',
        ]

        global _g_notificator_api_requests
        for path in paths:
            _g_notificator_api_requests[path] = tornado.concurrent.Future()

        self._app = tornado.web.Application([(path, NotificatorApiHandler) for path in paths])
        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(port)

    def start(self):
        self._srv.start(1)


@tornado.gen.coroutine
def wait_for_mock():
    Logger.init('notificator_api_test', True)
    log = Logger.get()
    global _g_notificator_api_mock, _g_notificator_api_initialized, _g_notificator_api_initializing

    if _g_notificator_api_initialized:
        return True

    if _g_notificator_api_initializing:
        yield _g_notificator_api_future
        return True

    _g_notificator_api_initializing = True

    with PortManager() as pm:
        port = pm.get_port()

        config.set_by_path('notificator.uniproxy.url', f'http://localhost:{port}')
        config.set_by_path('notificator.uniproxy.retries', 1)

        log.info(f'Starting notificator api mock server at port {port}')
        _g_notificator_api_mock = NotificatorApiServerMock('localhost', port)
        _g_notificator_api_mock.start()

        yield tornado.gen.sleep(0.2)

        log.info(f'Starting notificator api mock server at port {port} done')

    _g_notificator_api_initialized = True
    _g_notificator_api_future.set_result(True)

    return True


@alice.uniproxy.library.testing.ioloop_run
def test_send_typed_semantic_frame():
    yield wait_for_mock()

    payload = {
        'puid': '13071999',
        'device_id': 'MEGADEVICE_GOBLIN_3000',
        'ttl': 228,
        'semantic_frame_request_data': {
            'typed_semantic_frame': {
                'weather_semantic_frame': {
                    'when': {
                        'datetime_value': '13:07:1999',
                    },
                },
            },
            'analytics': {
                'product_scenario': 'Weather',
                'origin': 'Scenario',
            },
            'origin': {
                'device_id': 'another_device_id',
                'uuid': 'another_uuid',
            },
        },
    }

    notificator = NotificatorApi('token', 'ip')
    yield notificator.push_typed_semantic_frame(payload)

    global _g_notificator_api_requests
    req = yield _g_notificator_api_requests[r'/delivery/push']

    assert req.method == 'POST'
    assert req.headers['Content-Type'] == 'application/protobuf'

    msg = TDelivery()
    msg.ParseFromString(req.body)
    assert msg.Puid == '13071999'
    assert msg.DeviceId == 'MEGADEVICE_GOBLIN_3000'
    assert msg.Ttl == 228
    assert msg.SemanticFrameRequestData.TypedSemanticFrame.WeatherSemanticFrame.When.DateTimeValue == '13:07:1999'
    assert msg.SemanticFrameRequestData.Analytics.ProductScenario == 'Weather'
    assert msg.SemanticFrameRequestData.Analytics.Origin == 2  # code for "Scenario"
    assert msg.SemanticFrameRequestData.Origin.DeviceId == 'another_device_id'
    assert msg.SemanticFrameRequestData.Origin.Uuid == 'another_uuid'
