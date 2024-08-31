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
    get_directive_status,
    get_manage_subscription,
    get_notification_change_status,
    get_push_message,
)

from google.protobuf import json_format


class TestGDPR(MatrixTestBase):

    @pytest.mark.asyncio
    async def test_simple(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        def _check_user_data(
            expected_subscription_ids,
            expected_notification_ids,
            expected_archived_notification_count,
            expected_unsubscribed_devices,
            expected_directive_statuses,
        ):
            notifications_response = matrix.perform_notifications_request(puid)

            assert set(map(lambda subscription: int(subscription.Id), notifications_response.Subscriptions)) == set(expected_subscription_ids)
            assert set(map(lambda notification: notification.Id, notifications_response.Notifications)) == set(expected_notification_ids)
            assert notifications_response.CountArchived == expected_archived_notification_count
            assert set(map(lambda unsubscribed_device: unsubscribed_device.DeviceId, notifications_response.UnsubscribedDevices)) == set(expected_unsubscribed_devices)

            for current_device_id, push_id, status in expected_directive_statuses:
                assert matrix.perform_directive_status_request(get_directive_status(puid, current_device_id, push_id)).Status == status

        await matrix.register_connection(puid, device_id, ip="127.0.0.1")

        # Register unsubscribed device
        unsubscribed_device_id = f"{device_id}_unsubscribed"
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
        for i in range(4):
            push_message.Notification.Text = f"{puid}_{i}"
            delivery_response = matrix.perform_delivery_request(push_message).json()
            assert delivery_response["code"] == 200
            notification_ids.append(delivery_response["id"])

            subway_request, req_count = subway_mock.get_requests_info()
            assert req_count == 2 + i
            # Do not check subway_request from delivery request

        matrix.perform_notifications_change_status_request(get_notification_change_status(puid, notification_ids[:2]))

        subway_request, req_count = subway_mock.get_requests_info()
        assert req_count == 6
        # Do not check subway_request from notifications change status request

        device_and_push_ids = []
        for current_device_id in [device_id, unsubscribed_device_id]:
            delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, current_device_id))
            assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK
            assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.OK
            device_and_push_ids.append((current_device_id, delivery_push_response.PushId))

            subway_request, req_count = subway_mock.get_requests_info()
            assert req_count == (8 if current_device_id == unsubscribed_device_id else 7)
            # Do not check subway_request from delivery push request

        _check_user_data(
            expected_subscription_ids=[1],
            expected_notification_ids=notification_ids[2:],
            expected_archived_notification_count=2,
            expected_unsubscribed_devices=[unsubscribed_device_id],
            expected_directive_statuses=[
                (current_device_id, push_id, EDirectiveStatus.ED_NEW)
                for (current_device_id, push_id) in device_and_push_ids
            ],
        )

        matrix.perform_gdpr_request(puid)

        _check_user_data(
            expected_subscription_ids=[],
            expected_notification_ids=[],
            expected_archived_notification_count=0,
            expected_unsubscribed_devices=[],
            # We can't get a list of directives,
            # but we can check that the data of a particular directives has been deleted
            expected_directive_statuses=[
                (current_device_id, push_id, EDirectiveStatus.ED_NOT_FOUND)
                for (current_device_id, push_id) in device_and_push_ids
            ],
        )

        subway_request, req_count = subway_mock.get_requests_info()
        assert req_count == 10

        assert len(subway_request.Destinations) == 1
        # Unsubscribed devices settings are removed
        # So an empty state is sent to all devices
        assert subway_request.Destinations[0].DeviceId in [device_id, unsubscribed_device_id]
        assert len(subway_request.QuasarMsg.SkDirectives) == 1
        assert json_format.MessageToDict(subway_request.QuasarMsg.SkDirectives[0]) == {
            "name": "notify",
            "payload": {
                "version_id": "6",
            }
        }

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_puid", b"'puid' param not found"),
        ("empty_puid", b"Puid is empty"),
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
            http_response = matrix.perform_gdpr_request(None, raise_for_status=False)
        elif error_type == "empty_puid":
            http_response = matrix.perform_gdpr_request("", raise_for_status=False)
        else:
            assert False, f"Unknown error_type {error_type}"

        assert http_response.status_code == 400
        assert error_message in http_response.content

        assert subway_mock.get_requests_count() == 0


class TestGDPRMockMode(MatrixTestBase):
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

        matrix.perform_gdpr_request(puid)
        assert subway_mock.get_requests_count() == 0
