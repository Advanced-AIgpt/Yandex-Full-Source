import pytest
import json
from .common import Cuttlefish, create_grpc_request
from .common.constants import ItemTypes, ServiceHandles, Sources
from .common.items import create_synchronize_state_event, create_http_response, get_answer_item
from .common.utils import get_synchronize_state_event
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TUserInfo, TConnectionInfo
from alice.cuttlefish.library.protos.wsevent_pb2 import TEventHeader
from apphost.lib.proto_answers.http_pb2 import THttpRequest


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
def test_no_auth_token(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_PRE,
        request=create_grpc_request(
            items=[
                {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                {"type": ItemTypes.SYNCRHONIZE_STATE_EVENT, "data": create_synchronize_state_event(app_token="")},
            ]
        ),
    )

    assert len(response.Answers) == 1
    event_exception = get_answer_item(response, ItemTypes.DIRECTIVE)
    assert event_exception.Header.Namespace == TEventHeader.EMessageNamespace.SYSTEM
    assert event_exception.Header.Name == TEventHeader.EMessageName.EVENT_EXCEPTION
    assert event_exception.Exception.Text == "Invalid auth_token"


# -------------------------------------------------------------------------------------------------
def test_auth_token_is_not_in_whitelist(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_PRE,
        request=create_grpc_request(
            items=[
                {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                {
                    "type": ItemTypes.SYNCRHONIZE_STATE_EVENT,
                    "data": create_synchronize_state_event(app_token="app-token-absent-in-whitelist"),
                },
            ]
        ),
    )

    get_answer_item(response, ItemTypes.SESSION_CONTEXT)  # ensure that SessionContext isn't lost
    apikeys_http_request = get_answer_item(response, ItemTypes.APIKEYS_HTTP_REQUEST)
    assert apikeys_http_request.Method == THttpRequest.Get
    assert apikeys_http_request.Scheme == THttpRequest.Http


# -------------------------------------------------------------------------------------------------
def test_apikeys_service_token(cuttlefish: Cuttlefish):
    def get_apikeys_request(payload=None):
        response = cuttlefish.make_grpc_request(
            handle=ServiceHandles.SYNCHRONIZE_STATE_PRE,
            request=create_grpc_request(
                items=[
                    {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                    {
                        "type": ItemTypes.SYNCRHONIZE_STATE_EVENT,
                        "data": get_synchronize_state_event(cuttlefish, payload=payload),
                    },
                ]
            ),
        )

        return get_answer_item(response, ItemTypes.APIKEYS_HTTP_REQUEST)

    assert "service_token=speechkitmobile_" in get_apikeys_request().Path
    assert "service_token=speechkitmobile_" in get_apikeys_request({"service_name": "12345"}).Path
    assert "service_token=speechkitjsapi_" in get_apikeys_request({"service_name": "jsapi"}).Path


# -------------------------------------------------------------------------------------------------
def test_apikeys_rejected_key(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_POST,
        request=create_grpc_request(
            items=[
                {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                {
                    "type": ItemTypes.APIKEYS_HTTP_RESPONSE,
                    "data": create_http_response(status_code=404, content=b'{"error": "Key not found"}'),
                },
            ]
        ),
    )

    assert len(response.Answers) == 1
    event_exception = get_answer_item(response, ItemTypes.DIRECTIVE)
    assert event_exception.Header.Namespace == TEventHeader.EMessageNamespace.SYSTEM
    assert event_exception.Header.Name == TEventHeader.EMessageName.EVENT_EXCEPTION
    assert event_exception.Exception.Text == "Invalid auth_token"


# -------------------------------------------------------------------------------------------------
def test_with_yandex_uid_and_oauth_token(cuttlefish: Cuttlefish):
    # PRE
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_PRE,
        request=create_grpc_request(
            items=[
                {
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(ConnectionInfo=TConnectionInfo(IpAddress="123.123.123.123")),
                },
                {
                    "type": ItemTypes.SYNCRHONIZE_STATE_EVENT,
                    "data": get_synchronize_state_event(
                        cuttlefish,
                        payload={
                            "auth_token": "best-app-token-ever",
                            "yandexuid": "best-yandex-uid-ever",
                            "oauth_token": "best-oauth-token-ever",
                            "vins": {"application": {"device_id": "my-adorable-device-14", "platform": "android"}},
                            "wifi_networks": [
                                {"signal_strength": -62, "mac": "e0:d9:e3:7e:e7:ed"},
                                {"signal_strength": -65, "mac": "74:da:da:d0:d8:f4"},
                                {"signal_strength": -70, "mac": "f8:35:dd:8e:aa:c6"},
                            ],
                        },
                    ),
                },
            ]
        ),
    )

    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.HasField("DeviceInfo")

    blackbox_req = get_answer_item(response, ItemTypes.BLACKBOX_HTTP_REQUEST)
    assert blackbox_req.Method == THttpRequest.Get
    assert blackbox_req.Scheme == THttpRequest.Https
    assert "oauth_token=best-oauth-token-ever" in blackbox_req.Path

    apikeys_req = get_answer_item(response, ItemTypes.APIKEYS_HTTP_REQUEST)
    assert apikeys_req.Method == THttpRequest.Get
    assert apikeys_req.Scheme == THttpRequest.Http
    assert "key=best-app-token-ever" in apikeys_req.Path

    # POST

    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_POST,
        request=create_grpc_request(
            items=[
                {
                    "source_name": ServiceHandles.SYNCHRONIZE_STATE_PRE,
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": session_ctx,
                },
                {
                    "source_name": Sources.APIKEYS,
                    "type": ItemTypes.APIKEYS_HTTP_RESPONSE,
                    "data": create_http_response(content=json.dumps({"result": "OK", "key_info": {}})),
                },
                {
                    "source_name": Sources.BLACKBOX,
                    "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                    "data": create_http_response(
                        content=json.dumps(
                            {
                                "status": {"value": "VALID"},
                                "oauth": {"uid": "1234567890"},
                                "aliases": {"13": "KingOfYandex"},
                            }
                        )
                    ),
                },
            ]
        ),
    )

    assert len(response.Answers) == 1
    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.AppToken == "best-app-token-ever"
    assert session_ctx.UserInfo.Puid == "1234567890"
    assert session_ctx.UserInfo.StaffLogin == "KingOfYandex"


# -------------------------------------------------------------------------------------------------
def test_apply_personal_settings(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_POST,
        request=create_grpc_request(
            items=[
                {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                {
                    "type": ItemTypes.DATASYNC_HTTP_RESPONSE,
                    "data": create_http_response(
                        status_code=200,
                        content=b'{"items":[{"id":"do_not_use_user_logs","do_not_use_user_logs":true}]}',
                    ),
                },
            ]
        ),
    )

    assert len(response.Answers) == 1
    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.UserOptions.DoNotUseLogs is True


# -------------------------------------------------------------------------------------------------
def test_do_not_use_logs(cuttlefish: Cuttlefish):
    # BB request must have happened (AuthTokenType is OAUTH) but lack of Puid means that it's failed
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_POST,
        request=create_grpc_request(
            items=[
                {
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(
                        UserInfo=TUserInfo(
                            AuthTokenType=TUserInfo.ETokenType.OAUTH, AuthToken="the-best-oauth-token-ever"
                        )
                    ),
                }
            ]
        ),
    )
    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.UserOptions.DoNotUseLogs is True

    # BB request hasn't happened (AuthTokenType is not OAUTH)
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_POST,
        request=create_grpc_request(
            items=[
                {
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(
                        UserInfo=TUserInfo(
                            AuthTokenType=TUserInfo.ETokenType.OAUTH_TEAM, AuthToken="the-best-oauth-token-ever"
                        )
                    ),
                }
            ]
        ),
    )
    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.UserOptions.DoNotUseLogs is False

    # BB request succeeded but DataSync somehow failed
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_POST,
        request=create_grpc_request(
            items=[
                {
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(
                        UserInfo=TUserInfo(
                            AuthTokenType=TUserInfo.ETokenType.OAUTH, AuthToken="the-best-oauth-token-ever"
                        )
                    ),
                },
                {
                    "source_name": Sources.BLACKBOX,
                    "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                    "data": create_http_response(
                        content=json.dumps(
                            {
                                "status": {"value": "VALID"},
                                "oauth": {"uid": "1234567890"},
                                "aliases": {"13": "KingOfYandex"},
                            }
                        )
                    ),
                },
            ]
        ),
    )
    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.UserOptions.DoNotUseLogs is True

    # DataSync contains no 'do_not_use_user_logs' item
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_POST,
        request=create_grpc_request(
            items=[
                {
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(
                        UserInfo=TUserInfo(
                            AuthTokenType=TUserInfo.ETokenType.OAUTH, AuthToken="the-best-oauth-token-ever"
                        )
                    ),
                },
                {
                    "source_name": Sources.DATASYNC,
                    "type": ItemTypes.DATASYNC_HTTP_RESPONSE,
                    "data": create_http_response(content=json.dumps({"items": []})),
                },
            ]
        ),
    )
    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.UserOptions.DoNotUseLogs is False


