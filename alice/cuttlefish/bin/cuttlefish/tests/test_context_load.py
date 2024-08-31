import base64
import itertools
import json
import logging
import pytest

from .common import Cuttlefish, create_grpc_request
from .common.utils import find_header, find_header_json
from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.python.testing.items import get_answer_item, get_answer_items
from alice.cuttlefish.library.python.testing.utils import extract_json
from alice.cachalot.api.protos.cachalot_pb2 import (
    EResponseStatus,
    TMegamindSessionLoadResponse,
    TGetRequest as TCachalotGetRequest,
    TSetRequest as TCachalotSetRequest,
    TResponse as TCachalotResponse,
    TGetResponse as TCachalotGetResponse,
)
from alice.cuttlefish.library.protos.antirobot_pb2 import (
    EAntirobotMode,
    TAntirobotInputData,
    TAntirobotInputSettings,
    TRobotnessData,
)
from alice.cuttlefish.library.protos.context_load_pb2 import (
    TContextLoadLaasRequestOptions,
    TContextLoadResponse,
    TContextLoadSmarthomeUid,
)
from alice.cuttlefish.library.protos.session_pb2 import (
    TAbFlagsProviderOptions,
    TConnectionInfo,
    TDeviceInfo,
    TFlagsInfo,
    TRequestContext,
    TSessionContext,
    TUserInfo,
)
from alice.cuttlefish.library.protos.uniproxy2_pb2 import (
    TUniproxyDirective,
)
from voicetech.library.settings_manager.proto.settings_pb2 import (
    TManagedSettings,
)

from alice.iot.bulbasaur.protos.apphost.iot_pb2 import TAlice4Business
from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from alice.megamind.protos.scenarios.directives_pb2 import TPatchAsrOptionsForNextRequestDirective
from alice.memento.proto.api_pb2 import TRespGetAllObjects, TReqGetAllObjects
from apphost.lib.proto_answers.http_pb2 import THttpRequest, THttpResponse
from apphost.lib.proto_answers.tvm_user_ticket_pb2 import TTvmUserTicket
from mssngr.router.lib.protos.registry.alice_pb2 import TListContactsRequest

from google.protobuf.wrappers_pb2 import BoolValue, FloatValue, Int32Value
from http import HTTPStatus

DATASYNC_REQUEST_CONTENT = json.dumps(
    {
        'items': [
            {'method': 'GET', 'relative_url': '/v2/personality/profile/addresses'},
            {'method': 'GET', 'relative_url': '/v1/personality/profile/alisa/kv'},
            {'method': 'GET', 'relative_url': '/v1/personality/profile/alisa/settings'},
        ]
    },
    separators=(',', ':'),
)

MINIMAL_SESSION_CONTEXT = TSessionContext(
    UserInfo=TUserInfo(
        Uuid="abacaba",  # request must have uuid
    )
)


