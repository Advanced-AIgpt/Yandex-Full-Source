import base64
import json
import logging
import pytest
from .common import Cuttlefish, create_grpc_request
from .common.utils import find_header

from alice.cuttlefish.library.protos.audio_separator_pb2 import TFullIncomingAudio
from alice.cachalot.api.protos.cachalot_pb2 import (
    TSetRequest as TCachalotSetRequest,
    TDeleteRequest as TCachalotDeleteRequest,
    TResponse as TCachalotResponse,
    EResponseStatus,
)
from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadBlackboxUid
from alice.cuttlefish.library.protos.context_save_pb2 import (
    TContextSaveRequest,
    TContextSavePreRequestsInfo,
    TContextSaveResponse,
)
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TUserInfo, TRequestContext
from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.python.testing.items import get_answer_item

from alice.megamind.protos.scenarios.directives_pb2 import TPatchAsrOptionsForNextRequestDirective
from alice.megamind.protos.speechkit.directives_pb2 import TDirective as TSpeechkitDirective
from alice.megamind.protos.speechkit.directives_pb2 import TProtobufUniproxyDirective

from alice.protos.api.matrix.delivery_pb2 import TDelivery
from alice.protos.api.matrix.schedule_action_pb2 import TScheduleAction

from alice.uniproxy.library.protos.notificator_pb2 import (
    TManageSubscription,
    TNotificationChangeStatus,
    TSupMessage,
    ENotificationRead,
)

from apphost.lib.proto_answers.http_pb2 import THttpRequest, THttpResponse
from apphost.lib.proto_answers.tvm_user_ticket_pb2 import TTvmUserTicket

import alice.memento.proto.api_pb2 as memento_pb

from voicetech.library.settings_manager.proto.settings_pb2 import (
    TManagedSettings,
)

from google.protobuf import json_format
from google.protobuf.any_pb2 import Any as google_protobuf_Any
from google.protobuf.struct_pb2 import Struct as google_protobuf_Struct


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish(
        env={
            "AWS_ACCESS_KEY_ID": "aws_access_key",
            "AWS_SECRET_ACCESS_KEY": "aws_secret_access_key",
        },
    ) as x:
        yield x


# -------------------------------------------------------------------------------------------------
def make_speechkit_directive(name, payload=None):
    directive = TSpeechkitDirective()
    directive.Name = name
    if payload:
        directive.Payload.MergeFrom(payload)
    return directive


def check_context_save_pre_requests_info(response, request_created_for_directives, failed_directives):
    context_save_pre_requests_info = get_answer_item(
        response, ItemTypes.CONTEXT_SAVE_PRE_REQUESTS_INFO, proto=TContextSavePreRequestsInfo
    )
    assert context_save_pre_requests_info.RequestCreatedForDirectives == request_created_for_directives
    assert context_save_pre_requests_info.FailedDirectives == failed_directives