# -------------------------------------------------------------------------------------------------
def test_app_type(cuttlefish: Cuttlefish):
    event = create_synchronize_state_event()  # default event
    event.SyncState.ApplicationInfo.Id = "ru.yandex.quasar.app"

    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_PRE,
        request=create_grpc_request(
            items=[
                {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                {"type": ItemTypes.SYNCRHONIZE_STATE_EVENT, "data": event},
            ]
        ),
    )

    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.AppType == "quasar"


# -------------------------------------------------------------------------------------------------
def test_sycnhronize_state_response(cuttlefish: Cuttlefish):
    def make_request(speechkit_version=None):
        session_ctx = TSessionContext()
        if speechkit_version is not None:
            session_ctx.SpeechkitVersion = speechkit_version
        return cuttlefish.make_grpc_request(
            handle=ServiceHandles.SYNCHRONIZE_STATE_POST,
            request=create_grpc_request(items=[{"type": ItemTypes.SESSION_CONTEXT, "data": session_ctx}]),
        )

    for ver in (None, "asdasd", "9999.9999.9999", "1.2.-100", "4.5.999"):
        response = make_request(ver)
        get_answer_item(response, ItemTypes.SESSION_CONTEXT)  # ensure that it's not lost
        with pytest.raises(KeyError):
            get_answer_item(response, ItemTypes.DIRECTIVE)
    for ver in ("4.6.0", "4.6.1", "999.999.999"):
        response = make_request(ver)
        get_answer_item(response, ItemTypes.SESSION_CONTEXT)  # ensure that it's not lost
        msg = get_answer_item(response, ItemTypes.DIRECTIVE)
        assert msg.Header.Namespace == TEventHeader.EMessageNamespace.SYSTEM
        assert msg.Header.Name == TEventHeader.EMessageName.SYNCHRONIZE_STATE_RESPONSE
        assert msg.HasField("SyncStateResponse")


