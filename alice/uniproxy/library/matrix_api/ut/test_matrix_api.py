import alice.uniproxy.library.testing
import tornado.gen
import tornado.web

from alice.protos.api.matrix.schedule_action_pb2 import TScheduleAction

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.matrix_api import MatrixApi
from alice.uniproxy.library.settings import config

from yatest.common.network import PortManager

_g_matrix_api_mock = None
_g_matrix_api_initialized = False
_g_matrix_api_initializing = False
_g_matrix_api_future = tornado.concurrent.Future()
_g_matrix_api_requests = {}


class MatrixApiHandler(tornado.web.RequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def post(self):
        Logger.get().info(f"handling post request {self.request.body}")

        global _g_matrix_api_requests
        _g_matrix_api_requests[self.request.path].set_result(self.request)

        self.set_status(200)
        self.set_header("Content-Type", "application/json")
        self.finish("{'result': 'Operation success'}")


class MatrixApiServerMock(object):
    def __init__(self, host, port):
        super().__init__()

        paths = [
            r"/schedule",
        ]

        global _g_matrix_api_requests
        for path in paths:
            _g_matrix_api_requests[path] = tornado.concurrent.Future()

        self._app = tornado.web.Application([(path, MatrixApiHandler) for path in paths])
        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(port)

    def start(self):
        self._srv.start(1)


@tornado.gen.coroutine
def wait_for_mock():
    Logger.init("matrix_api_test", True)
    log = Logger.get()
    global _g_matrix_api_mock, _g_matrix_api_initialized, _g_matrix_api_initializing

    if _g_matrix_api_initialized:
        return True

    if _g_matrix_api_initializing:
        yield _g_matrix_api_future
        return True

    _g_matrix_api_initializing = True

    with PortManager() as pm:
        port = pm.get_port()

        config.set_by_path("matrix.url", f"http://localhost:{port}")
        config.set_by_path("matrix.retries", 1)

        log.info(f"Starting matrix api mock server at port {port}")
        _g_matrix_api_mock = MatrixApiServerMock("localhost", port)
        _g_matrix_api_mock.start()

        yield tornado.gen.sleep(0.2)

        log.info(f"Starting matrix api mock server at port {port} done")

    _g_matrix_api_initialized = True
    _g_matrix_api_future.set_result(True)

    return True


@alice.uniproxy.library.testing.ioloop_run
def test_add_schedule_action():
    yield wait_for_mock()

    payload = {
        "schedule_action": {
            "Id": "delivery_action",
            "Puid": "339124070",
            "DeviceId": "MOCK_DEVICE_ID",
            "StartPolicy": {
                "StartAtTimestampMs": 123
            },
            "SendPolicy": {
                "SendOncePolicy": {
                    "RetryPolicy": {
                        "MaxRetries": 1,
                        "RestartPeriodScaleMs": 200,
                        "RestartPeriodBackOff": 2,
                        "MinRestartPeriodMs": 10000,
                        "MaxRestartPeriodMs": 100000
                    }
                }
            },
            "Action": {
                "OldNotificatorRequest": {
                    "Delivery": {
                        "puid": "339124070",
                        "device_id": "MOCK_DEVICE_ID",
                        "ttl": 1,
                        "semantic_frame_request_data": {
                            "typed_semantic_frame": {
                                "iot_broadcast_start": {
                                    "pairing_token": {
                                        "StringValue": "token"
                                    }
                                }
                            },
                            "analytics": {
                                "purpose": "video"
                            },
                            "origin": {
                                "device_id": "another_device_id"
                            }
                        }
                    }
                }
            }
        }
    }

    matrix = MatrixApi()
    yield matrix.add_schedule_action(payload)

    global _g_matrix_api_requests
    req = yield _g_matrix_api_requests[r"/schedule"]

    assert req.method == "POST"
    assert req.headers["Content-Type"] == "application/protobuf"

    msg = TScheduleAction()
    msg.ParseFromString(req.body)
    assert msg.Id == "delivery_action"
    assert msg.Puid == "339124070"
    assert msg.DeviceId == "MOCK_DEVICE_ID"
    assert msg.StartPolicy.StartAtTimestampMs == 123
    assert msg.SendPolicy.SendOncePolicy.RetryPolicy.MaxRetries == 1
    assert msg.SendPolicy.SendOncePolicy.RetryPolicy.RestartPeriodScaleMs == 200
    assert msg.SendPolicy.SendOncePolicy.RetryPolicy.RestartPeriodBackOff == 2
    assert msg.SendPolicy.SendOncePolicy.RetryPolicy.MinRestartPeriodMs == 10000
    assert msg.SendPolicy.SendOncePolicy.RetryPolicy.MaxRestartPeriodMs == 100000
    assert msg.Action.OldNotificatorRequest.Delivery.Puid == "339124070"
    assert msg.Action.OldNotificatorRequest.Delivery.DeviceId == "MOCK_DEVICE_ID"
    assert msg.Action.OldNotificatorRequest.Delivery.Ttl == 1
    assert msg.Action.OldNotificatorRequest.Delivery.SemanticFrameRequestData.TypedSemanticFrame.IoTBroadcastStartSemanticFrame.PairingToken.StringValue == "token"
    assert msg.Action.OldNotificatorRequest.Delivery.SemanticFrameRequestData.Analytics.Purpose == "video"
    assert msg.Action.OldNotificatorRequest.Delivery.SemanticFrameRequestData.Origin.DeviceId == "another_device_id"