def assert_starts_with(string, prefix):
    assert string.startswith(prefix)
    return string[len(prefix) :]


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
class TestContextLoadPre:
    def _check_response_contains_required_items(self, response):
        assert get_answer_item(response, ItemTypes.DATASYNC_DEVICE_ID_HTTP_REQUEST)
        assert get_answer_item(response, ItemTypes.DATASYNC_UUID_HTTP_REQUEST)

    def test_context_almost_empty(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": MINIMAL_SESSION_CONTEXT,
                    }
                ],
            ),
        )

        assert len(response.Answers) == 1
        assert get_answer_item(response, ItemTypes.NOTHING)

    def test_session_id_auth(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            ConnectionInfo=TConnectionInfo(
                                IpAddress="10.20.30.40",
                                Origin="yandex.ya",
                                XYambCookie="yandexuid=123; Session_id=sessid; some-other=stuff-here",
                            ),
                        ),
                    }
                ],
            ),
        )
        blackbox_http_req = get_answer_item(response, ItemTypes.BLACKBOX_HTTP_REQUEST, proto=THttpRequest)
        logging.info(f"Blackbox request: {blackbox_http_req}")
        assert "userip=10.20.30.40" in blackbox_http_req.Path
        assert "host=yandex.ya" in blackbox_http_req.Path
        assert "sessionid=sessid" in blackbox_http_req.Path

    def test_only_blackbox(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(
                                AuthToken="my-lovely-auth-token",
                                Uuid="abacaba",
                            ),
                            ConnectionInfo=TConnectionInfo(
                                IpAddress="10.20.30.40",
                            ),
                        ),
                    }
                ],
            ),
        )

        assert len(response.Answers) == 1

        blackbox_http_req = get_answer_item(response, ItemTypes.BLACKBOX_HTTP_REQUEST, proto=THttpRequest)
        logging.info(f"Blackbox request: {blackbox_http_req}")
        assert "userip=10.20.30.40" in blackbox_http_req.Path
        assert "oauth_token=my-lovely-auth-token" in blackbox_http_req.Path

    def test_only_datasync(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(
                                Uuid="some-cool-uuid",
                            ),
                        ),
                    }
                ],
            ),
        )

        assert len(response.Answers) == 1
        assert get_answer_item(response, ItemTypes.NOTHING)

    def test_only_datasync_cache_when_caching_disabled(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(
                                AuthToken="my-lovely-auth-token",
                                Uuid="abacaba",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            SettingsFromManager=TManagedSettings(
                                UseDatasyncCache=False,
                            ),
                        ),
                    },
                ],
            ),
        )

        assert not list(get_answer_items(response, ItemTypes.DATASYNC_CACHE_GET_REQUEST, proto=TCachalotGetRequest))

    def test_only_datasync_cache_without_auth_token(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(
                                Uuid="abacaba",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            SettingsFromManager=TManagedSettings(
                                UseDatasyncCache=True,
                            ),
                        ),
                    },
                ],
            ),
        )

        assert not list(get_answer_items(response, ItemTypes.DATASYNC_CACHE_GET_REQUEST, proto=TCachalotGetRequest))

    def test_only_antirobot(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": MINIMAL_SESSION_CONTEXT,
                    },
                    {
                        "type": ItemTypes.ANTIROBOT_INPUT_SETTINGS,
                        "data": TAntirobotInputSettings(Mode=EAntirobotMode.EVALUATE),
                    },
                    {
                        "type": ItemTypes.ANTIROBOT_INPUT_DATA,
                        "data": TAntirobotInputData(ForwardedFor='10.0.0.1', Ja3='Ja3', Ja4='Ja4', Body='ANTIBODY'),
                    },
                ],
            ),
        )
        assert len(response.Answers) == 1

        req = get_answer_item(response, ItemTypes.ANTIROBOT_HTTP_REQUEST)
        assert req
        assert req.Path == "/fullreq"
        assert '\r\n\r\nANTIBODY' in req.Content.decode("ascii")
        assert find_header(req, "X-Forwarded-For-Y") == '10.0.0.1'
        assert find_header(req, "X-Yandex-Ja3") == 'Ja3'
        assert find_header(req, "X-Yandex-Ja4") == 'Ja4'

    def test_only_antirobot_no_data_evaluate(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": MINIMAL_SESSION_CONTEXT,
                    },
                    {
                        "type": ItemTypes.ANTIROBOT_INPUT_SETTINGS,
                        "data": TAntirobotInputSettings(Mode=EAntirobotMode.EVALUATE),
                    },
                ],
            ),
        )
        assert len(response.Answers) == 1
        assert get_answer_item(response, ItemTypes.NOTHING)

    def test_only_antirobot_no_data_apply(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": MINIMAL_SESSION_CONTEXT,
                    },
                    {
                        "type": ItemTypes.ANTIROBOT_INPUT_SETTINGS,
                        "data": TAntirobotInputSettings(Mode=EAntirobotMode.APPLY),
                    },
                ],
            ),
        )
        assert len(response.Answers) == 1
        assert get_answer_item(response, ItemTypes.NOTHING)

    def test_only_antirobot_no_settings(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": MINIMAL_SESSION_CONTEXT,
                    },
                    {
                        "type": ItemTypes.ANTIROBOT_INPUT_DATA,
                        "data": TAntirobotInputData(ForwardedFor='10.0.0.1', Ja3='Ja3', Ja4='Ja4', Body='ANTIBODY'),
                    },
                ],
            ),
        )
        assert len(response.Answers) == 1
        assert get_answer_item(response, ItemTypes.NOTHING)

    @pytest.mark.parametrize("prev_req_id", ["", "prev_req_id"])
    def test_cachalot_load_asr_options_patch_request(self, cuttlefish: Cuttlefish, prev_req_id):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": MINIMAL_SESSION_CONTEXT,
                    },
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            Header=TRequestContext.THeader(
                                MessageId="message_id",
                                SessionId="session_id",
                                PrevReqId=prev_req_id,
                            ),
                        ),
                    },
                ],
            ),
        )

        cachalot_load_asr_options_patch_reqs = list(
            get_answer_items(response, ItemTypes.CACHALOT_LOAD_ASR_OPTIONS_PATCH_REQUEST, proto=TCachalotGetRequest)
        )
        if prev_req_id == "":
            # No request for empty prev_req_id
            assert len(cachalot_load_asr_options_patch_reqs) == 0
        else:
            assert len(cachalot_load_asr_options_patch_reqs) == 1
            assert cachalot_load_asr_options_patch_reqs[0]

            cachalot_get_req = cachalot_load_asr_options_patch_reqs[0]
            logging.info(f"Cachalot load asr options patch get request: {cachalot_get_req}")
            assert cachalot_get_req.Key == prev_req_id
            assert cachalot_get_req.StorageTag == "AsrOptions"