# -------------------------------------------------------------------------------------------------
def test_client_type(cuttlefish: Cuttlefish):
    def do_request(token):
        event = get_synchronize_state_event(cuttlefish, {"auth_token": token})
        return cuttlefish.make_grpc_request(
            handle=ServiceHandles.SYNCHRONIZE_STATE_PRE,
            request=create_grpc_request(
                items=[
                    {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                    {"type": ItemTypes.SYNCRHONIZE_STATE_EVENT, "data": event},
                ]
            ),
        )

    response = do_request("some-not-quasar-auth-token")
    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.ClientType == TSessionContext.EClientType.CLIENT_TYPE_UNDEFINED

    response = do_request("51ae06cc-5c8f-48dc-93ae-7214517679e6")
    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.ClientType == TSessionContext.EClientType.CLIENT_TYPE_QUASAR


# -------------------------------------------------------------------------------------------------
def test_merge_user_info(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_PRE,
        request=create_grpc_request(
            items=[
                {
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(
                        ConnectionInfo=TConnectionInfo(
                            IpAddress="123.123.123.123",
                        ),
                        UserInfo=TUserInfo(
                            Uuid="old-uuid",
                            Yuid="old-yuid",
                        ),
                    ),
                },
                {
                    "type": ItemTypes.SYNCRHONIZE_STATE_EVENT,
                    "data": create_synchronize_state_event(
                        app_token="best-app-token-ever", uuid="new-uuid", vins_app_uuid="NEW-UUID"
                    ),
                },
            ]
        ),
    )

    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    user_info = session_ctx.UserInfo

    assert user_info.Uuid == "new-uuid"
    assert user_info.VinsApplicationUuid == "NEW-UUID"
    assert user_info.Yuid == "old-yuid"
