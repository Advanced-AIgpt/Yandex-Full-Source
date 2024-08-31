import time
import tornado.gen

from .constants import ServiceHandlers

from alice.matrix.library.testing.python.http_server_base import HttpServerBase

from alice.uniproxy.library.protos.uniproxy_pb2 import TSubwayMessage, TSubwayResponse


class RequestsInfo:
    def __init__(self):
        self.reset()

    def reset(self):
        self.last_request = TSubwayMessage()
        self.requests_count = 0


class SubwayRequestHandler(tornado.web.RequestHandler):
    def initialize(self, requests_info, missing_devices):
        self.requests_info = requests_info
        self.missing_devices = missing_devices

    @tornado.gen.coroutine
    def post(self):
        self.requests_info.requests_count += 1
        self.requests_info.last_request.ParseFromString(self.request.body)

        if not self.request.headers.get("X-RTLog-Token"):
            self.set_status(400, "X-RTLog-Token header is empty or missed")
            yield self.finish()
        else:
            self.set_status(200)

            rsp = TSubwayResponse()
            rsp.Timestamp = int(time.time() * 10**6)
            rsp.Status = 200
            rsp.MissingDevices[:] = self.missing_devices
            yield self.finish(rsp.SerializeToString())


class SubwayMock(HttpServerBase):
    handlers = [
        (f"/{ServiceHandlers.HTTP_SUBWAY_PUSH}", SubwayRequestHandler),
    ]

    def __init__(self):
        super(SubwayMock, self).__init__()

        self.requests_info = RequestsInfo()
        self.missing_devices = []
        self.reset()

        self.config = {
            "requests_info": self.requests_info,
            "missing_devices": self.missing_devices,
        }

    def reset(self):
        self.requests_info.reset()
        self.set_missing_devices(["abc123", "qqqq9999"])

    def get_last_request(self):
        last_request_copy = TSubwayMessage()
        last_request_copy.CopyFrom(self.requests_info.last_request)
        # Return copy of protobuf
        return last_request_copy

    def get_requests_count(self):
        return self.requests_info.requests_count

    def get_requests_info(self):
        return self.get_last_request(), self.get_requests_count()

    def set_missing_devices(self, missing_devices):
        self.missing_devices[:] = missing_devices
