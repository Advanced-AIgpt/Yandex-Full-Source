import pytest
import uuid

from alice.matrix.notificator.tests.library.test_base import MatrixTestBase

from google.protobuf import json_format


def _get_first_demo_subscription_text():
    return """
        Хотела вам рассказать, что нового в моем "Утреннем шоу".
        Теперь вы можете выбрать источники новостей и темы для шоу на свой потрясающий вкус.
        Сделайте выбор и слушайте только то, что вам нравится
    """.replace("  ", "").replace("\n", " ").strip()


class TestDeliveryDemo(MatrixTestBase):

    @pytest.mark.asyncio
    async def test_simple(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        register_connection_count = 5

        first_device_id = f"{device_id}_1"
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

        delivery_demo_response = matrix.perform_delivery_demo_request(puid, 1).json()
        assert delivery_demo_response["code"] == 200
        notification_id = delivery_demo_response["id"]

        for current_device_id in [first_device_id, second_device_id]:
            notifications_response = matrix.perform_notifications_request(puid, current_device_id)

            assert not notifications_response.Subscriptions
            assert notifications_response.CountArchived == 0

            assert len(notifications_response.UnsubscribedDevices) == 1
            assert notifications_response.UnsubscribedDevices[0].DeviceId == unsubscribed_device_id

            assert len(notifications_response.Notifications) == 1
            assert notifications_response.Notifications[0].Id == notification_id
            assert notifications_response.Notifications[0].Text == _get_first_demo_subscription_text()

            assert int(notifications_response.Notifications[0].Timestamp) > 0
            assert notifications_response.Notifications[0].SubscriptionId == "1"

        subway_request, req_count = subway_mock.get_requests_info()
        assert req_count == 3

        assert len(subway_request.Destinations) == 1
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
                        "text": _get_first_demo_subscription_text(),
                        "subscription_id": "1",
                    }
                ],
                "version_id": "1",
                "ring": "Delicate",
            }
        }

        # Check that duplicate is ok for demo notifications
        delivery_demo_response = matrix.perform_delivery_demo_request(puid, 1).json()
        assert delivery_demo_response["code"] == 200
        new_notification_id = delivery_demo_response["id"]

        notifications_response = matrix.perform_notifications_request(puid, current_device_id)

        assert not notifications_response.Subscriptions
        assert notifications_response.CountArchived == 0

        assert len(notifications_response.UnsubscribedDevices) == 1
        assert notifications_response.UnsubscribedDevices[0].DeviceId == unsubscribed_device_id

        assert len(notifications_response.Notifications) == 2
        assert set((notifications_response.Notifications[0].Id, notifications_response.Notifications[1].Id)) == set((notification_id, new_notification_id))
        for notification in notifications_response.Notifications:
            assert notification.Text == _get_first_demo_subscription_text()
            assert int(notification.Timestamp) > 0
            assert notification.SubscriptionId == "1"

        subway_request, req_count = subway_mock.get_requests_info()
        assert req_count == 5

        assert len(subway_request.Destinations) == 1
        assert subway_request.Destinations[0].DeviceId in (first_device_id, second_device_id)

        assert subway_request.QuasarMsg.Uid == puid
        assert subway_request.QuasarMsg.DeviceId == subway_request.Destinations[0].DeviceId
        assert subway_request.QuasarMsg.StartTime
        assert len(subway_request.QuasarMsg.PushIds) == 0

        assert len(subway_request.QuasarMsg.SkDirectives) == 1
        sk_directive_json = json_format.MessageToDict(subway_request.QuasarMsg.SkDirectives[0])
        sk_directive_json["payload"]["notifications"].sort(key=lambda x: x["id"])
        assert sk_directive_json == {
            "name": "notify",
            "payload": {
                "notifications": [
                    {
                        "id": current_notification_id,
                        "text": _get_first_demo_subscription_text(),
                        "subscription_id": "1",
                    }
                    for current_notification_id in sorted((notification_id, new_notification_id))
                ],
                "version_id": "2",
                "ring": "Delicate",
            }
        }

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_puid", b"'puid' param not found"),
        ("empty_puid", b"Puid is empty"),
        ("no_subscription_id", b"'subscription_id' param not found"),
        ("not_a_number_subscription_id", b"Unable to cast subscription id \'not_int\' to ui64"),
        ("unknown_subscription_id", b"Unknown subscription id"),
        ("no_demo_subscription_id", b"Subscription does not have a demo notification"),
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

        if error_type == "no_puid":
            http_response = matrix.perform_delivery_demo_request(None, 1, raise_for_status=False)
        elif error_type == "empty_puid":
            http_response = matrix.perform_delivery_demo_request("", 1, raise_for_status=False)
        elif error_type == "no_subscription_id":
            http_response = matrix.perform_delivery_demo_request(puid, None, raise_for_status=False)
        elif error_type == "empty_subscription_id":
            http_response = matrix.perform_delivery_demo_request(puid, "", raise_for_status=False)
        elif error_type == "not_a_number_subscription_id":
            http_response = matrix.perform_delivery_demo_request(puid, "not_int", raise_for_status=False)
        elif error_type == "unknown_subscription_id":
            http_response = matrix.perform_delivery_demo_request(puid, 100500, raise_for_status=False)
        elif error_type == "no_demo_subscription_id":
            http_response = matrix.perform_delivery_demo_request(puid, 4, raise_for_status=False)
        else:
            assert False, f"Unknown error_type {error_type}"

        assert http_response.status_code == 400
        assert error_message in http_response.content

    @pytest.mark.asyncio
    async def test_no_locations(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        assert matrix.perform_delivery_demo_request(puid, 1).json() == {
            "code": 404,
            "error": "No locations at all",
        }

        notifications_response = matrix.perform_notifications_request(puid, device_id)

        assert len(notifications_response.Notifications) == 1
        assert len(notifications_response.Notifications[0].Id) > 0
        assert notifications_response.Notifications[0].Text == _get_first_demo_subscription_text()
        assert int(notifications_response.Notifications[0].Timestamp) > 0
        assert notifications_response.Notifications[0].SubscriptionId == "1"

        assert subway_mock.get_requests_count() == 0


class TestDeliveryDemoWhiteList(MatrixTestBase):
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

        delivery_demo_response = matrix.perform_delivery_demo_request(puid, 1).json()
        assert delivery_demo_response["code"] == 200
        notification_id = delivery_demo_response["id"]

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

        delivery_demo_response = matrix.perform_delivery_demo_request(puid, 1, raise_for_status=False)
        assert delivery_demo_response.status_code == 403
        assert b"Puid from request is not in white list" in delivery_demo_response.content

        notifications_response = matrix.perform_notifications_request(puid, device_id)
        assert len(notifications_response.Notifications) == 0

        assert subway_mock.get_requests_count() == 0


class TestDeliveryDemoMockMode(MatrixTestBase):
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

        delivery_demo_response = matrix.perform_delivery_demo_request(puid, 1).json()
        assert delivery_demo_response["code"] == 200
        notification_id = delivery_demo_response["id"]

        notifications_response = matrix.perform_notifications_request(puid, device_id)
        assert len(notifications_response.Notifications) == 1
        assert notifications_response.Notifications[0].Id == notification_id

        assert subway_mock.get_requests_count() == 0
