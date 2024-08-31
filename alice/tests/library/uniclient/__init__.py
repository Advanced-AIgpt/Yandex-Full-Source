import base64

import attr
import alice.library.client.protos.client_info_pb2 as client_into_pb2
import alice.megamind.protos.common.device_state_pb2 as device_state_pb2
import alice.megamind.protos.common.environment_state_pb2 as environment_state_pb2
import alice.megamind.protos.common.iot_pb2 as iot_pb2
import alice.megamind.protos.common.location_pb2 as location_pb2
import alice.megamind.protos.speechkit.request_pb2 as request_pb2

from . import event
from .proto_wrapper import ProtoWrapper
from .scraper_uniclient import ScraperUniclient
from .uniclient import Uniclient

__all__ = [
    'event',
    'ScraperUniclient',
    'Uniclient',
]


@attr.s(slots=True)
class AliceSettings(object):
    uniproxy_url = attr.ib(default=None)
    vins_url = attr.ib(default=None)
    log_type = attr.ib(default='null')
    auth_token = attr.ib(default=None)
    asr_topic = attr.ib(default=None)
    user_agent = attr.ib(default=None)
    application = attr.ib(factory=dict)
    oauth_token = attr.ib(default=None)
    experiments = attr.ib(factory=dict)
    environment_state = attr.ib(default=None)
    region = attr.ib(default=None)
    iot = attr.ib(default=None)
    supported_features = attr.ib(factory=dict)
    unsupported_features = attr.ib(factory=dict)
    permissions = attr.ib(factory=list)
    predefined_contacts = attr.ib(default=None)
    sync_state_supported_features = attr.ib(default=None)
    radio_stations = attr.ib(default=None)
    scraper_mode = attr.ib(default=False)


class AdditionalOptions(ProtoWrapper):
    proto_cls = request_pb2.TSpeechKitRequestProto.TRequest.TAdditionalOptions


class SpeechKitHeader(ProtoWrapper):
    proto_cls = request_pb2.TSpeechKitRequestProto.THeader


class Application(ProtoWrapper):
    proto_cls = client_into_pb2.TClientInfoProto


class IoTUserInfo(ProtoWrapper):
    proto_cls = iot_pb2.TIoTUserInfo

    def dict(self):
        return base64.b64encode(self._o.SerializeToString()).decode('utf-8')


class EnvironmentState(ProtoWrapper):
    proto_cls = environment_state_pb2.TEnvironmentState


class DeviceState(ProtoWrapper):
    proto_cls = device_state_pb2.TDeviceState


class Location(ProtoWrapper):
    proto_cls = location_pb2.TLocation
