from .common.utils import find_header
from .common import Cuttlefish, create_grpc_request
from .common.constants import ItemTypes, ServiceHandles, Sources
from .common.items import get_answer_item, create_http_response
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TUserOptions
from alice.cuttlefish.library.protos.wsevent_pb2 import TEventHeader
from apphost.lib.proto_answers.http_pb2 import THttpResponse, THttpRequest, THeader
import pytest
import json


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
def test_post__blackbox_ok__valid(cuttlefish: Cuttlefish):
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_POST,
        request=create_grpc_request(
            items=[
                {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                {
                    "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                    "data": THttpResponse(
                        StatusCode=200,
                        Content=json.dumps(
                            {
                                "status": {"value": "VALID"},
                                "user_ticket": "best-user-ticket-ever",
                                "oauth": {"uid": "1234567890"},
                                "aliases": {"13": "KingOfYandex"},
                            }
                        ).encode("utf-8"),
                    ),
                },
            ]
        ),
    )

    with pytest.raises(KeyError):
        get_answer_item(response, ItemTypes.DIRECTIVE)

    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert session_ctx.UserInfo.Puid == "1234567890"
    assert session_ctx.UserInfo.StaffLogin == "KingOfYandex"


# -------------------------------------------------------------------------------------------------
def test_post__blackbox_ok__invalid(cuttlefish: Cuttlefish):
    def send(session_ctx):
        return cuttlefish.make_grpc_request(
            handle=ServiceHandles.SYNCHRONIZE_STATE_POST,
            request=create_grpc_request(
                items=[
                    {"type": ItemTypes.SESSION_CONTEXT, "data": session_ctx},
                    {
                        "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                        "data": THttpResponse(
                            StatusCode=200, Content=json.dumps({"status": {"value": "EXPIRED"}}).encode("utf-8")
                        ),
                    },
                ]
            ),
        )

    # default
    response = send(TSessionContext())
    with pytest.raises(KeyError):
        get_answer_item(response, ItemTypes.DIRECTIVE)
    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert not session_ctx.UserInfo.HasField("Puid")
    assert not session_ctx.UserInfo.HasField("StaffLogin")

    # with "accept_invalid_auth"
    response = send(TSessionContext(UserOptions=TUserOptions(AcceptInvalidAuth=True)))
    msg = get_answer_item(response, ItemTypes.DIRECTIVE)
    assert msg.Header.Namespace == TEventHeader.EMessageNamespace.SYSTEM
    assert msg.Header.Name == TEventHeader.EMessageName.INVALID_AUTH
    session_ctx = get_answer_item(response, ItemTypes.SESSION_CONTEXT)
    assert not session_ctx.UserInfo.HasField("Puid")
    assert not session_ctx.UserInfo.HasField("StaffLogin")


# -------------------------------------------------------------------------------------------------
def test_blackbox_setdown(cuttlefish: Cuttlefish):
    """BlackBox answers with valid status"""
    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_BLACKBOX_SETDOWN,
        request=create_grpc_request(
            items=[
                {
                    "source_name": Sources.SYNCHRONIZE_STATE_PRE,
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(),
                },
                {
                    "source_name": Sources.BLACKBOX,
                    "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                    "data": create_http_response(
                        content=json.dumps(
                            {
                                "status": {"value": "VALID"},
                                "user_ticket": "best-user-ticket-ever",
                                "oauth": {"uid": "1234567890"},
                                "aliases": {"13": "KingOfYandex"},
                            }
                        )
                    ),
                },
                {
                    "source_name": Sources.SYNCHRONIZE_STATE_PRE,
                    "type": ItemTypes.HTTP_REQUEST_DRAFT,
                    "data": THttpRequest(
                        Scheme=THttpRequest.EScheme.Https,
                        Path=f"{ItemTypes.DATASYNC_HTTP_REQUEST}@/v1/personality/profile/alisa/settings",
                        Method=THttpRequest.EMethod.Get,
                    ),
                },
            ]
        ),
    )

    # DataSync request got user ticket
    datasync_request = get_answer_item(response, ItemTypes.DATASYNC_HTTP_REQUEST)
    assert THeader(Name="X-Ya-User-Ticket", Value="best-user-ticket-ever") in datasync_request.Headers
    assert datasync_request.Path == "/v1/personality/profile/alisa/settings"