class TestContextSavePre:
    def test_empty(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(),
                    },
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 1
        assert get_answer_item(response, ItemTypes.NOTHING)

    def test_datasync(self, cuttlefish: Cuttlefish):
        payload = google_protobuf_Struct()
        payload["key"] = "my-lovely-key"
        payload["value"] = "my-lovely-value"
        payload["method"] = "my-lovely-method"

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(),
                    },
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                    {
                        "type": ItemTypes.BLACKBOX_UID,
                        "data": TContextLoadBlackboxUid(Uid="my-uid"),
                    },
                    {
                        "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                        "data": TContextSaveRequest(
                            Directives=[
                                make_speechkit_directive(name="update_datasync", payload=payload),
                            ]
                        ),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 2
        check_context_save_pre_requests_info(response, ["update_datasync"], [])

        datasync_http_req = get_answer_item(response, ItemTypes.DATASYNC_HTTP_REQUEST, proto=THttpRequest)
        logging.info(f"Datasync request: {datasync_http_req}")

        assert datasync_http_req.Path == "/v1/batch/request"
        assert (
            datasync_http_req.Content.decode("ascii")
            == "{\"items\":[{\"method\":\"my-lovely-method\",\"relative_url\":\"my-lovely-key\",\"body\":\"\\\"my-lovely-value\\\"\"}]}"
        )

        assert len(datasync_http_req.Headers) == 3
        assert find_header(datasync_http_req, "Content-Type") == "application/json; charset=utf-8"
        assert find_header(datasync_http_req, "X-Ya-User-Ticket") == "my-lovely-user-ticket"
        assert find_header(datasync_http_req, "X-Uid") == "my-uid"

    @pytest.mark.parametrize(
        "datasync_update_directive_received, datasync_cache_enabled, cache_invalidation_expected",
        [
            (True, True, True),
            (True, False, False),
            (False, True, False),
            (False, False, False),
        ],
    )
    def test_datasync_cache_invalidation(
        self,
        cuttlefish: Cuttlefish,
        datasync_update_directive_received,
        datasync_cache_enabled,
        cache_invalidation_expected,
    ):
        context_save_request = TContextSaveRequest()
        if datasync_update_directive_received:
            payload = google_protobuf_Struct()
            payload["key"] = "my-lovely-key"
            payload["value"] = "my-lovely-value"
            payload["method"] = "my-lovely-method"
            context_save_request.Directives.append(make_speechkit_directive(name="update_datasync", payload=payload))

        items = [
            {
                "type": ItemTypes.REQUEST_CONTEXT,
                "data": TRequestContext(
                    SettingsFromManager=TManagedSettings(
                        UseDatasyncCache=datasync_cache_enabled,
                    ),
                ),
            },
            {
                "type": ItemTypes.SESSION_CONTEXT,
                "data": TSessionContext(UserInfo=TUserInfo(AuthToken="TEST_AUTH_TOKEN")),
            },
            {
                "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                "data": context_save_request,
            },
            {
                "type": ItemTypes.TVM_USER_TICKET,
                "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
            },
        ]

        response = cuttlefish.make_grpc_request(ServiceHandles.CONTEXT_SAVE_PRE, create_grpc_request(items))

        if cache_invalidation_expected:
            datasync_cache_delete_req = get_answer_item(
                response, ItemTypes.DATASYNC_CACHE_DELETE_REQUEST, proto=TCachalotDeleteRequest
            )
            logging.info(f"datasync_cache_delete_request: {datasync_cache_delete_req}")

            assert datasync_cache_delete_req.Key == "9DF52458B8AC5A86384B176A0E5A6E059A85EB5421799600DE6893A445D537A4"
            assert datasync_cache_delete_req.StorageTag == "Datasync"
        else:
            assert not get_answer_item(
                response, ItemTypes.DATASYNC_CACHE_DELETE_REQUEST, proto=TCachalotDeleteRequest, noexcept=True
            )

    def test_memento(self, cuttlefish: Cuttlefish):
        req_ctx = TRequestContext(
            Header=TRequestContext.THeader(SessionId="cool-session-id", MessageId="cool-message-id")
        )
        req_ctx.ExpFlags.update({"use_memento": "1"})

        def make_any(key, value):
            struct = google_protobuf_Struct()
            struct[key] = value
            elem = google_protobuf_Any()
            elem.Pack(struct)
            return elem

        def parse_any(obj):
            struct = google_protobuf_Struct()
            obj.Unpack(struct)
            assert len(struct) == 1
            for key, value in struct.items():
                return key, value

        def make_req_part(i):
            # fill TReqChangeUserObjects
            req = memento_pb.TReqChangeUserObjects()

            config_keys = [
                memento_pb.EConfigKey.CK_NEWS,
                memento_pb.EConfigKey.CK_MORNING_SHOW,
                memento_pb.EConfigKey.CK_MORNING_SHOW_TOPICS,
            ]
            req.UserConfigs.append(memento_pb.TConfigKeyAnyPair(Key=config_keys[i - 1]))

            req.ScenarioData.get_or_create(f"leaf{i % 2}").MergeFrom(make_any(f"key{i}", f"value{i}"))

            req.SurfaceScenarioData.get_or_create(f"branch{(i + 1) % 2}").MergeFrom(
                memento_pb.TSurfaceScenarioData(ScenarioData={"aba": make_any(f"aba{i}", f"caba{i}")})
            )

            req.CurrentSurfaceId = f"1234{i}"

            # fill directive payload
            payload = google_protobuf_Struct()
            payload["user_objects"] = base64.b64encode(req.SerializeToString()).decode("ascii")
            return payload

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": req_ctx,
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(),
                    },
                    {
                        "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                        "data": TContextSaveRequest(
                            Directives=[
                                make_speechkit_directive(name="update_memento", payload=make_req_part(1)),
                                make_speechkit_directive(name="update_memento", payload=make_req_part(2)),
                                make_speechkit_directive(name="update_memento", payload=make_req_part(3)),
                            ]
                        ),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 2
        check_context_save_pre_requests_info(response, ["update_memento"], [])

        memento_http_req = get_answer_item(response, ItemTypes.MEMENTO_HTTP_REQUEST, proto=THttpRequest)
        logging.info(f"Memento request: {memento_http_req}")

        assert memento_http_req.Path == "/memento/update_objects"
        assert len(memento_http_req.Headers) == 2
        assert find_header(memento_http_req, "Content-Type") == "application/protobuf"
        assert find_header(memento_http_req, "X-Ya-User-Ticket") == "my-lovely-user-ticket"

        proto_req = memento_pb.TReqChangeUserObjects()
        assert proto_req.MergeFromString(memento_http_req.Content)
        assert proto_req.CurrentSurfaceId == "12343"
        assert len(proto_req.UserConfigs) == 3
        assert proto_req.UserConfigs[0].Key == memento_pb.EConfigKey.CK_NEWS
        assert proto_req.UserConfigs[1].Key == memento_pb.EConfigKey.CK_MORNING_SHOW
        assert proto_req.UserConfigs[2].Key == memento_pb.EConfigKey.CK_MORNING_SHOW_TOPICS
        assert parse_any(proto_req.ScenarioData["leaf0"]) == ("key2", "value2")
        assert parse_any(proto_req.ScenarioData["leaf1"]) == ("key3", "value3")
        assert len(proto_req.SurfaceScenarioData["branch0"].ScenarioData) == 1
        assert len(proto_req.SurfaceScenarioData["branch1"].ScenarioData) == 1
        assert parse_any(proto_req.SurfaceScenarioData["branch0"].ScenarioData["aba"]) == ("aba3", "caba3")
        assert parse_any(proto_req.SurfaceScenarioData["branch1"].ScenarioData["aba"]) == ("aba2", "caba2")

    def test_notificator(self, cuttlefish: Cuttlefish):
        payload = google_protobuf_Struct()
        payload["subscription_id"] = 1234567
        payload["unsubscribe"] = True

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(Puid="my-lovely-puid"),
                            AppId="megadevice",
                        ),
                    },
                    {
                        "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                        "data": TContextSaveRequest(
                            Directives=[
                                make_speechkit_directive(name="update_notification_subscription", payload=payload),
                            ]
                        ),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 2
        check_context_save_pre_requests_info(response, ["update_notification_subscription"], [])

        notificator_http_req = get_answer_item(
            response, ItemTypes.NOTIFICATOR_SUBSCRIPTION_HTTP_REQUEST, proto=THttpRequest
        )
        logging.info(f"Notificator request: {notificator_http_req}")

        assert notificator_http_req.Path == "/subscriptions/manage"
        msg = TManageSubscription()
        msg.MergeFromString(notificator_http_req.Content)
        assert msg.SubscriptionId == 1234567
        assert msg.Method == TManageSubscription.EUnsubscribe
        assert msg.Puid == "my-lovely-puid"
        assert msg.AppId == "megadevice"

        assert len(notificator_http_req.Headers) == 1
        assert find_header(notificator_http_req, "Content-Type") == "application/protobuf"

    def test_notificator_mark_as_read(self, cuttlefish: Cuttlefish):
        payload = google_protobuf_Struct()
        payload["notification_id"] = "extra_id"

        main_ids = payload.get_or_create_list("notification_ids")
        main_ids.append("id1")
        main_ids.append("id2")
        main_ids.append("id3")

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(Puid="my-lovely-puid"),
                            AppId="megadevice",
                        ),
                    },
                    {
                        "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                        "data": TContextSaveRequest(
                            Directives=[
                                make_speechkit_directive(name="mark_notification_as_read", payload=payload),
                            ]
                        ),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 2
        check_context_save_pre_requests_info(response, ["mark_notification_as_read"], [])

        notificator_http_req = get_answer_item(
            response, ItemTypes.NOTIFICATOR_MARK_AS_READ_HTTP_REQUEST, proto=THttpRequest
        )
        logging.info(f"Notificator (mark as read) request: {notificator_http_req}")

        assert notificator_http_req.Path == "/notifications/change_status"
        msg = TNotificationChangeStatus()
        msg.MergeFromString(notificator_http_req.Content)
        assert msg.NotificationIds == ["id1", "id2", "id3", "extra_id"]
        assert msg.Status == ENotificationRead
        assert msg.Puid == "my-lovely-puid"
        assert msg.AppId == "megadevice"

        assert len(notificator_http_req.Headers) == 1
        assert find_header(notificator_http_req, "Content-Type") == "application/protobuf"

    def test_notificator_send_sup_push(self, cuttlefish: Cuttlefish):
        payload = google_protobuf_Struct()
        payload["title"] = "some-nice-title"
        payload["body"] = "some-nice-body"
        payload["link"] = "links-2-3-4"

        req_ctx = TRequestContext(
            Header=TRequestContext.THeader(SessionId="cool-session-id", MessageId="cool-message-id")
        )
        req_ctx.ExpFlags.update(
            {
                "rubbish=nothing": "1",
                "notificator_sup_test_iddddd=what": "1",
                "notificator_sup_test_id=some-cool-id": "1",
            }
        )

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": req_ctx,
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(Puid="my-lovely-puid"),
                            AppId="megadevice",
                        ),
                    },
                    {
                        "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                        "data": TContextSaveRequest(
                            Directives=[
                                make_speechkit_directive(name="push_message", payload=payload),
                            ]
                        ),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 2
        check_context_save_pre_requests_info(response, ["push_message"], [])

        notificator_http_req = get_answer_item(
            response, ItemTypes.NOTIFICATOR_SEND_SUP_PUSH_HTTP_REQUEST, proto=THttpRequest
        )
        logging.info(f"Notificator (send sup push) request: {notificator_http_req}")

        assert notificator_http_req.Path == "/delivery/sup"
        msg = TSupMessage()
        msg.MergeFromString(notificator_http_req.Content)

        assert msg.PushMsg.Title == "some-nice-title"
        assert msg.PushMsg.Body == "some-nice-body"
        assert msg.PushMsg.Link == "links-2-3-4"

        assert msg.Puid == "my-lovely-puid"
        assert msg.AppId == "megadevice"
        assert msg.TestId == "some-cool-id"

        assert len(notificator_http_req.Headers) == 1
        assert find_header(notificator_http_req, "Content-Type") == "application/protobuf"

    def test_personal_cards_good(self, cuttlefish: Cuttlefish):
        json_payload = {
            "card": {
                "card_id": "ace-of-spades",
                "date_from": 123.456,
                "date_to": 789.10,
                "button_url": "http://azino-tri-topora.com/",
                "text": "Вложись в УралВагонЗавод, гарантия доходности 300%!",
                "yandex.station_film": {
                    "min_price": 300,
                },
            },
            "remove_existing_cards": True,
        }
        payload = json_format.Parse(json.dumps(json_payload), google_protobuf_Struct())

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(Puid="my-lovely-puid"),
                        ),
                    },
                    {
                        "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                        "data": TContextSaveRequest(
                            Directives=[
                                make_speechkit_directive(name="personal_cards", payload=payload),
                            ]
                        ),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 3
        check_context_save_pre_requests_info(response, ["personal_cards"], [])

        add_http_req = get_answer_item(response, ItemTypes.PERSONAL_CARDS_ADD_HTTP_REQUEST, proto=THttpRequest)
        logging.info(f"PersonalCards (add) request: {add_http_req}")
        assert add_http_req.Path == "/addPushCards"
        assert json.loads(add_http_req.Content.decode()) == {
            "card": {
                "card": {
                    "data": {
                        "min_price": 300,
                        "text": "Вложись в УралВагонЗавод, гарантия доходности 300%!",
                        "button_url": "http://azino-tri-topora.com/",
                    },
                    "type": "yandex.station_film",
                    "card_id": "ace-of-spades",
                    "date_to": 789.1,
                    "date_from": 123.456,
                },
            },
            "uid": "my-lovely-puid",
        }

        dismiss_http_req = get_answer_item(response, ItemTypes.PERSONAL_CARDS_DISMISS_HTTP_REQUEST, proto=THttpRequest)
        logging.info(f"PersonalCards (dismiss) request: {dismiss_http_req}")
        assert dismiss_http_req.Path == "/dismiss"
        assert json.loads(dismiss_http_req.Content.decode()) == {
            "card_id": "*",
            "auth": {
                "uid": "my-lovely-puid",
            },
        }

    def test_notificator_push_typed_semantic_frame_directive(self, cuttlefish: Cuttlefish):
        json_payload = {
            "puid": "13071999",
            "device_id": "MEGADEVICE_GOBLIN_3000",
            "ttl": 228,
            "semantic_frame_request_data": {
                "typed_semantic_frame": {
                    "weather_semantic_frame": {
                        "when": {
                            "datetime_value": "13:07:1999",
                        },
                    },
                },
                "analytics": {
                    "product_scenario": "Weather",
                    "origin": "Scenario",
                },
            },
        }
        payload = json_format.Parse(json.dumps(json_payload), google_protobuf_Struct())

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            Header=TRequestContext.THeader(SessionId="cool-session-id", MessageId="cool-message-id")
                        ),
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(Puid="my-lovely-puid"),
                            AppId="megadevice",
                        ),
                    },
                    {
                        "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                        "data": TContextSaveRequest(
                            Directives=[
                                make_speechkit_directive(name="push_typed_semantic_frame", payload=payload),
                            ]
                        ),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 2
        check_context_save_pre_requests_info(response, ["push_typed_semantic_frame"], [])

        notificator_http_req = get_answer_item(
            response, ItemTypes.NOTIFICATOR_PUSH_TYPED_SEMANTIC_FRAME_REQUEST, proto=THttpRequest
        )
        logging.info(f"Notificator (push typed semantic frame) request: {notificator_http_req}")

        assert notificator_http_req.Path == "/delivery/push"
        msg = TDelivery()
        msg.MergeFromString(notificator_http_req.Content)

        assert msg.Puid == '13071999'
        assert msg.DeviceId == 'MEGADEVICE_GOBLIN_3000'
        assert msg.Ttl == 228
        assert msg.SemanticFrameRequestData.TypedSemanticFrame.WeatherSemanticFrame.When.DateTimeValue == '13:07:1999'
        assert msg.SemanticFrameRequestData.Analytics.ProductScenario == 'Weather'
        assert msg.SemanticFrameRequestData.Analytics.Origin == 2  # code for "Scenario"

        assert len(notificator_http_req.Headers) == 1
        assert find_header(notificator_http_req, "Content-Type") == "application/protobuf"

    def test_matrix_scheduler_add_schedule_action_directive(self, cuttlefish: Cuttlefish):
        json_payload = {
            "schedule_action": {
                "Id": "delivery_action",
                "Puid": "339124070",
                "DeviceId": "MOCK_DEVICE_ID",
                "StartPolicy": {"StartAtTimestampMs": 123},
                "SendPolicy": {
                    "SendOncePolicy": {
                        "RetryPolicy": {
                            "MaxRetries": 1,
                            "RestartPeriodScaleMs": 200,
                            "RestartPeriodBackOff": 2,
                            "MinRestartPeriodMs": 10000,
                            "MaxRestartPeriodMs": 100000,
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
                                    "iot_broadcast_start": {"pairing_token": {"StringValue": "token"}}
                                },
                                "analytics": {"purpose": "video"},
                            },
                        }
                    }
                },
            }
        }
        payload = json_format.Parse(json.dumps(json_payload), google_protobuf_Struct())

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            Header=TRequestContext.THeader(SessionId="cool-session-id", MessageId="cool-message-id")
                        ),
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(Puid="my-lovely-puid"),
                            AppId="megadevice",
                        ),
                    },
                    {
                        "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                        "data": TContextSaveRequest(
                            Directives=[
                                make_speechkit_directive(name="add_schedule_action", payload=payload),
                            ]
                        ),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 2
        check_context_save_pre_requests_info(response, ["add_schedule_action"], [])

        matrix_scheduler_http_req = get_answer_item(
            response, ItemTypes.MATRIX_SCHEDULER_ADD_SCHEDULE_ACTION_REQUEST, proto=THttpRequest
        )
        logging.info(f"Matrix (add schedule action) request: {matrix_scheduler_http_req}")

        assert matrix_scheduler_http_req.Path == "/schedule"
        msg = TScheduleAction()
        msg.MergeFromString(matrix_scheduler_http_req.Content)

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
        assert (
            msg.Action.OldNotificatorRequest.Delivery.SemanticFrameRequestData.TypedSemanticFrame.IoTBroadcastStartSemanticFrame.PairingToken.StringValue
            == "token"
        )
        assert msg.Action.OldNotificatorRequest.Delivery.SemanticFrameRequestData.Analytics.Purpose == "video"

    @pytest.mark.parametrize(
        "parts_to_save, expected_audio_to_save",
        [
            ("SpotterPartOnly", b"spotter_part"),
            ("MainPartOnly", b"main_part"),
        ],
    )
    def test_save_user_audio_directive(self, cuttlefish: Cuttlefish, parts_to_save, expected_audio_to_save):
        json_payload = {
            "storage_options": {
                "s3_storage": {
                    "bucket": "my-super-bucket",
                    "path": "my-super-path",
                },
            },
            "parts_to_save": parts_to_save,
        }
        payload = json_format.Parse(json.dumps(json_payload), google_protobuf_Struct())

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            Header=TRequestContext.THeader(SessionId="cool-session-id", MessageId="cool-message-id")
                        ),
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(Puid="my-lovely-puid"),
                            AppId="megadevice",
                        ),
                    },
                    {
                        "type": ItemTypes.FULL_INCOMING_AUDIO,
                        "data": TFullIncomingAudio(
                            SpotterPart=b"spotter_part",
                            MainPart=b"main_part",
                        ),
                    },
                    {
                        "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                        "data": TContextSaveRequest(
                            Directives=[
                                make_speechkit_directive(name="save_user_audio", payload=payload),
                            ]
                        ),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 2
        check_context_save_pre_requests_info(response, ["save_user_audio"], [])

        s3_save_user_audio_http_request = get_answer_item(
            response, ItemTypes.S3_SAVE_USER_AUDIO_HTTP_REQUEST, proto=THttpRequest
        )
        logging.info(f"S3 save user audio http request: {s3_save_user_audio_http_request}")

        assert s3_save_user_audio_http_request.Method == THttpRequest.EMethod.Put
        assert s3_save_user_audio_http_request.Path == "/my-super-path"
        assert s3_save_user_audio_http_request.Content == expected_audio_to_save
        assert len(s3_save_user_audio_http_request.Headers) == 4
        # Current time is a part of signature
        # So we can't just canonize values of some headers
        assert find_header(s3_save_user_audio_http_request, "host") == "my-super-bucket.s3.mds.yandex.net"
        assert find_header(s3_save_user_audio_http_request, "x-amz-date") is not None
        assert find_header(s3_save_user_audio_http_request, "x-amz-content-sha256") == "UNSIGNED-PAYLOAD"
        assert "AWS4-HMAC-SHA256" in find_header(s3_save_user_audio_http_request, "authorization")

    @pytest.mark.parametrize(
        "error_type",
        ["no_full_incoming_audio", "duplicate", "full_incoming_audio_with_error", "bad_parts_to_save", "none"],
    )
    def test_save_user_audio_directive_failed(self, cuttlefish: Cuttlefish, error_type):
        json_payload = {
            "storage_options": {
                "s3_storage": {
                    "bucket": "my-super-bucket",
                    "path": "my-super-path",
                },
            },
            "parts_to_save": "unknown" if error_type == "bad_parts_to_save" else "MainPartOnly",
        }
        payload = json_format.Parse(json.dumps(json_payload), google_protobuf_Struct())
        directives = [
            make_speechkit_directive(name="save_user_audio", payload=payload),
        ]
        if error_type == "duplicate":
            directives.append(make_speechkit_directive(name="save_user_audio", payload=payload))

        items = [
            {
                "type": ItemTypes.TVM_USER_TICKET,
                "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
            },
            {
                "type": ItemTypes.REQUEST_CONTEXT,
                "data": TRequestContext(
                    Header=TRequestContext.THeader(SessionId="cool-session-id", MessageId="cool-message-id")
                ),
            },
            {
                "type": ItemTypes.SESSION_CONTEXT,
                "data": TSessionContext(
                    UserInfo=TUserInfo(Puid="my-lovely-puid"),
                    AppId="megadevice",
                ),
            },
            {
                "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                "data": TContextSaveRequest(Directives=directives),
            },
        ]

        if error_type != "no_full_incoming_audio":
            full_incoming_audio = TFullIncomingAudio(
                SpotterPart=b"spotter_part",
                MainPart=b"main_part",
            )

            if error_type == "full_incoming_audio_with_error":
                full_incoming_audio.ErrorMessage = "error"

            items.append(
                {
                    "type": ItemTypes.FULL_INCOMING_AUDIO,
                    "data": full_incoming_audio,
                }
            )

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=items,
            ),
        )

        if error_type not in ("none", "duplicate"):
            assert len(response.Answers) == 1
            check_context_save_pre_requests_info(response, [], ["save_user_audio"])
        else:
            assert len(response.Answers) == 2
            check_context_save_pre_requests_info(
                response, ["save_user_audio"], ["save_user_audio"] if error_type == "duplicate" else []
            )
            assert get_answer_item(response, ItemTypes.S3_SAVE_USER_AUDIO_HTTP_REQUEST, proto=THttpRequest)

    def test_patch_asr_options_for_next_request_directive(self, cuttlefish: Cuttlefish):
        json_payload = {
            "advanced_asr_options_patch": {
                "max_silence_duration_ms": 4000,
                "enable_sidespeech_detector": False,
                "eou_threshold": 2,
                "initial_max_silence_duration_ms": 10000,
            }
        }
        payload = json_format.Parse(json.dumps(json_payload), google_protobuf_Struct())

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            Header=TRequestContext.THeader(
                                SessionId="cool-session-id", MessageId="cool-message-id", ReqId="cool-request-id"
                            )
                        ),
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(Puid="my-lovely-puid"),
                            AppId="megadevice",
                        ),
                    },
                    {
                        "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                        "data": TContextSaveRequest(
                            Directives=[
                                make_speechkit_directive(name="patch_asr_options_for_next_request", payload=payload),
                            ]
                        ),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 2
        check_context_save_pre_requests_info(response, ["patch_asr_options_for_next_request"], [])

        cachalot_save_asr_options_patch_request = get_answer_item(
            response, ItemTypes.CACHALOT_SAVE_ASR_OPTIONS_PATCH_REQUEST, proto=TCachalotSetRequest
        )
        logging.info(f"Cachalot save asr options patch request: {cachalot_save_asr_options_patch_request}")

        assert cachalot_save_asr_options_patch_request.Key == "cool-request-id"
        assert cachalot_save_asr_options_patch_request.StorageTag == "AsrOptions"

        data = TPatchAsrOptionsForNextRequestDirective()
        data.ParseFromString(cachalot_save_asr_options_patch_request.Data)
        assert data.AdvancedASROptionsPatch.MaxSilenceDurationMS.value == 4000
        assert not data.AdvancedASROptionsPatch.EnableSidespeechDetector.value
        assert abs(data.AdvancedASROptionsPatch.EouThreshold.value - 2.0) < 10**-9
        assert data.AdvancedASROptionsPatch.InitialMaxSilenceDurationMS.value == 10000

    @pytest.mark.parametrize(
        "input_type, has_answer, empty_body",
        [
            ("my_lovely_request_type", True, False),
            ("", False, False),
            ("my_lovely_request_type", False, True),
        ],
    )
    def test_context_save_directive(self, cuttlefish: Cuttlefish, input_type: str, has_answer: bool, empty_body: bool):
        puid = 'my_lovely_puid'

        csd = TProtobufUniproxyDirective.TContextSaveDirective()
        csd.DirectiveId = input_type
        if not empty_body:
            csd.Payload.Pack(TUserInfo(Puid=puid))

        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.TVM_USER_TICKET,
                        "data": TTvmUserTicket(UserTicket="my-lovely-user-ticket"),
                    },
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            Header=TRequestContext.THeader(SessionId="cool-session-id", MessageId="cool-message-id")
                        ),
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(Puid="my-lovely-puid"),
                            AppId="megadevice",
                        ),
                    },
                    {
                        "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                        "data": TContextSaveRequest(ContextSaveDirectives=[csd]),
                    },
                ],
            ),
        )

        assert (
            len(response.Answers) == int(has_answer) + 1
        )  # +1 because CONTEXT_SAVE_PRE_REQUESTS_INFO is always present.

        preRequestInfo = get_answer_item(
            response, ItemTypes.CONTEXT_SAVE_PRE_REQUESTS_INFO, proto=TContextSavePreRequestsInfo
        )
        assert preRequestInfo
        assert len(preRequestInfo.FailedDirectives) == int(not has_answer)

        if has_answer:
            payload = get_answer_item(response, input_type, proto=TUserInfo)
            assert payload
            assert payload.Puid == puid
        else:
            assert preRequestInfo.FailedDirectives[0] == 'context_save_directive'