class TestContextLoadBlackboxSetdown:
    @pytest.mark.parametrize(
        "bb_response",
        [
            # non-200 code
            THttpResponse(
                StatusCode=418,
                Content=json.dumps(
                    {
                        "status": {"value": "KEK"},
                        "description": "YA TOMAT",
                    }
                ).encode("utf-8"),
            ),
            # non-valid response
            THttpResponse(
                StatusCode=200,
                Content=json.dumps(
                    {
                        "exception": {
                            "value": "INVALID_PARAMS",
                            "id": 2,
                        },
                        "error": "BlackBox error: param 'get_user_ticket' is allowed only with header 'X-Ya-Service-Ticket'.",
                    }
                ).encode("utf-8"),
            ),
            # no Blackbox response (timeout or smth)
            None,
        ],
    )
    def test_blackbox_fails(self, cuttlefish: Cuttlefish, bb_response):
        items = [
            {
                "type": ItemTypes.SESSION_CONTEXT,
                "data": TSessionContext(
                    UserInfo=TUserInfo(
                        AuthToken="my-lovely-auth-token",
                        Uuid="abacaba",
                    ),
                    ConnectionInfo=TConnectionInfo(
                        IpAddress="10.20.30.40",
                    ),
                ),
            }
        ]

        if bb_response is not None:
            items.append(
                {
                    "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                    "data": bb_response,
                }
            )

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_BLACKBOX_SETDOWN, create_grpc_request(items=items)
        )
        assert len(response.Answers) == 3

        for typename, uid_value in [
            (ItemTypes.DATASYNC_DEVICE_ID_HTTP_REQUEST, "device_id:abacaba"),
            (ItemTypes.DATASYNC_UUID_HTTP_REQUEST, "uuid:abacaba"),
        ]:
            datasync_http_req = get_answer_item(response, typename, proto=THttpRequest)
            assert datasync_http_req
            logging.info(f"Datasync ({typename}) request: {datasync_http_req}")

            assert datasync_http_req.Path == "/v1/batch/request"
            assert datasync_http_req.Content.decode("ascii") == DATASYNC_REQUEST_CONTENT

            assert len(datasync_http_req.Headers) == 2
            assert find_header(datasync_http_req, "Content-Type") == "application/json; charset=utf-8"
            assert find_header(datasync_http_req, "X-Uid") == uid_value

    @pytest.mark.parametrize("is_notifications_supported", [False, True])
    def test_blackbox_correct(self, cuttlefish: Cuttlefish, is_notifications_supported):
        expected_item_count = 7
        supported_features = []

        if is_notifications_supported:
            expected_item_count += 1
            supported_features.append("notifications")

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_BLACKBOX_SETDOWN,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(
                                AuthToken="my-lovely-auth-token",
                                Uuid="my-lovely-uuid",
                            ),
                            ConnectionInfo=TConnectionInfo(
                                IpAddress="10.20.30.40",
                            ),
                            DeviceInfo=TDeviceInfo(
                                DeviceId="my lovely! device id",
                                DeviceModel="kitayfon",
                                SupportedFeatures=supported_features,
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.SMARTHOME_UID,
                        "data": TContextLoadSmarthomeUid(
                            Value="2808",
                        ),
                    },
                    {
                        "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                        "data": THttpResponse(
                            StatusCode=200,
                            Content=json.dumps(
                                {
                                    "status": {"value": "VALID"},
                                    "user_ticket": "my-lovely-user-ticket",
                                    "oauth": {"uid": "1234567890"},
                                    "aliases": {"13": "KingOfYandex"},
                                }
                            ).encode("utf-8"),
                        ),
                    },
                ]
            ),
        )

        memento_req = get_answer_item(response, ItemTypes.MEMENTO_GET_ALL_OBJECTS_REQUEST, proto=TReqGetAllObjects)
        logging.info(f"Memento request: {memento_req}")
        assert len(memento_req.SurfaceId) == 1
        assert memento_req.SurfaceId[0] == "my-lovely-uuid"

        datasync_http_req = get_answer_item(response, ItemTypes.DATASYNC_HTTP_REQUEST, proto=THttpRequest)
        logging.info(f"Datasync request: {datasync_http_req}")
        assert datasync_http_req.Path == "/v1/batch/request"
        assert datasync_http_req.Content.decode("ascii") == DATASYNC_REQUEST_CONTENT
        assert find_header(datasync_http_req, "Content-Type") == "application/json; charset=utf-8"
        assert find_header(datasync_http_req, "X-Ya-User-Ticket") == "my-lovely-user-ticket"
        assert find_header(datasync_http_req, "X-Uid") == "1234567890"
        assert len(datasync_http_req.Headers) == 3

        iot_user_info_req = get_answer_item(response, ItemTypes.TVM_USER_TICKET, proto=TTvmUserTicket)
        logging.info(f"IoT request (user ticket): {iot_user_info_req}")
        assert iot_user_info_req.UserTicket == "my-lovely-user-ticket"

        tvm_user_ticket = get_answer_item(response, ItemTypes.TVM_USER_TICKET, proto=TTvmUserTicket)
        logging.info(f"IoT request (user ticket): {tvm_user_ticket}")
        assert tvm_user_ticket.UserTicket == "my-lovely-user-ticket"

        iot_a4b_req = get_answer_item(response, ItemTypes.QUASARIOT_REQUEST_ALICE_FOR_BUSINESS, proto=TAlice4Business)
        logging.info(f"IoT request (Alice4Business): {iot_a4b_req}")
        assert iot_a4b_req.Uid == 2808

        contacts_proto_http_req = get_answer_item(response, ItemTypes.CONTACTS_PROTO_HTTP_REQUEST, proto=THttpRequest)
        logging.info(f"contacts proto request: {contacts_proto_http_req}")
        assert contacts_proto_http_req.Path == "/meta_api/list_contacts_alice"
        content = TListContactsRequest()
        content.ParseFromString(contacts_proto_http_req.Content)
        assert content.Uid == 1234567890

        if is_notifications_supported:
            notificator_http_req = get_answer_item(response, ItemTypes.NOTIFICATOR_HTTP_REQUEST, proto=THttpRequest)
            logging.info(f"Notificator request: {notificator_http_req}")
            assert (
                notificator_http_req.Path
                == "/notifications?puid=1234567890&device_id=my lovely! device id&device_model=kitayfon"
            )
            assert find_header(datasync_http_req, "Content-Type") == "application/json; charset=utf-8"
            assert len(notificator_http_req.Headers) == 1
        else:
            assert len(list(get_answer_items(response, ItemTypes.NOTIFICATOR_HTTP_REQUEST, proto=THttpRequest))) == 0

        assert len(response.Answers) == expected_item_count


class TestContextLoadMakeContactsRequest:
    def test_make_contacts(self, cuttlefish: Cuttlefish):
        supported_features = []

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_MAKE_CONTACTS_REQUEST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(
                                AuthToken="my-lovely-auth-token",
                                Uuid="my-lovely-uuid",
                            ),
                            ConnectionInfo=TConnectionInfo(
                                IpAddress="10.20.30.40",
                            ),
                            DeviceInfo=TDeviceInfo(
                                DeviceId="my lovely! device id",
                                DeviceModel="kitayfon",
                                SupportedFeatures=supported_features,
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                        "data": THttpResponse(
                            StatusCode=200,
                            Content=json.dumps(
                                {
                                    "status": {"value": "VALID"},
                                    "user_ticket": "my-lovely-user-ticket",
                                    "oauth": {"uid": "1234567890"},
                                    "aliases": {"13": "KingOfYandex"},
                                }
                            ).encode("utf-8"),
                        ),
                    },
                ]
            ),
        )

        contacts_proto_http_req = get_answer_item(response, ItemTypes.CONTACTS_PROTO_HTTP_REQUEST, proto=THttpRequest)
        logging.info(f"contacts proto request: {contacts_proto_http_req}")
        assert contacts_proto_http_req.Path == "/meta_api/list_contacts_alice"
        content = TListContactsRequest()
        content.ParseFromString(contacts_proto_http_req.Content)
        assert content.Uid == 1234567890


