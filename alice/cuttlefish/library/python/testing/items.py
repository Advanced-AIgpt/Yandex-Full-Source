from uuid import uuid4
import json
from .constants import ItemTypes
from .utils import extract_protobuf, pack_json, pack_protobuf
from apphost.lib.grpc.protos.service_pb2 import TServiceRequest
from alice.cuttlefish.library.protos.asr_pb2 import TSpotterValidation
from alice.cuttlefish.library.protos.antirobot_pb2 import TAntirobotInputSettings, TAntirobotInputData
from alice.cuttlefish.library.protos.events_pb2 import (
    TEvent,
    TDirective,
    TApplicationInfo,
    TSynchronizeStateEvent,
    TEventException,
)
from alice.cuttlefish.library.protos.megamind_pb2 import TMegamindRequest, TMegamindResponse
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TUserInfo, TDeviceInfo
from alice.cuttlefish.library.protos.yabio_pb2 import TResponse as TYabioResponse
from alice.cuttlefish.library.protos.wsevent_pb2 import TWsEvent, TEventHeader
from voicetech.asr.engine.proto_api.response_pb2 import TASRResponse
from voicetech.library.proto_api.yabio_pb2 import YabioContext
from apphost.lib.proto_answers.http_pb2 import THttpRequest, THttpResponse


def _or(arg, default):
    return arg if (arg is not None) else default


NAMESPACE_DICT = {
    "system": TEventHeader.EMessageNamespace.SYSTEM,
    "log": TEventHeader.EMessageNamespace.LOG,
    "vins": TEventHeader.EMessageNamespace.VINS,
}


NAME_DICT = {
    "synchronizestate": TEventHeader.EMessageName.SYNCHRONIZE_STATE,
    "eventexception": TEventHeader.EMessageName.EVENT_EXCEPTION,
    "spotter": TEventHeader.EMessageName.NM_SPOTTER,
    "textinput": TEventHeader.EMessageName.TEXT_INPUT,
    "voiceinput": TEventHeader.EMessageName.VOICE_INPUT,
    "musicinput": TEventHeader.EMessageName.MUSIC_INPUT,
}


TYPES_TO_PROTOBUF = {
    ItemTypes.ASR_PROTO_RESPONSE: TASRResponse,
    ItemTypes.ASR_SPOTTER_VALIDATION: TSpotterValidation,
    ItemTypes.SESSION_CONTEXT: TSessionContext,
    ItemTypes.SYNCRHONIZE_STATE_EVENT: TEvent,
    ItemTypes.EVENT_EXCEPTION: TDirective,
    ItemTypes.DIRECTIVE: TDirective,
    ItemTypes.WS_MESSAGE: TWsEvent,
    ItemTypes.MEGAMIND_REQUEST: TMegamindRequest,
    ItemTypes.MEGAMIND_RESPONSE: TMegamindResponse,
    ItemTypes.APIKEYS_HTTP_REQUEST: THttpRequest,
    ItemTypes.BLACKBOX_HTTP_REQUEST: THttpRequest,
    ItemTypes.DATASYNC_HTTP_REQUEST: THttpRequest,
    ItemTypes.LAAS_HTTP_REQUEST: THttpRequest,
    ItemTypes.HTTP_REQUEST_DRAFT: THttpRequest,
    ItemTypes.TVMTOOL_HTTP_REQUEST: THttpRequest,
    ItemTypes.YABIO_CONTEXT: YabioContext,
    ItemTypes.YABIO_PROTO_RESPONSE: TYabioResponse,
    ItemTypes.ANTIROBOT_INPUT_DATA: TAntirobotInputData,
    ItemTypes.ANTIROBOT_INPUT_SETTINGS: TAntirobotInputSettings,
    ItemTypes.ANTIROBOT_HTTP_REQUEST: THttpRequest,
}


# -------------------------------------------------------------------------------------------------
def add_grpc_request_items(request, items=[], codec_id=0):
    for i in items:
        answer = request.Answers.add()
        answer.SourceName = i.get("source_name", "INIT")
        answer.Type = i.get("type", "some_type")
        data = i["data"]
        if isinstance(data, dict):
            answer.Data = pack_json(data, codec_id)
        else:
            answer.Data = pack_protobuf(data, codec_id)


