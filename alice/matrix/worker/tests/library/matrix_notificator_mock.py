import tornado.gen

from .constants import ServiceHandlers

from alice.matrix.library.testing.python.http_server_base import HttpServerBase

from alice.protos.api.matrix.delivery_pb2 import TDelivery, TDeliveryResponse


class RequestsInfo:
    def __init__(self):
        self.reset()

    def reset(self):
        self.last_request = TDelivery()
        self.requests_count = 0


class MatrixNotificatorDeliveryPushHandler(tornado.web.RequestHandler):
    def initialize(self, requests_info, response_config):
        self.requests_info = requests_info
        self.response_config = response_config

    @tornado.gen.coroutine
    def post(self):
        self.requests_info.requests_count += 1
        self.requests_info.last_request.ParseFromString(self.request.body)

        if not self.request.headers.get("X-RTLog-Token"):
            self.set_status(400, "X-RTLog-Token header is empty or missed")
            yield self.finish()
        else:
            self.set_status(self.response_config["http_status_code"])

            rsp = TDeliveryResponse(
                AddPushToDatabaseStatus=TDeliveryResponse.TAddPushToDatabaseStatus(
                    Status=self.response_config["add_push_to_database_status"],
                ),
                PushId=self.requests_info.last_request.PushId,
                SubwayRequestStatus=TDeliveryResponse.TSubwayRequestStatus(
                    Status=self.response_config["subway_request_status"],
                )
            )
            yield self.finish(rsp.SerializeToString())


class MatrixNotificatorMock(HttpServerBase):
    handlers = [
        (f"/{ServiceHandlers.HTTP_DELIVERY_PUSH}", MatrixNotificatorDeliveryPushHandler),
    ]

    def __init__(self):
        super(MatrixNotificatorMock, self).__init__()

        self.requests_info = RequestsInfo()
        self.response_config = {}
        self.reset()

        self.config = {
            "requests_info": self.requests_info,
            "response_config": self.response_config,
        }

    def reset(self):
        self.requests_info.reset()
        self.set_response_config(
            http_status_code=200,
            add_push_to_database_status=TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK,
            subway_request_status=TDeliveryResponse.TSubwayRequestStatus.EStatus.OK,
        )

    def get_last_request(self):
        last_request_copy = TDelivery()
        last_request_copy.CopyFrom(self.requests_info.last_request)
        # Return copy of protobuf
        return last_request_copy

    def get_requests_count(self):
        return self.requests_info.requests_count

    def get_requests_info(self):
        return self.get_last_request(), self.get_requests_count()

    def set_response_config(self, http_status_code, add_push_to_database_status, subway_request_status):
        self.response_config.update({
            "http_status_code": http_status_code,
            "add_push_to_database_status": add_push_to_database_status,
            "subway_request_status": subway_request_status,
        })