# -------------------------------------------------------------------------------------------------
def test_blackbox_setdown__invalid(cuttlefish: Cuttlefish):
    """Test what if BlackBox answers that given OAauth token is not valid"""

    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_BLACKBOX_SETDOWN,
        request=create_grpc_request(
            items=[
                {
                    "source_name": Sources.SYNCHRONIZE_STATE_PRE,
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(),
                },
                {
                    "source_name": Sources.BLACKBOX,
                    "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                    "data": create_http_response(content=json.dumps({"status": {"value": "EXPIRED"}})),
                },
                {
                    "source_name": Sources.SYNCHRONIZE_STATE_PRE,
                    "type": ItemTypes.HTTP_REQUEST_DRAFT,
                    "data": THttpRequest(
                        Scheme=THttpRequest.EScheme.Https,
                        Path=f"{ItemTypes.DATASYNC_HTTP_REQUEST}@/v1/personality/profile/alisa/settings",
                        Method=THttpRequest.EMethod.Get,
                    ),
                },
                {
                    "source_name": Sources.SYNCHRONIZE_STATE_PRE,
                    "type": ItemTypes.HTTP_REQUEST_DRAFT,
                    "data": THttpRequest(
                        Scheme=THttpRequest.EScheme.Http,
                        Path=f"{ItemTypes.FLAGS_JSON_HTTP_REQUEST}@?uuid=bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb",
                        Method=THttpRequest.EMethod.Get,
                    ),
                },
            ]
        ),
    )

    # DataSync request is dropped
    with pytest.raises(KeyError):
        get_answer_item(response, ItemTypes.DATASYNC_HTTP_REQUEST)

    # flags.json is used without puid
    flag_json_request = get_answer_item(response, ItemTypes.FLAGS_JSON_HTTP_REQUEST, proto=THttpRequest)
    assert flag_json_request.Path == "?uuid=bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb"
    find_header(flag_json_request, "X-Yandex-Puid") is None


# -------------------------------------------------------------------------------------------------
def test_blackbox_setdown__no_blackbox(cuttlefish: Cuttlefish):
    """Test what if BlackBox doesn't answer (or there was no request)"""

    response = cuttlefish.make_grpc_request(
        handle=ServiceHandles.SYNCHRONIZE_STATE_BLACKBOX_SETDOWN,
        request=create_grpc_request(
            items=[
                {
                    "source_name": Sources.SYNCHRONIZE_STATE_PRE,
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(),
                },
                {
                    "source_name": Sources.SYNCHRONIZE_STATE_PRE,
                    "type": ItemTypes.HTTP_REQUEST_DRAFT,
                    "data": THttpRequest(
                        Scheme=THttpRequest.EScheme.Https,
                        Path=f"{ItemTypes.DATASYNC_HTTP_REQUEST}@/v1/personality/profile/alisa/settings",
                        Method=THttpRequest.EMethod.Get,
                    ),
                },
                {
                    "source_name": Sources.SYNCHRONIZE_STATE_PRE,
                    "type": ItemTypes.HTTP_REQUEST_DRAFT,
                    "data": THttpRequest(
                        Scheme=THttpRequest.EScheme.Http,
                        Path=f"{ItemTypes.FLAGS_JSON_HTTP_REQUEST}@?uuid=bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb",
                        Method=THttpRequest.EMethod.Get,
                    ),
                },
            ]
        ),
    )

    # DataSync request is dropped
    with pytest.raises(KeyError):
        get_answer_item(response, ItemTypes.DATASYNC_HTTP_REQUEST)

    # flags.json is used without puid
    flag_json_request = get_answer_item(response, ItemTypes.FLAGS_JSON_HTTP_REQUEST, proto=THttpRequest)
    assert flag_json_request.Path == "?uuid=bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb"
    find_header(flag_json_request, "X-Yandex-Puid") is None
