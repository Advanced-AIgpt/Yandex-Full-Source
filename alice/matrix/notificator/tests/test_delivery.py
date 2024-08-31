import pytest
import uuid

from alice.matrix.notificator.tests.library.test_base import MatrixTestBase

from alice.uniproxy.library.protos.notificator_pb2 import (
    TManageSubscription,
)
from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_manage_subscription,
    get_push_message,
)

from google.protobuf import json_format


class TestDelivery(MatrixTestBase):

    @pytest.mark.asyncio
    @pytest.mark.parametrize("with_device_id", [False, True])
    async def test_simple(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
        with_device_id,
    ):
        register_connection_count = 5

        first_device_id = f"{device_id}_1"
        push_message = get_push_message(puid, (first_device_id if with_device_id else ""))
        for i in range(register_connection_count):
            await matrix.register_connection(puid, first_device_id, ip=f"127.0.0.{i}")

        # Register second device
        second_device_id = f"{device_id}_2"
        for i in range(register_connection_count):
            await matrix.register_connection(puid, second_device_id, ip=f"127.0.{i}.0")

        # Register unsubscribed device
        unsubscribed_device_id = f"{device_id}_3"
        for i in range(register_connection_count):
            await matrix.register_connection(puid, unsubscribed_device_id, ip=f"127.{i}.0.0")

        # Register bad model device
        bad_model_device_id = f"{device_id}_4"
        for i in range(register_connection_count):
            await matrix.register_connection(puid, bad_model_device_id, ip=f"127.{i + 1}.{i + 1}.0", device_model="unkonwn")

        # Unsubscribe special device
        assert matrix.perform_subscriptions_devices_manage_request(puid, unsubscribed_device_id, "unsubscribe").json()["code"] == 200

        last_req, req_count = subway_mock.get_requests_info()
        assert req_count == 1
        # Do not check last_req from unsubscribe request

        assert matrix.perform_delivery_request(push_message).json() == {
            "code": 404,
            "error": "User or device is not subscribed",
        }

        last_req_delivery_push, req_count = subway_mock.get_requests_info()
        # Nothing happened because the user is not subscribed
        assert req_count == 1

        notifications_response = matrix.perform_notifications_request(puid)
        assert not notifications_response.Notifications
        assert not notifications_response.Subscriptions
        assert notifications_response.CountArchived == 0

        assert len(notifications_response.UnsubscribedDevices) == 1
        assert notifications_response.UnsubscribedDevices[0].DeviceId == unsubscribed_device_id

        # Subscribe
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        delivery_response = matrix.perform_delivery_request(push_message).json()
        assert delivery_response["code"] == 200
        notification_id = delivery_response["id"]

        for current_device_id in [first_device_id, second_device_id]:
            notifications_response = matrix.perform_notifications_request(puid, device_id=current_device_id)

            assert len(notifications_response.Subscriptions) == 1
            assert notifications_response.Subscriptions[0].Name == "Дайджест Алисы"
            assert int(notifications_response.Subscriptions[0].Timestamp) > 0
            assert notifications_response.CountArchived == 0

            assert len(notifications_response.UnsubscribedDevices) == 1
            assert notifications_response.UnsubscribedDevices[0].DeviceId == unsubscribed_device_id

            if with_device_id and current_device_id == second_device_id:
                assert not notifications_response.Notifications
            else:
                assert len(notifications_response.Notifications) == 1
                assert notifications_response.Notifications[0].Id == notification_id
                assert notifications_response.Notifications[0].Text == push_message.Notification.Text
                assert int(notifications_response.Notifications[0].Timestamp) > 0
                assert notifications_response.Notifications[0].SubscriptionId == push_message.Notification.SubscriptionId

        subway_request, req_count = subway_mock.get_requests_info()
        assert req_count == (2 if with_device_id else 3)
        assert len(subway_request.Destinations) == 1

        if with_device_id:
            assert subway_request.Destinations[0].DeviceId == first_device_id
        else:
            assert subway_request.Destinations[0].DeviceId in (first_device_id, second_device_id)

        assert subway_request.QuasarMsg.Uid == puid
        assert subway_request.QuasarMsg.DeviceId == subway_request.Destinations[0].DeviceId
        assert subway_request.QuasarMsg.StartTime
        assert len(subway_request.QuasarMsg.PushIds) == 0

        assert len(subway_request.QuasarMsg.SkDirectives) == 1
        assert json_format.MessageToDict(subway_request.QuasarMsg.SkDirectives[0]) == {
            "name": "notify",
            "payload": {
                "notifications": [
                    {
                        "id": notification_id,
                        "text": push_message.Notification.Text,
                        "subscription_id": "1",
                    }
                ],
                "version_id": "1",
                "ring": "Delicate",
            }
        }

        # Check that duplicate is ignored
        delivery_response = matrix.perform_delivery_request(push_message, raise_for_status=False)
        assert delivery_response.status_code == 208
        assert delivery_response.json()["code"] == 208
        assert delivery_response.json()["error"] == "Notification duplicate"

        notifications_response = matrix.perform_notifications_request(puid, first_device_id)

        assert len(notifications_response.Subscriptions) == 1
        assert notifications_response.Subscriptions[0].Name == "Дайджест Алисы"
        assert int(notifications_response.Subscriptions[0].Timestamp) > 0
        assert notifications_response.CountArchived == 0

        assert len(notifications_response.UnsubscribedDevices) == 1
        assert notifications_response.UnsubscribedDevices[0].DeviceId == unsubscribed_device_id

        assert len(notifications_response.Notifications) == 1
        assert notifications_response.Notifications[0].Id == notification_id
        assert notifications_response.Notifications[0].Text == push_message.Notification.Text
        assert int(notifications_response.Notifications[0].Timestamp) > 0
        assert notifications_response.Notifications[0].SubscriptionId == push_message.Notification.SubscriptionId

        assert subway_mock.get_requests_count() == (2 if with_device_id else 3)

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_puid", b"Puid is empty"),
        ("no_notification", b"Notification content is empty"),
    ])
    async def test_invalid_request(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
        error_type,
        error_message,
    ):
        await matrix.register_connection(puid, device_id)

        push_message = get_push_message(puid, device_id)
        if error_type == "no_puid":
            push_message.ClearField("Uid")
        elif error_type == "no_notification":
            push_message.ClearField("Notification")
        else:
            assert False, f"Unknown error_type {error_type}"

        delivery_response = matrix.perform_delivery_request(push_message, raise_for_status=False)
        assert delivery_response.status_code == 400
        assert error_message in delivery_response.content

        assert subway_mock.get_requests_count() == 0

    @pytest.mark.asyncio
    async def test_no_locations(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        push_message = get_push_message(puid, device_id)

        # Subscribe
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        assert matrix.perform_delivery_request(push_message).json() == {
            "code": 404,
            "error": "No locations at all",
        }

        notifications_response = matrix.perform_notifications_request(puid, device_id)

        assert len(notifications_response.Notifications) == 1
        assert len(notifications_response.Notifications[0].Id) > 0
        assert notifications_response.Notifications[0].Text == push_message.Notification.Text
        assert int(notifications_response.Notifications[0].Timestamp) > 0
        assert notifications_response.Notifications[0].SubscriptionId == push_message.Notification.SubscriptionId

        assert subway_mock.get_requests_count() == 0


class TestDeliveryWhiteList(MatrixTestBase):
    matrix_user_white_list = [str(uuid.uuid4())]

    @pytest.mark.asyncio
    async def test_simple(
        self,
        matrix,
        subway_mock,
        device_id,
    ):
        puid = self.matrix_user_white_list[0]
        await matrix.register_connection(puid, device_id)

        push_message = get_push_message(puid, device_id)

        # Subscribe
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        delivery_response = matrix.perform_delivery_request(push_message).json()
        assert delivery_response["code"] == 200
        notification_id = delivery_response["id"]

        notifications_response = matrix.perform_notifications_request(puid, device_id)
        assert len(notifications_response.Notifications) == 1
        assert notifications_response.Notifications[0].Id == notification_id

        assert subway_mock.get_requests_count() == 1

    @pytest.mark.asyncio
    async def test_puid_is_not_in_white_list(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        await matrix.register_connection(puid, device_id)

        push_message = get_push_message(puid, device_id)

        # Subscribe
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        delivery_response = matrix.perform_delivery_request(push_message, raise_for_status=False)
        assert delivery_response.status_code == 403
        assert delivery_response.text == "Puid from request is not in white list"

        notifications_response = matrix.perform_notifications_request(puid, device_id)
        assert len(notifications_response.Notifications) == 0

        assert subway_mock.get_requests_count() == 0


class TestDeliveryMockMode(MatrixTestBase):
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

        push_message = get_push_message(puid, device_id)

        # Subscribe
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        delivery_response = matrix.perform_delivery_request(push_message).json()
        assert delivery_response["code"] == 200
        notification_id = delivery_response["id"]

        notifications_response = matrix.perform_notifications_request(puid, device_id)
        assert len(notifications_response.Notifications) == 1
        assert notifications_response.Notifications[0].Id == notification_id

        assert subway_mock.get_requests_count() == 0