class TestContextLoadPost:

    http_response_types = [
        (ItemTypes.DATASYNC_DEVICE_ID_HTTP_RESPONSE, "DatasyncDeviceIdResponse"),
        (ItemTypes.DATASYNC_UUID_HTTP_RESPONSE, "DatasyncUuidResponse"),
        (ItemTypes.NOTIFICATOR_HTTP_RESPONSE, "NotificatorResponse"),
    ]

    @pytest.mark.parametrize("item_type, field_name", http_response_types)
    def test_single_answer(self, cuttlefish: Cuttlefish, item_type, field_name):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_POST,
            create_grpc_request(
                items=[
                    {
                        "type": item_type,
                        "data": THttpResponse(
                            StatusCode=418,
                            Content=b"xXx_SoMeAnSwEr_xXx",
                        ),
                    }
                ]
            ),
        )

        assert len(response.Answers) == 1
        response = get_answer_item(response, ItemTypes.CONTEXT_LOAD_RESPONSE, proto=TContextLoadResponse)
        logging.info(f"ContextLoad response: {response}")

        assert len(response.ListFields()) == 1
        assert response.HasField(field_name)

        http_resp = getattr(response, field_name)
        assert http_resp.StatusCode == 418
        assert http_resp.Content == b"xXx_SoMeAnSwEr_xXx"

    def test_memento_answer(self, cuttlefish: Cuttlefish):
        memento_response = TRespGetAllObjects()
        memento_response.UserConfigs.TtsWhisperConfig.Enabled = True

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.MEMENTO_USER_OBJECTS,
                        "data": memento_response,
                    }
                ]
            ),
        )

        assert len(response.Answers) == 1
        response = get_answer_item(response, ItemTypes.CONTEXT_LOAD_RESPONSE, proto=TContextLoadResponse)
        logging.info(f"ContextLoad response: {response}")

        assert len(response.ListFields()) == 1
        assert response.HasField("MementoResponse")

        assert memento_response.ParseFromString(response.MementoResponse.Content)
        assert memento_response.UserConfigs.TtsWhisperConfig.Enabled

    def test_mm_session(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.MM_SESSION_RESPONSE,
                        "data": TCachalotResponse(
                            Status=EResponseStatus.OK,
                            MegamindSessionLoadResp=TMegamindSessionLoadResponse(
                                Data=b"bratok, nishtyak",
                            ),
                        ),
                    }
                ]
            ),
        )

        assert len(response.Answers) == 1
        response = get_answer_item(response, ItemTypes.CONTEXT_LOAD_RESPONSE, proto=TContextLoadResponse)

        assert len(response.ListFields()) == 1
        assert response.HasField("MegamindSessionResponse")

        mm_session_resp = response.MegamindSessionResponse
        assert mm_session_resp.Status == EResponseStatus.OK
        assert mm_session_resp.MegamindSessionLoadResp.Data == b"bratok, nishtyak"

    def test_full_answers(self, cuttlefish: Cuttlefish):
        items = []
        for item_type, field_name in self.http_response_types:
            items.append(
                {
                    "type": item_type,
                    "data": THttpResponse(
                        StatusCode=418,
                        Content=str.encode("xXx_SoMeAnSwEr_xXx: " + field_name),
                    ),
                }
            )

        items.append(
            {
                "type": ItemTypes.QUASARIOT_RESPONSE_IOT_USER_INFO,
                "data": TIoTUserInfo(
                    RawUserInfo="iot-raw-data",
                ),
            }
        )

        response = cuttlefish.make_grpc_request(ServiceHandles.CONTEXT_LOAD_POST, create_grpc_request(items=items))
        assert len(response.Answers) == 1

        response = get_answer_item(response, ItemTypes.CONTEXT_LOAD_RESPONSE, proto=TContextLoadResponse)
        assert len(response.ListFields()) == len(self.http_response_types) + 1

        for item_type, field_name in self.http_response_types:
            assert response.HasField(field_name)
            http_resp = getattr(response, field_name)
            assert http_resp.StatusCode == 418
            assert http_resp.Content == str.encode("xXx_SoMeAnSwEr_xXx: " + field_name)

        assert response.HasField("IoTUserInfo")
        iot_resp = getattr(response, "IoTUserInfo")
        assert iot_resp.RawUserInfo == "iot-raw-data"

    def test_user_ticket(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                        "data": THttpResponse(
                            StatusCode=200,
                            Content=json.dumps(
                                {
                                    "status": {"value": "VALID"},
                                    "user_ticket": "my-lovely-user-ticket",
                                    "oauth": {"uid": "1234567890"},
                                    "aliases": {"13": "KingOfYandex"},
                                }
                            ).encode("utf-8"),
                        ),
                    }
                ]
            ),
        )

        assert len(response.Answers) == 1
        response = get_answer_item(response, ItemTypes.CONTEXT_LOAD_RESPONSE, proto=TContextLoadResponse)

        assert len(response.ListFields()) == 2

        assert response.HasField("UserTicket")
        assert response.UserTicket == "my-lovely-user-ticket"

        assert response.HasField("BlackboxUid")
        assert response.BlackboxUid == "1234567890"

    @pytest.mark.parametrize(
        "data, settings, expected",
        [
            (
                THttpResponse(StatusCode=302),
                TAntirobotInputSettings(Mode=EAntirobotMode.EVALUATE),
                TRobotnessData(IsRobot=False, Robotness=0.0),
            ),
            (
                THttpResponse(StatusCode=403),
                TAntirobotInputSettings(Mode=EAntirobotMode.EVALUATE),
                TRobotnessData(IsRobot=False, Robotness=0.0),
            ),
            (
                THttpResponse(StatusCode=200),
                TAntirobotInputSettings(Mode=EAntirobotMode.EVALUATE),
                TRobotnessData(IsRobot=False, Robotness=0.0),
            ),
            (
                THttpResponse(StatusCode=302),
                TAntirobotInputSettings(Mode=EAntirobotMode.APPLY),
                TRobotnessData(IsRobot=True, Robotness=1.0),
            ),
            (
                THttpResponse(StatusCode=403),
                TAntirobotInputSettings(Mode=EAntirobotMode.APPLY),
                TRobotnessData(IsRobot=True, Robotness=1.0),
            ),
            (
                THttpResponse(StatusCode=200),
                TAntirobotInputSettings(Mode=EAntirobotMode.APPLY),
                TRobotnessData(IsRobot=False, Robotness=0.0),
            ),
            (THttpResponse(StatusCode=302), TAntirobotInputSettings(Mode=EAntirobotMode.OFF), None),
            (THttpResponse(StatusCode=403), TAntirobotInputSettings(Mode=EAntirobotMode.OFF), None),
            (THttpResponse(StatusCode=200), TAntirobotInputSettings(Mode=EAntirobotMode.OFF), None),
            (THttpResponse(StatusCode=302), None, None),
            (THttpResponse(StatusCode=403), None, None),
            (THttpResponse(StatusCode=200), None, None),
            (None, TAntirobotInputSettings(Mode=EAntirobotMode.APPLY), None),
            (None, TAntirobotInputSettings(Mode=EAntirobotMode.EVALUATE), None),
            (None, TAntirobotInputSettings(Mode=EAntirobotMode.OFF), None),
            (None, None, None),
        ],
    )
    def test_antirobot_response(self, cuttlefish: Cuttlefish, data, settings, expected):
        items = []

        if data:
            items.append(
                {
                    "type": ItemTypes.ANTIROBOT_HTTP_RESPONSE,
                    "data": data,
                }
            )

        if settings:
            items.append(
                {
                    "type": ItemTypes.ANTIROBOT_INPUT_SETTINGS,
                    "data": settings,
                }
            )

        response = cuttlefish.make_grpc_request(ServiceHandles.CONTEXT_LOAD_POST, create_grpc_request(items=items))

        assert len(response.Answers) == 1
        response = get_answer_item(response, ItemTypes.CONTEXT_LOAD_RESPONSE, proto=TContextLoadResponse)

        if expected is not None:
            assert len(response.ListFields()) == 1
            assert response.HasField("Robotness")
            assert response.Robotness == expected
        else:
            assert len(response.ListFields()) == 0

    def test_flags_info_ok(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.FLAGS_JSON_HTTP_RESPONSE,
                        "data": {
                            "flags_json_version": "1632",
                            "all": {
                                "CONTEXT": {
                                    "MAIN": {
                                        "VOICE": {
                                            "flags": [
                                                "UaasVinsUrl_aHR0cDovL21lZ2FtaW5kLXJjLmFsaWNlLnlhbmRleC5uZXQ=",
                                                "zero_testing_code=43929",
                                            ]
                                        },
                                        "ASR": {"flags": ["asr_flag"]},
                                    }
                                },
                                "TESTID": ["384379"],
                            },
                            "reqid": "over9000",
                            "exphandler": "uniproxy",
                            "exp_config_version": "1028186",
                            "ids": ["384379"],
                            "exp_boxes": "384379,0,-1",
                        },
                    }
                ]
            ),
        )

        assert len(response.Answers) == 3

        cl_response = get_answer_item(response, ItemTypes.CONTEXT_LOAD_RESPONSE, proto=TContextLoadResponse)
        assert cl_response.HasField("FlagsJsonResponse")

        fi_response = get_answer_item(response, ItemTypes.FLAGS_INFO, proto=TFlagsInfo)

        def get_str_flag(key):
            return fi_response.VoiceFlags.Storage[key].String

        assert fi_response.ExpConfigVersion == 1028186
        assert fi_response.RequestId == "over9000"
        assert fi_response.ExpBoxes == "384379,0,-1"
        assert get_str_flag("UaasVinsUrl_aHR0cDovL21lZ2FtaW5kLXJjLmFsaWNlLnlhbmRleC5uZXQ=") == "1"
        assert get_str_flag("zero_testing_code=43929") == "1"
        assert fi_response.AsrFlagsJson == '{"ASR":{"boxes":"384379,0,-1","flags":["asr_flag"]}}'
        assert not fi_response.HasField("BioFlagsJson")

        fj_log_directive = get_answer_item(response, ItemTypes.UNIPROXY2_DIRECTIVE, proto=TUniproxyDirective)
        assert fj_log_directive.SessionLog.Name == "Directive"
        assert fj_log_directive.SessionLog.Action == "log_flags_json"
        assert json.loads(fj_log_directive.SessionLog.Value) == {"type": "FlagsJson", "Body": {"test_ids": ["384379"]}}

    def test_flags_info_neok(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_POST,
            create_grpc_request(
                items=[
                    {"type": ItemTypes.FLAGS_JSON_HTTP_RESPONSE, "data": {}},
                    {"type": ItemTypes.FLAGS_JSON_HTTP_RESPONSE, "data": {}},
                ]
            ),
        )

        assert len(response.Answers) == 1
        cl_response = get_answer_item(response, ItemTypes.CONTEXT_LOAD_RESPONSE, proto=TContextLoadResponse)
        assert cl_response.HasField("FlagsJsonResponse")

        assert get_answer_item(response, ItemTypes.FLAGS_INFO, proto=TFlagsInfo, noexcept=True) is None

    @pytest.mark.parametrize(
        "datasync_response_code, datasync_response_expected",
        [
            (HTTPStatus.OK, True),
            (HTTPStatus.NO_CONTENT, True),
            (HTTPStatus.NOT_FOUND, False),
            (HTTPStatus.INTERNAL_SERVER_ERROR, False),
        ],
    )
    def test_datasync_response_without_cache(
        self, cuttlefish: Cuttlefish, datasync_response_code, datasync_response_expected
    ):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.DATASYNC_HTTP_RESPONSE,
                        "data": THttpResponse(StatusCode=datasync_response_code, Content=b"TEST_CONTENT"),
                    }
                ]
            ),
        )

        context_load_response = get_answer_item(response, ItemTypes.CONTEXT_LOAD_RESPONSE, proto=TContextLoadResponse)

        if datasync_response_expected:
            assert context_load_response.HasField("DatasyncResponse")
            assert context_load_response.DatasyncResponse.StatusCode == datasync_response_code
            assert context_load_response.DatasyncResponse.Content == b"TEST_CONTENT"
        else:
            assert not context_load_response.HasField("DatasyncResponse")

    @pytest.mark.parametrize(
        "datasync_response_code, cache_response_status, expected_content",
        [
            (HTTPStatus.OK, EResponseStatus.OK, b"from datasync"),
            (HTTPStatus.OK, EResponseStatus.NO_CONTENT, b"from datasync"),
            (HTTPStatus.OK, EResponseStatus.NOT_FOUND, b"from datasync"),
            (HTTPStatus.OK, EResponseStatus.INTERNAL_ERROR, b"from datasync"),
            (HTTPStatus.NO_CONTENT, EResponseStatus.OK, b"from datasync"),
            (HTTPStatus.NO_CONTENT, EResponseStatus.NO_CONTENT, b"from datasync"),
            (HTTPStatus.NO_CONTENT, EResponseStatus.NOT_FOUND, b"from datasync"),
            (HTTPStatus.NO_CONTENT, EResponseStatus.INTERNAL_ERROR, b"from datasync"),
            (HTTPStatus.NOT_FOUND, EResponseStatus.OK, b"from cache"),
            (HTTPStatus.NOT_FOUND, EResponseStatus.NO_CONTENT, None),
            (HTTPStatus.NOT_FOUND, EResponseStatus.NOT_FOUND, None),
            (HTTPStatus.NOT_FOUND, EResponseStatus.INTERNAL_ERROR, None),
            (HTTPStatus.INTERNAL_SERVER_ERROR, EResponseStatus.OK, b"from cache"),
            (HTTPStatus.INTERNAL_SERVER_ERROR, EResponseStatus.NO_CONTENT, None),
            (HTTPStatus.INTERNAL_SERVER_ERROR, EResponseStatus.NOT_FOUND, None),
            (HTTPStatus.INTERNAL_SERVER_ERROR, EResponseStatus.INTERNAL_ERROR, None),
        ],
    )
    def test_datasync_response_with_cache(
        self, cuttlefish: Cuttlefish, datasync_response_code, cache_response_status, expected_content
    ):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.DATASYNC_HTTP_RESPONSE,
                        "data": THttpResponse(StatusCode=datasync_response_code, Content=b"from datasync"),
                    },
                    {
                        "type": ItemTypes.DATASYNC_CACHE_GET_RESPONSE,
                        "data": TCachalotResponse(
                            Status=cache_response_status,
                            GetResp=TCachalotGetResponse(Key="CACHE_KEY", Data=b"from cache"),
                        ),
                    },
                ]
            ),
        )

        context_load_response = get_answer_item(response, ItemTypes.CONTEXT_LOAD_RESPONSE, proto=TContextLoadResponse)

        if expected_content is None:
            assert not context_load_response.HasField("DatasyncResponse")
        else:
            assert context_load_response.HasField("DatasyncResponse")
            assert context_load_response.DatasyncResponse.Content == expected_content

    @pytest.mark.parametrize(
        "datasync_response_code, cache_response_status, should_cache",
        [
            (HTTPStatus.OK, EResponseStatus.OK, True),
            (HTTPStatus.OK, EResponseStatus.NO_CONTENT, True),
            (HTTPStatus.OK, EResponseStatus.INTERNAL_ERROR, True),
            (HTTPStatus.NO_CONTENT, EResponseStatus.OK, True),
            (HTTPStatus.NO_CONTENT, EResponseStatus.NO_CONTENT, True),
            (HTTPStatus.NO_CONTENT, EResponseStatus.INTERNAL_ERROR, True),
            (HTTPStatus.NOT_FOUND, EResponseStatus.OK, False),
            (HTTPStatus.NOT_FOUND, EResponseStatus.NO_CONTENT, False),
            (HTTPStatus.NOT_FOUND, EResponseStatus.INTERNAL_ERROR, False),
            (HTTPStatus.INTERNAL_SERVER_ERROR, EResponseStatus.OK, False),
            (HTTPStatus.INTERNAL_SERVER_ERROR, EResponseStatus.NO_CONTENT, False),
            (HTTPStatus.INTERNAL_SERVER_ERROR, EResponseStatus.INTERNAL_ERROR, False),
        ],
    )
    def test_set_cache_from_datasync(
        self, cuttlefish: Cuttlefish, datasync_response_code, cache_response_status, should_cache
    ):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.DATASYNC_HTTP_RESPONSE,
                        "data": THttpResponse(StatusCode=datasync_response_code, Content=b"from datasync"),
                    },
                    {
                        "type": ItemTypes.DATASYNC_CACHE_GET_RESPONSE,
                        "data": TCachalotResponse(
                            Status=cache_response_status,
                            GetResp=TCachalotGetResponse(
                                Key="46CBC9EF420DE000D67B8396AE693E9948018A21B48D027CE9D01E91D26FB193",
                                Data=b"from cache",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(
                                AuthToken="my-lovely-auth-token",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            SettingsFromManager=TManagedSettings(
                                UseDatasyncCache=True,
                            ),
                        ),
                    },
                ]
            ),
        )

        datasync_cache_set_req = get_answer_item(
            response, ItemTypes.DATASYNC_CACHE_SET_REQUEST, proto=TCachalotSetRequest, noexcept=True
        )

        if should_cache:
            assert datasync_cache_set_req is not None
            assert datasync_cache_set_req.Key == "46CBC9EF420DE000D67B8396AE693E9948018A21B48D027CE9D01E91D26FB193"
            assert datasync_cache_set_req.StorageTag == "Datasync"
            assert datasync_cache_set_req.Data == b"from datasync"
        else:
            assert datasync_cache_set_req is None

    @pytest.mark.parametrize(
        "has_auth_token, use_datasync_cache",
        [
            (True, True),
            (True, False),
            (False, True),
            (False, False),
        ],
    )
    def test_datasync_set_datasync_cache(self, cuttlefish: Cuttlefish, has_auth_token, use_datasync_cache):
        items = [
            {
                "type": ItemTypes.DATASYNC_HTTP_RESPONSE,
                "data": THttpResponse(StatusCode=HTTPStatus.OK, Content=b"from datasync"),
            }
        ]

        if use_datasync_cache:
            items.append(
                {
                    "type": ItemTypes.DATASYNC_CACHE_GET_RESPONSE,
                    "data": TCachalotResponse(
                        Status=HTTPStatus.NO_CONTENT,
                        GetResp=TCachalotGetResponse(
                            Key="46CBC9EF420DE000D67B8396AE693E9948018A21B48D027CE9D01E91D26FB193", Data=b"from cache"
                        ),
                    ),
                }
            )

        if has_auth_token:
            items.append(
                {
                    "type": ItemTypes.SESSION_CONTEXT,
                    "data": TSessionContext(
                        UserInfo=TUserInfo(
                            AuthToken="my-lovely-auth-token",
                        ),
                    ),
                }
            )

        response = cuttlefish.make_grpc_request(ServiceHandles.CONTEXT_LOAD_POST, create_grpc_request(items))
        datasync_cache_set_req = get_answer_item(
            response, ItemTypes.DATASYNC_CACHE_SET_REQUEST, proto=TCachalotSetRequest, noexcept=True
        )

        if has_auth_token:
            assert datasync_cache_set_req is not None
        else:
            assert datasync_cache_set_req is None

    @pytest.mark.parametrize(
        "cachalot_response_status", [EResponseStatus.OK, EResponseStatus.NO_CONTENT, EResponseStatus.BAD_REQUEST]
    )
    def test_cachalot_load_asr_options_patch_response(self, cuttlefish: Cuttlefish, cachalot_response_status):
        patch_asr_options_for_next_request_directive = TPatchAsrOptionsForNextRequestDirective(
            AdvancedASROptionsPatch=TPatchAsrOptionsForNextRequestDirective.TAdvancedASROptionsPatch(
                MaxSilenceDurationMS=Int32Value(
                    value=4000,
                ),
                EnableSidespeechDetector=BoolValue(
                    value=True,
                ),
                EouThreshold=FloatValue(
                    value=2.3,
                ),
                InitialMaxSilenceDurationMS=Int32Value(
                    value=10000,
                ),
            ),
        )

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.CACHALOT_LOAD_ASR_OPTIONS_PATCH_RESPONSE,
                        "data": TCachalotResponse(
                            Status=cachalot_response_status,
                            GetResp=TCachalotGetResponse(
                                Key="some_key",
                                Data=patch_asr_options_for_next_request_directive.SerializeToString(),
                            ),
                        ),
                    }
                ]
            ),
        )

        assert len(response.Answers) == 1
        response = get_answer_item(response, ItemTypes.CONTEXT_LOAD_RESPONSE, proto=TContextLoadResponse)

        if cachalot_response_status != EResponseStatus.OK:
            assert len(response.ListFields()) == 0
        else:
            assert len(response.ListFields()) == 1
            assert response.PatchAsrOptionsForNextRequestDirective == patch_asr_options_for_next_request_directive


