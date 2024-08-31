import tornado.gen

from .constants import ServiceHandlers

from alice.matrix.library.testing.python.http_server_base import HttpServerBase

from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from alice.protos.data.device.info_pb2 import EUserDeviceType
from alice.protos.data.location.room_pb2 import TUserRoom


class RequestsInfo:
    def __init__(self):
        self.reset()

    def reset(self):
        self.last_request_service_ticket = ""
        self.last_request_user_ticket = ""
        self.requests_count = 0


class IoTUserInfoRequestHandler(tornado.web.RequestHandler):
    needed_headers = [
        "Accept",
        "Host",

        "X-RTLog-Token",
        "X-Ya-Service-Ticket",
        "X-Ya-User-Ticket",
    ]

    expected_accept_header_value = "application/protobuf"
    expected_host_header_value = "localhost"

    def initialize(self, requests_info, devices, rooms):
        self.requests_info = requests_info
        self.devices = devices
        self.rooms = rooms

    @tornado.gen.coroutine
    def get(self):
        self.requests_info.requests_count += 1
        self.requests_info.last_request_service_ticket = self.request.headers.get("X-Ya-Service-Ticket")
        self.requests_info.last_request_user_ticket = self.request.headers.get("X-Ya-User-Ticket")

        for header in self.needed_headers:
            if not self.request.headers.get(header):
                self.set_status(400, f"{header} header is empty or missed")
                yield self.finish()
                return

        if self.request.headers.get("Accept") != self.expected_accept_header_value:
            self.set_status(400, f"Accept header value is {self.request.headers.get('Accept')}, but it must be {self.expected_accept_header_value}")
            yield self.finish()
            return

        if self.request.headers.get("Host") != self.expected_host_header_value:
            self.set_status(400, f"Host header value is {self.request.headers.get('Host')}, but it must be {self.expected_host_header_value}")
            yield self.finish()
            return

        self.set_status(200)

        rsp = TIoTUserInfo(
            Devices=[
                TIoTUserInfo.TDevice(
                    Id=device["id"],
                    Type=device["type"],
                    Name=device["name"],
                    RoomId=device["room_id"],
                    AnalyticsType=device["analytics_type"],

                    QuasarInfo=TIoTUserInfo.TDevice.TQuasarInfo(
                        DeviceId=device["quasar_id"],
                        Platform=device["quasar_platform"],
                    ),
                )
                for device in self.devices
            ],
            Rooms=[
                TUserRoom(
                    Id=room["id"],
                    Name=room["name"],
                    HouseholdId=room["household_id"],
                )
                for room in self.rooms
            ],
        )
        yield self.finish(rsp.SerializeToString())


class IoTMock(HttpServerBase):
    handlers = [
        (f"/{ServiceHandlers.HTTP_IOT_USER_INFO}", IoTUserInfoRequestHandler),
    ]

    def __init__(self):
        super(IoTMock, self).__init__()

        self.requests_info = RequestsInfo()
        self.devices = []
        self.rooms = []
        self.reset()

        self.config = {
            "requests_info": self.requests_info,
            "devices": self.devices,
            "rooms": self.rooms,
        }

    def reset(
        self,
        device_id_base="device_id",
    ):
        self.requests_info.reset()
        self.devices[:] = [
            {
                "id": "system_id1",
                "name": "First device",
                "type": EUserDeviceType.YandexStationDeviceType,
                "room_id": "system_room_id",
                "analytics_type": "devices.types.smart_speaker.yandex.station.station",

                "quasar_id": f"{device_id_base}_1",
                "quasar_platform": "yandexstation",
            },
            {
                "id": "system_id2_not_alice_device",
                "name": "Second device",
                "type": EUserDeviceType.PurifierDeviceType,
                "room_id": "",
                "analytics_type": "trash",

                "quasar_id": f"{device_id_base}_2_not_alice_device",
                "quasar_platform": "magic",
            },
            {
                "id": "system_id3",
                "name": "Third device",
                "type": EUserDeviceType.YandexStationMiniDeviceType,
                "room_id": "",
                "analytics_type": "devices.types.smart_speaker.yandex.station.mini",

                "quasar_id": f"{device_id_base}_3",
                "quasar_platform": "yandexmidi",
            },

        ]
        self.rooms[:] = [
            {
                "id": "system_room_id1",
                "name": "First room",
                "household_id": "household_id1",
            },
            {
                "id": "system_room_id2",
                "name": "Second room",
                "household_id": "household_id2",
            },
        ]

    def get_last_service_ticket(self):
        return self.requests_info.last_request_service_ticket

    def get_last_user_ticket(self):
        return self.requests_info.last_request_user_ticket

    def get_requests_count(self):
        return self.requests_info.requests_count

    def get_requests_info(self):
        return self.get_last_service_ticket(), self.get_last_user_ticket(), self.get_requests_count()

    def set_devices(self, devices):
        self.devices[:] = devices

    def set_rooms(self, rooms):
        self.rooms[:] = rooms