class TestContextSavePost:
    def test_empty(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(ServiceHandles.CONTEXT_SAVE_POST, create_grpc_request())

        assert len(response.Answers) == 1
        resp = get_answer_item(response, ItemTypes.CONTEXT_SAVE_RESPONSE, proto=TContextSaveResponse)
        assert not resp.FailedDirectives

    def test_http_good(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.DATASYNC_HTTP_RESPONSE,
                        "data": THttpResponse(StatusCode=200, Content=b"I want potatoes"),
                    },
                    {
                        "type": ItemTypes.NOTIFICATOR_SUBSCRIPTION_HTTP_RESPONSE,
                        "data": THttpResponse(StatusCode=200, Content=b"I want potatoes"),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 1
        resp = get_answer_item(response, ItemTypes.CONTEXT_SAVE_RESPONSE, proto=TContextSaveResponse)
        assert not resp.FailedDirectives

    def test_http_bad(self, cuttlefish: Cuttlefish):
        bad_http_resp = THttpResponse(StatusCode=418, Content=b"I am a smart Alice Teapot, buy me!")
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.DATASYNC_HTTP_RESPONSE,
                        "data": bad_http_resp,
                    },
                    {
                        "type": ItemTypes.MEMENTO_HTTP_RESPONSE,
                        "data": bad_http_resp,
                    },
                    {
                        "type": ItemTypes.NOTIFICATOR_SUBSCRIPTION_HTTP_RESPONSE,
                        "data": bad_http_resp,
                    },
                    {
                        "type": ItemTypes.NOTIFICATOR_MARK_AS_READ_HTTP_RESPONSE,
                        "data": bad_http_resp,
                    },
                    {
                        "type": ItemTypes.NOTIFICATOR_SEND_SUP_PUSH_HTTP_RESPONSE,
                        "data": bad_http_resp,
                    },
                    {
                        "type": ItemTypes.PERSONAL_CARDS_DISMISS_HTTP_RESPONSE,
                        "data": bad_http_resp,
                    },
                    {
                        "type": ItemTypes.PERSONAL_CARDS_ADD_HTTP_RESPONSE,
                        "data": bad_http_resp,
                    },
                    {
                        "type": ItemTypes.NOTIFICATOR_PUSH_TYPED_SEMANTIC_FRAME_RESPONSE,
                        "data": bad_http_resp,
                    },
                    {
                        "type": ItemTypes.MATRIX_SCHEDULER_ADD_SCHEDULE_ACTION_RESPONSE,
                        "data": bad_http_resp,
                    },
                    {
                        "type": ItemTypes.S3_SAVE_USER_AUDIO_HTTP_RESPONSE,
                        "data": bad_http_resp,
                    },
                ],
            ),
        )

        assert len(response.Answers) == 1
        resp = get_answer_item(response, ItemTypes.CONTEXT_SAVE_RESPONSE, proto=TContextSaveResponse)
        assert resp.FailedDirectives == [
            "add_schedule_action",
            "mark_notification_as_read",
            "personal_cards",
            "push_message",
            "push_typed_semantic_frame",
            "save_user_audio",
            "update_datasync",
            "update_memento",
            "update_notification_subscription",
        ]

    def test_http_silent_bad(self, cuttlefish: Cuttlefish):
        http_req = THttpRequest(Path="/somewhere", Content=b"I am a smart Alice Teapot, buy me!")
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.DATASYNC_HTTP_REQUEST,
                        "data": http_req,
                    },
                    {
                        "type": ItemTypes.MEMENTO_HTTP_REQUEST,
                        "data": http_req,
                    },
                    {
                        "type": ItemTypes.NOTIFICATOR_SUBSCRIPTION_HTTP_REQUEST,
                        "data": http_req,
                    },
                    {
                        "type": ItemTypes.NOTIFICATOR_MARK_AS_READ_HTTP_REQUEST,
                        "data": http_req,
                    },
                    {
                        "type": ItemTypes.NOTIFICATOR_SEND_SUP_PUSH_HTTP_REQUEST,
                        "data": http_req,
                    },
                    {
                        "type": ItemTypes.PERSONAL_CARDS_DISMISS_HTTP_REQUEST,
                        "data": http_req,
                    },
                    {
                        "type": ItemTypes.PERSONAL_CARDS_ADD_HTTP_REQUEST,
                        "data": http_req,
                    },
                    {
                        "type": ItemTypes.NOTIFICATOR_PUSH_TYPED_SEMANTIC_FRAME_REQUEST,
                        "data": http_req,
                    },
                    {
                        "type": ItemTypes.MATRIX_SCHEDULER_ADD_SCHEDULE_ACTION_REQUEST,
                        "data": http_req,
                    },
                    {
                        "type": ItemTypes.S3_SAVE_USER_AUDIO_HTTP_REQUEST,
                        "data": http_req,
                    },
                ],
            ),
        )

        assert len(response.Answers) == 1
        resp = get_answer_item(response, ItemTypes.CONTEXT_SAVE_RESPONSE, proto=TContextSaveResponse)
        assert resp.FailedDirectives == [
            "add_schedule_action",
            "mark_notification_as_read",
            "personal_cards",
            "push_message",
            "push_typed_semantic_frame",
            "save_user_audio",
            "update_datasync",
            "update_memento",
            "update_notification_subscription",
        ]

    def test_failed_directives_from_requests_info(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.CONTEXT_SAVE_PRE_REQUESTS_INFO,
                        "data": TContextSavePreRequestsInfo(
                            RequestCreatedForDirectives=["update_datasync"],
                            FailedDirectives=[
                                "personal_cards",
                                "push_message",
                                "push_typed_semantic_frame",
                                "unknown",
                            ],
                        ),
                    }
                ],
            ),
        )

        assert len(response.Answers) == 1
        resp = get_answer_item(response, ItemTypes.CONTEXT_SAVE_RESPONSE, proto=TContextSaveResponse)
        assert resp.FailedDirectives == [
            "personal_cards",
            "push_message",
            "push_typed_semantic_frame",
            "unknown",
        ]

    @pytest.mark.parametrize(
        "response_type, directive_name",
        [
            (ItemTypes.MM_SESSION_RESPONSE, "cachalot_mm_session"),
            (ItemTypes.CACHALOT_SAVE_ASR_OPTIONS_PATCH_RESPONSE, "patch_asr_options_for_next_request"),
        ],
    )
    @pytest.mark.parametrize(
        "status, failed",
        [
            (EResponseStatus.PENDING, True),
            (EResponseStatus.OK, False),
            (EResponseStatus.CREATED, False),
            (EResponseStatus.NO_CONTENT, True),
            (EResponseStatus.BAD_REQUEST, True),
            (EResponseStatus.UNAUTHORIZED, True),
            (EResponseStatus.NOT_FOUND, True),
        ],
    )
    def test_cachalot_response(self, cuttlefish: Cuttlefish, response_type, directive_name, status, failed):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.CONTEXT_SAVE_POST,
            create_grpc_request(
                items=[
                    {
                        "type": response_type,
                        "data": TCachalotResponse(
                            Status=status,
                        ),
                    }
                ],
            ),
        )

        assert len(response.Answers) == 1
        resp = get_answer_item(response, ItemTypes.CONTEXT_SAVE_RESPONSE, proto=TContextSaveResponse)
        if directive_name == "cachalot_mm_session":
            # MM session is special
            assert resp.FailedMegamindSession == failed
            resp.FailedDirectives == []
        else:
            assert not resp.FailedMegamindSession
            assert resp.FailedDirectives == ([directive_name] if failed else [])

    @pytest.mark.parametrize(
        "output_type, add_response_item, has_failed_directives",
        [
            ("my_lovely_response_type", False, True),
            ("my_lovely_response_type", True, False),
            (None, None, False),
        ],
    )
    def test_check_response_for_directive_request(
        self, cuttlefish: Cuttlefish, output_type: str, add_response_item: bool, has_failed_directives: bool
    ):
        directive_name = "TProtobufUniproxyDirective"

        check_for_directives = []
        if output_type:
            check_for_directives.append(
                TContextSavePreRequestsInfo.TCheckResponseForDirectiveRequest(
                    OutputType=output_type, DirectiveName=directive_name
                )
            )

        request_items = [
            {
                "type": ItemTypes.CONTEXT_SAVE_PRE_REQUESTS_INFO,
                "data": TContextSavePreRequestsInfo(CheckResponseForDirectiveRequest=check_for_directives),
            }
        ]
        if add_response_item and output_type:
            request_items.append({"type": output_type, "data": TUserInfo(Puid="my-lovely-puid")})
        response = cuttlefish.make_grpc_request(ServiceHandles.CONTEXT_SAVE_POST, create_grpc_request(request_items))

        resp = get_answer_item(response, ItemTypes.CONTEXT_SAVE_RESPONSE, proto=TContextSaveResponse)

        assert len(resp.FailedDirectives) == int(has_failed_directives)
        if has_failed_directives:
            assert f"{directive_name}.{output_type}" == resp.FailedDirectives[0]