class TestContextLoadPrepareLaas:
    SESSION_CONTEXT_1 = TSessionContext(
        UserInfo=TUserInfo(
            Uuid="my-lovely-uuid",
            Yuid="my-lovely-yuid",
        ),
        ConnectionInfo=TConnectionInfo(
            IpAddress="10.20.30.40",
        ),
        ClientType=TSessionContext.EClientType.CLIENT_TYPE_QUASAR,
        DeviceInfo=TDeviceInfo(
            DeviceId="my_new_station",
            WifiNetworks=[
                TDeviceInfo.TWifiNetwork(Mac="12:34:56::78", SignalStrength=90),
            ],
        ),
    )

    SESSION_CONTEXT_2 = TSessionContext(
        UserInfo=TUserInfo(
            Uuid="my-lovely-uuid",
        ),
        ConnectionInfo=TConnectionInfo(
            IpAddress="10.20.30.40",
        ),
    )

    BB_OK = THttpResponse(
        StatusCode=200,
        Content=json.dumps(
            {
                "status": {"value": "VALID"},
                "user_ticket": "my-lovely-user-ticket",
                "oauth": {"uid": "9753124680"},
                "aliases": {"13": "KingOfYandex"},
            }
        ).encode("utf-8"),
    )

    BB_ERROR = THttpResponse(
        StatusCode=200,
        Content=json.dumps(
            {
                "exception": {
                    "value": "INVALID_PARAMS",
                    "id": 2,
                },
                "error": "BlackBox error: param 'get_user_ticket' is allowed only with header 'X-Ya-Service-Ticket'.",
            }
        ).encode("utf-8"),
    )

    IOT_OK = TIoTUserInfo(
        Devices=[
            TIoTUserInfo.TDevice(
                QuasarInfo=TIoTUserInfo.TDevice.TQuasarInfo(DeviceId="my_new_station"),
                HouseholdId="my_house_built_from_bricks",
                SkillId="Q",
            ),
            TIoTUserInfo.TDevice(
                QuasarInfo=TIoTUserInfo.TDevice.TQuasarInfo(DeviceId="my_freezer"),
                HouseholdId="my_house_built_from_bricks",
                SkillId="F",
            ),
        ],
        Households=[
            TIoTUserInfo.THousehold(Id="my_house_built_from_bricks", Longitude=28, Latitude=0.8),
        ],
    )

    IOT_ERROR = TIoTUserInfo()

    PREDEFINED_IOT_OK = {
        "has_predefined_iot_config": True,
        "serialized_iot_config": base64.standard_b64encode(
            TIoTUserInfo(
                Devices=[
                    TIoTUserInfo.TDevice(
                        QuasarInfo=TIoTUserInfo.TDevice.TQuasarInfo(DeviceId="my_new_station"),
                        HouseholdId="my_apartment",
                        SkillId="Q",
                    ),
                    TIoTUserInfo.TDevice(
                        QuasarInfo=TIoTUserInfo.TDevice.TQuasarInfo(DeviceId="my_freezer"),
                        HouseholdId="my_apartment_without_station",
                        SkillId="Q",
                    ),
                ],
                Households=[
                    TIoTUserInfo.THousehold(Id="my_apartment", Longitude=22, Latitude=0.7),
                    TIoTUserInfo.THousehold(Id="my_apartment_without_station", Longitude=14, Latitude=0.4),
                ],
            ).SerializeToString()
        ).decode("ascii"),
    }

    PREDEFINED_IOT_EMPTY = {
        "has_predefined_iot_config": True,
    }

    LAAS_OPTIONS = TContextLoadLaasRequestOptions(UseCoordinatesFromIoT=True)

    @pytest.mark.parametrize(
        "session_ctx, bb_response, iot_response, predefined_iot, laas_opts",
        itertools.product(
            (SESSION_CONTEXT_1, SESSION_CONTEXT_2),
            (BB_OK, BB_ERROR, None),
            (IOT_OK, IOT_ERROR, None),
            (PREDEFINED_IOT_OK, PREDEFINED_IOT_EMPTY, None),
            (LAAS_OPTIONS, None),
        ),
    )
    def test_all(self, cuttlefish: Cuttlefish, session_ctx, bb_response, iot_response, predefined_iot, laas_opts):
        input_items = [
            {
                "type": ItemTypes.SESSION_CONTEXT,
                "data": session_ctx,
            },
        ]

        if bb_response:
            input_items.append(
                {
                    "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                    "data": bb_response,
                }
            )

        if iot_response:
            input_items.append(
                {
                    "type": ItemTypes.QUASARIOT_RESPONSE_IOT_USER_INFO,
                    "data": iot_response,
                }
            )

        if predefined_iot:
            input_items.append(
                {
                    "type": ItemTypes.PREDEFINED_IOT_CONFIG,
                    "data": predefined_iot,
                }
            )

        if laas_opts:
            input_items.append(
                {
                    "type": ItemTypes.LAAS_REQUEST_OPTIONS,
                    "data": laas_opts,
                }
            )

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PREPARE_LAAS, create_grpc_request(items=input_items)
        )

        assert len(response.Answers) == 1

        laas_request = get_answer_item(response, ItemTypes.LAAS_HTTP_REQUEST, proto=THttpRequest)
        logging.info(f"Laas request: {laas_request}")

        assert laas_request.Method == THttpRequest.Get
        assert laas_request.Scheme == THttpRequest.Http

        path = laas_request.Path
        path = assert_starts_with(path, "/region?real-ip=10.20.30.40&uuid=my-lovely-uuid")

        if session_ctx.ClientType == TSessionContext.EClientType.CLIENT_TYPE_QUASAR:
            path = assert_starts_with(path, "&service=quasar")

        if bb_response and (b"INVALID" not in bb_response.Content):
            path = assert_starts_with(path, "&puid=9753124680")

        if len(session_ctx.DeviceInfo.WifiNetworks) > 0:
            path = assert_starts_with(path, "&wifinetworks=12345678:90")

        # https://st.yandex-team.ru/VOICESERV-3682
        if laas_opts and laas_opts.UseCoordinatesFromIoT and session_ctx.DeviceInfo.HasField("DeviceId"):
            if predefined_iot and ("serialized_iot_config" in predefined_iot):
                path = assert_starts_with(path, "&lat=0.7&lon=22&location_accuracy=1&location_recency=0")
            elif iot_response and (len(iot_response.Households) > 0):
                path = assert_starts_with(path, "&lat=0.8&lon=28&location_accuracy=1&location_recency=0")

        if session_ctx.UserInfo.HasField("Yuid"):
            assert len(laas_request.Headers) == 1
            assert find_header(laas_request, "Cookie") == "yandexuid=my-lovely-yuid"
        else:
            assert len(laas_request.Headers) == 0


