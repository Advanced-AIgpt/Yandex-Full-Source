import pytest

from alice.matrix.notificator.tests.library.test_base import MatrixTestBase

from alice.uniproxy.library.protos.notificator_pb2 import (
    TManageSubscription,
)
from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_manage_subscription,
    get_notification_change_status,
    get_push_message,
)

from google.protobuf import json_format


class TestNotifications(MatrixTestBase):

    @pytest.mark.asyncio
    @pytest.mark.parametrize("with_device_id", [False, True])
    async def test_notifications(
        self,
        matrix,
        puid,
        device_id,
        with_device_id,
    ):
        await matrix.register_connection(puid, device_id)

        push_message = get_push_message(puid, (device_id if with_device_id else ""))
        delivery_response = matrix.perform_delivery_request(push_message).json()
        assert delivery_response["code"] == 404
        assert delivery_response["error"] == "User or device is not subscribed"

        notifications_response = matrix.perform_notifications_request(puid, device_id)
        assert not notifications_response.Notifications
        assert not notifications_response.Subscriptions
        assert notifications_response.CountArchived == 0
        assert not notifications_response.UnsubscribedDevices

        # Subscribe
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        delivery_response = matrix.perform_delivery_request(push_message).json()
        assert delivery_response["code"] == 200
        notification_id = delivery_response["id"]

        for current_device_id in [None, device_id]:
            notifications_response = matrix.perform_notifications_request(puid, current_device_id)

            assert len(notifications_response.Subscriptions) == 1
            assert notifications_response.Subscriptions[0].Name == "Дайджест Алисы"
            assert int(notifications_response.Subscriptions[0].Timestamp) > 0
            assert notifications_response.CountArchived == 0
            assert not notifications_response.UnsubscribedDevices

            if with_device_id and current_device_id is None:
                assert not notifications_response.Notifications
            else:
                assert len(notifications_response.Notifications) == 1
                assert notifications_response.Notifications[0].Id == notification_id
                assert notifications_response.Notifications[0].Text == push_message.Notification.Text
                assert int(notifications_response.Notifications[0].Timestamp) > 0
                assert notifications_response.Notifications[0].SubscriptionId == push_message.Notification.SubscriptionId

        notifications_response = matrix.perform_notifications_request(puid, device_id, device_model="unknown")
        assert not notifications_response.Notifications
        assert len(notifications_response.Subscriptions) == 1
        assert notifications_response.Subscriptions[0].Name == "Дайджест Алисы"
        assert int(notifications_response.Subscriptions[0].Timestamp) > 0
        assert notifications_response.CountArchived == 0
        assert not notifications_response.UnsubscribedDevices

        # Unsubscribe device
        assert matrix.perform_subscriptions_devices_manage_request(puid, device_id, "unsubscribe").json()["code"] == 200

        notifications_response = matrix.perform_notifications_request(puid, device_id)
        assert not notifications_response.Notifications
        assert not notifications_response.Subscriptions
        assert notifications_response.CountArchived == 0
        assert len(notifications_response.UnsubscribedDevices) == 1
        assert notifications_response.UnsubscribedDevices[0].DeviceId == device_id

        # Subscribe device back
        assert matrix.perform_subscriptions_devices_manage_request(puid, device_id, "subscribe").json()["code"] == 200

        for already_marked_as_read in [False, True]:
            notifications_response = matrix.perform_notifications_request(puid, device_id)

            assert len(notifications_response.Subscriptions) == 1
            assert notifications_response.Subscriptions[0].Name == "Дайджест Алисы"
            assert int(notifications_response.Subscriptions[0].Timestamp) > 0
            assert notifications_response.CountArchived == int(already_marked_as_read)
            assert not notifications_response.UnsubscribedDevices

            if already_marked_as_read:
                assert not notifications_response.Notifications
            else:
                assert len(notifications_response.Notifications) == 1
                assert notifications_response.Notifications[0].Id == notification_id
                assert notifications_response.Notifications[0].Text == push_message.Notification.Text
                assert int(notifications_response.Notifications[0].Timestamp) > 0
                assert notifications_response.Notifications[0].SubscriptionId == push_message.Notification.SubscriptionId

                matrix.perform_notifications_change_status_request(get_notification_change_status(puid, [notification_id]))

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_puid", b"'puid' param not found"),
        ("empty_puid", b"Puid is empty"),
    ])
    async def test_invalid_notifications(
        self,
        matrix,
        puid,
        error_type,
        error_message,
    ):
        if error_type == "no_puid":
            notifications_response, raw_response,  = matrix.perform_notifications_request(None, raise_for_status=False, need_raw_response=True)
        elif error_type == "empty_puid":
            notifications_response, raw_response = matrix.perform_notifications_request("", raise_for_status=False, need_raw_response=True)
        else:
            assert False, f"Unknown error_type {error_type}"

        assert raw_response.status_code == 400
        assert error_message in raw_response.content

    @pytest.mark.asyncio
    async def test_notifications_change_status(
        self,
        matrix,
        puid,
        device_id,
    ):
        await matrix.register_connection(puid, device_id)

        def _check_notification_ids_and_archived_notification_count(expected_notification_ids, expected_archived_notification_count):
            notifications_response = matrix.perform_notifications_request(puid)
            assert set(map(lambda notification: notification.Id, notifications_response.Notifications)) == set(expected_notification_ids)
            assert notifications_response.CountArchived == expected_archived_notification_count

        push_message = get_push_message(puid, "")

        # Subscribe
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        notification_ids = []
        for i in range(5):
            push_message.Notification.Text = f"{puid}_{i}"
            delivery_response = matrix.perform_delivery_request(push_message).json()
            assert delivery_response["code"] == 200
            notification_ids.append(delivery_response["id"])

        _check_notification_ids_and_archived_notification_count(notification_ids, 0)

        # Empty request
        matrix.perform_notifications_change_status_request(get_notification_change_status(puid, []))

        for i in range(2):
            matrix.perform_notifications_change_status_request(get_notification_change_status(puid, [notification_ids[i]]))
            _check_notification_ids_and_archived_notification_count(notification_ids[i + 1:], i + 1)

        matrix.perform_notifications_change_status_request(get_notification_change_status(puid, notification_ids[2:4]))
        _check_notification_ids_and_archived_notification_count(notification_ids[4:5], 4)

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("empty_puid", b"Puid is empty"),
    ])
    async def test_invalid_notifications_change_status(
        self,
        matrix,
        puid,
        error_type,
        error_message,
    ):
        if error_type == "empty_puid":
            http_response = matrix.perform_notifications_change_status_request(get_notification_change_status("", []), raise_for_status=False)
        else:
            assert False, f"Unknown error_type {error_type}"

        assert http_response.status_code == 400
        assert error_message in http_response.content

    @pytest.mark.asyncio
    async def test_notifications_change_status_send_state_to_device(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        first_device_id = f"{device_id}_1"
        await matrix.register_connection(puid, first_device_id, ip="127.0.0.1")

        # Register unsubscribed device
        unsubscribed_device_id = f"{device_id}_2"
        await matrix.register_connection(puid, unsubscribed_device_id, ip="127.0.0.2")

        # Unsubscribe special device
        assert matrix.perform_subscriptions_devices_manage_request(puid, unsubscribed_device_id, "unsubscribe").json()["code"] == 200

        subway_request, req_count = subway_mock.get_requests_info()
        assert req_count == 1
        # Do not check subway_request from unsubscribe request

        push_message = get_push_message(puid, "")

        # Subscribe user
        assert matrix.perform_subscriptions_manage_request(get_manage_subscription(puid, push_message.SubscriptionId, TManageSubscription.EMethod.ESubscribe)).json()["code"] == 200

        notification_ids = []
        for i in range(2):
            push_message.Notification.Text = f"{puid}_{i}"
            delivery_response = matrix.perform_delivery_request(push_message).json()
            assert delivery_response["code"] == 200
            notification_ids.append(delivery_response["id"])

            subway_request, req_count = subway_mock.get_requests_info()
            assert req_count == 2 + i
            # Do not check subway_request from delivery request

        matrix.perform_notifications_change_status_request(get_notification_change_status(puid, notification_ids[0:1]))

        subway_request, req_count = subway_mock.get_requests_info()
        assert req_count == 4

        assert len(subway_request.Destinations) == 1
        assert subway_request.Destinations[0].DeviceId == first_device_id
        assert len(subway_request.QuasarMsg.SkDirectives) == 1
        assert json_format.MessageToDict(subway_request.QuasarMsg.SkDirectives[0]) == {
            "name": "notify",
            "payload": {
                "notifications": [
                    {
                        "id": notification_ids[1],
                        "text": push_message.Notification.Text,
                        "subscription_id": "1",
                    }
                ],
                "version_id": "3",
            }
        }


class TestNotificationsMockMode(MatrixTestBase):
    matrix_pushes_and_notifications_mock_mode = True

    @pytest.mark.asyncio
    async def test_notifications_change_status_send_state_to_device(
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

        assert subway_mock.get_requests_count() == 0

        matrix.perform_notifications_change_status_request(get_notification_change_status(puid, [notification_id]))
        assert subway_mock.get_requests_count() == 0
