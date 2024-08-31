import pytest

from alice.matrix.notificator.tests.library.test_base import MatrixTestBase

from alice.protos.api.matrix.delivery_pb2 import (
    TDeliveryResponse,
)
from alice.protos.api.notificator.api_pb2 import (
    EDirectiveStatus,
)
from alice.uniproxy.library.protos.notificator_pb2 import (
    TManageSubscription,
)
from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_delivery,
    get_delivery_on_connect,
    get_directive_change_status,
    get_manage_subscription,
    get_push_message,
)

from google.protobuf import json_format


class TestDeliveryOnConnect(MatrixTestBase):

    @pytest.mark.asyncio
    async def test_simple(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        def _check_subway_request_basic_parts(last_req, req_count, expected_req_count, expected_push_ids):
            assert req_count == expected_req_count

            assert len(last_req.Destinations) == 1
            assert last_req.Destinations[0].DeviceId == device_id

            assert last_req.QuasarMsg.Uid == puid
            assert last_req.QuasarMsg.DeviceId == device_id
            assert last_req.QuasarMsg.StartTime

            assert last_req.QuasarMsg.PushIds == expected_push_ids

        await matrix.register_connection(puid, device_id)
        delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id))
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.OK
        push_ids = [delivery_push_response.PushId]

        last_req_delivery_push, req_count = subway_mock.get_requests_info()
        _check_subway_request_basic_parts(last_req_delivery_push, req_count, 1, push_ids)
        assert len(last_req_delivery_push.QuasarMsg.SkDirectives) == 1

        assert matrix.perform_delivery_on_connect_request(get_delivery_on_connect(puid, device_id)).json()["code"] == 200

        last_req, req_count = subway_mock.get_requests_info()
        _check_subway_request_basic_parts(last_req, req_count, 2, push_ids)
        assert len(last_req.QuasarMsg.SkDirectives) == 2
        delivery_push_is_first = (last_req.QuasarMsg.SkDirectives[0] == last_req_delivery_push.QuasarMsg.SkDirectives[0])
        assert last_req.QuasarMsg.SkDirectives[int(not delivery_push_is_first)] == last_req_delivery_push.QuasarMsg.SkDirectives[0]
        assert json_format.MessageToDict(last_req.QuasarMsg.SkDirectives[int(delivery_push_is_first)]) == {
            "name": "notify",
            "payload": {
                "version_id": "0",
            },
        }

        push_message = get_push_message(puid, device_id)

        # Subscribe
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        delivery_response = matrix.perform_delivery_request(push_message).json()
        assert delivery_response["code"] == 200
        notification_id = delivery_response["id"]

        last_req_delivery, req_count = subway_mock.get_requests_info()
        _check_subway_request_basic_parts(last_req, req_count, 3, push_ids)
        assert len(last_req_delivery.QuasarMsg.SkDirectives) == 1

        for device_model, expected_req_count in [
            ("yandexstation", 4),
            ("unknown", 5),
        ]:
            assert matrix.perform_delivery_on_connect_request(get_delivery_on_connect(puid, device_id, device_model=device_model)).json()["code"] == 200

            last_req, req_count = subway_mock.get_requests_info()
            _check_subway_request_basic_parts(last_req, req_count, expected_req_count, push_ids)
            assert len(last_req.QuasarMsg.SkDirectives) == 2
            delivery_push_is_first = (last_req.QuasarMsg.SkDirectives[0] == last_req_delivery_push.QuasarMsg.SkDirectives[0])
            assert last_req.QuasarMsg.SkDirectives[int(not delivery_push_is_first)] == last_req_delivery_push.QuasarMsg.SkDirectives[0]

            if device_model == "yandexstation":
                assert json_format.MessageToDict(last_req.QuasarMsg.SkDirectives[int(delivery_push_is_first)]) == {
                    "name": "notify",
                    "payload": {
                        "notifications": [
                            {
                                "id": notification_id,
                                "text": push_message.Notification.Text,
                                "subscription_id": "1",
                            }
                        ],
                        "version_id": "0",
                    }
                }
            elif device_model == "unknown":
                assert json_format.MessageToDict(last_req.QuasarMsg.SkDirectives[int(delivery_push_is_first)]) == {
                    "name": "notify",
                    "payload": {
                        "version_id": "0",
                    }
                }
            else:
                assert False, f"Unknown device_model {device_model}"

        # Unsubscribe device
        assert matrix.perform_subscriptions_devices_manage_request(puid, device_id, "unsubscribe").json()["code"] == 200

        last_req, req_count = subway_mock.get_requests_info()
        assert req_count == 6
        # Do not check last_req from unsubscribe request

        assert matrix.perform_delivery_on_connect_request(get_delivery_on_connect(puid, device_id)).json()["code"] == 200

        last_req, req_count = subway_mock.get_requests_info()
        _check_subway_request_basic_parts(last_req, req_count, 7, push_ids)
        assert len(last_req.QuasarMsg.SkDirectives) == 1
        assert last_req.QuasarMsg.SkDirectives[0] == last_req_delivery_push.QuasarMsg.SkDirectives[0]

        matrix.perform_directive_change_status_request(get_directive_change_status(puid, device_id, push_ids, EDirectiveStatus.ED_DELIVERED))
        assert matrix.perform_delivery_on_connect_request(get_delivery_on_connect(puid, device_id)).json()["code"] == 200

        last_req, req_count = subway_mock.get_requests_info()
        _check_subway_request_basic_parts(last_req, req_count, 8, [])
        assert len(last_req.QuasarMsg.SkDirectives) == 0


class TestDeliveryOnConnectMockMode(MatrixTestBase):
    matrix_pushes_and_notifications_mock_mode = True

    @pytest.mark.asyncio
    async def test_simple(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        await matrix.register_connection(puid, device_id)
        delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id))
        assert delivery_push_response.Code == TDeliveryResponse.EResponseCode.OK
        assert subway_mock.get_requests_count() == 0

        assert matrix.perform_delivery_on_connect_request(get_delivery_on_connect(puid, device_id)).json()["code"] == 200
        assert subway_mock.get_requests_count() == 0

        push_message = get_push_message(puid, device_id)

        # Subscribe
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        assert matrix.perform_delivery_request(push_message).json()["code"] == 200
        assert matrix.perform_delivery_on_connect_request(get_delivery_on_connect(puid, device_id)).json()["code"] == 200

        assert subway_mock.get_requests_count() == 0