class TestContextLoadPrepareFlagsJson:
    SESSION_CONTEXT_1 = TSessionContext(
        UserInfo=TUserInfo(
            Uuid="my-lovely-uuid",
            Yuid="my-lovely-yuid",
        ),
        DeviceInfo=TDeviceInfo(
            DeviceId="dell_102",
        ),
    )

    BB_OK_STAFF = THttpResponse(
        StatusCode=200,
        Content=json.dumps(
            {
                "status": {"value": "VALID"},
                "user_ticket": "my-lovely-user-ticket",
                "oauth": {"uid": "9753124680"},
                "aliases": {"13": "KingOfYandex"},
            }
        ).encode("utf-8"),
    )

    AB_OPTS_1 = TAbFlagsProviderOptions(
        TestIds=["384", "379"],
        DisregardUaas=False,
        Only100PercentFlags=True,
    )

    AB_OPTS_2 = TAbFlagsProviderOptions(
        DisregardUaas=True,
    )

    LAAS_OK = THttpResponse(
        StatusCode=200,
        Content=json.dumps(
            {
                "region_id": 9472003,
            }
        ).encode("utf-8"),
    )

    @pytest.mark.parametrize(
        "session_ctx, bb_response, ab_options, laas_response",
        itertools.product((SESSION_CONTEXT_1,), (BB_OK_STAFF, None), (AB_OPTS_1, AB_OPTS_2), (LAAS_OK,)),
    )
    def test_all(self, cuttlefish: Cuttlefish, session_ctx, bb_response, ab_options, laas_response):
        input_items = [
            {
                "type": ItemTypes.SESSION_CONTEXT,
                "data": session_ctx,
            },
        ]

        if bb_response:
            input_items.append(
                {
                    "type": ItemTypes.BLACKBOX_HTTP_RESPONSE,
                    "data": bb_response,
                }
            )

        if ab_options:
            input_items.append(
                {
                    "type": ItemTypes.AB_EXPERIMENTS_OPTIONS,
                    "data": ab_options,
                }
            )

        if laas_response:
            input_items.append(
                {
                    "type": ItemTypes.LAAS_HTTP_RESPONSE,
                    "data": laas_response,
                }
            )

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_LOAD_PREPARE_FLAGS_JSON, create_grpc_request(items=input_items)
        )

        assert len(response.Answers) == 1
        flags_json_req = get_answer_item(response, ItemTypes.FLAGS_JSON_HTTP_REQUEST, noexcept=True)

        if ab_options and ab_options.DisregardUaas:
            assert flags_json_req is None
            return

        flags_json_req = extract_json(flags_json_req)

        logging.info(f"flags_json request: {flags_json_req}")

        assert flags_json_req.get('method') == 'GET'

        path = flags_json_req.get('uri')
        path = assert_starts_with(path, "/uniproxy?uuid=my-lovely-uuid&deviceid=dell_102")

        if ab_options and ab_options.Only100PercentFlags:
            path = assert_starts_with(path, "&no-tests=1")

        if ab_options and ab_options.TestIds:
            path = assert_starts_with(path, "&test-id=" + '_'.join(ab_options.TestIds))

        # all cgi-params are checked
        assert path == ""

        headers = flags_json_req.get('headers')

        assert find_header_json(headers, "User-Agent") == "uniproxy"

        if bb_response:
            assert find_header_json(headers, "X-Yandex-Puid") == "9753124680"
            assert find_header_json(headers, "X-Ip-Properties")

        if laas_response:
            assert find_header_json(headers, "X-Region-City-Id") == "9472003"