def create_grpc_request(items=[], codec_id=0, **kwargs):
    request = TServiceRequest(**kwargs)
    add_grpc_request_items(request, items=items, codec_id=codec_id)
    return request


def parse_response_answers(response):
    res = {}  # type -> data
    for item in response.Answers:
        protobuf_type = TYPES_TO_PROTOBUF.get(item.Type)
        if protobuf_type is not None:
            res[item.Type] = extract_protobuf(item.Data, protobuf_type)
        else:
            res[item.Type] = item.Data
    return res


def get_answer_items(response, item_type, source_name=None, proto=None):
    for item in response.Answers:
        if item.Type != item_type:
            continue
        if (source_name is not None) and (item.SourceName != source_name):
            continue
        protobuf_type = proto or TYPES_TO_PROTOBUF.get(item_type)
        if protobuf_type is None:
            yield item.Data
        else:
            yield extract_protobuf(item.Data, protobuf_type)


def get_answer_item(response, item_type, source_name=None, proto=None, noexcept=False):
    for item in get_answer_items(response=response, item_type=item_type, source_name=source_name, proto=proto):
        return item
    if not noexcept:
        raise KeyError(f"No {source_name or ''}@{item_type} item in answers")


def get_only_answer_item(response, item_type, source_name=None):
    res = None
    for item in response.Answers:
        if item.Type != item_type:
            continue
        if (source_name is not None) and (item.SourceName != source_name):
            continue
        if res is not None:
            raise RuntimeError(f"More than 1 {source_name or ''}@{item_type} item in answers")
        protobuf_type = TYPES_TO_PROTOBUF.get(item_type)
        if protobuf_type is None:
            res = item.Data
        res = extract_protobuf(item.Data, protobuf_type)
    if res is None:
        raise RuntimeError(f"No {source_name or ''}@{item_type} item in answers")
    return res


# -------------------------------------------------------------------------------------------------
def create_message_header(header):
    return TEventHeader(
        Namespace=NAMESPACE_DICT.get(header["namespace"].lower(), TEventHeader.EMessageNamespace.UNKNOWN_NAMESPACE),
        Name=NAME_DICT.get(header["name"].lower(), TEventHeader.EMessageName.UNKNOWN_NAME),
        MessageId=header["messageId"],
    )


# -------------------------------------------------------------------------------------------------
def create_synchronize_state_event(message_id=None, app_id=None, app_token=None, **kwargs):
    return TEvent(
        Header=TEventHeader(
            Namespace=TEventHeader.EMessageNamespace.SYSTEM,
            Name=TEventHeader.EMessageName.SYNCHRONIZE_STATE,
            MessageId=_or(message_id, str(uuid4())),
        ),
        SyncState=TSynchronizeStateEvent(
            AppToken=_or(app_token, "some-valid-app-token"),
            ApplicationInfo=TApplicationInfo(Id=_or(app_id, "some.application.id")),
            UserInfo=TUserInfo(
                VinsApplicationUuid=kwargs.get("vins_app_uuid"),
                Uuid=kwargs.get("uuid", "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"),
                Yuid=kwargs.get("yandex_uid"),
                AuthToken=kwargs.get("auth_token"),
                AuthTokenType=_or(kwargs.get("auth_token_type"), TUserInfo.ETokenType.OAUTH),
            ),
            DeviceInfo=TDeviceInfo(
                DeviceManufacturer=kwargs.get("DeviceManufacturer", "yandex"),
                DeviceModel=kwargs.get("DeviceModel", "unit-test-client"),
                DeviceId=kwargs.get("DeviceId", "12345"),
            ),
        ),
    )


def create_event_exception(message_id=None, text=None):
    return TDirective(
        Header=TEventHeader(
            Namespace=TEventHeader.EMessageNamespace.SYSTEM,
            Name=TEventHeader.EMessageName.EVENT_EXCEPTION,
            MessageId=_or(message_id, str(uuid4())),
        ),
        Exception=TEventException(Text=_or(text, "error message")),
    )


def create_http_response(status_code=None, content=None):
    if isinstance(content, str):
        content = content.encode("utf-8")
    return THttpResponse(StatusCode=_or(status_code, 200), Content=_or(content, b""))


def create_ws_message(raw):
    body = raw.get("event", raw.get("directive", raw))
    return TWsEvent(Header=create_message_header(body["header"]), Text=json.dumps(raw))
