import pytest
import asyncio
import uuid

from alice.matrix.notificator.tests.library.test_base import MatrixTestBase

from alice.protos.api.matrix.delivery_pb2 import TDeliveryResponse
from alice.protos.api.notificator.api_pb2 import EDirectiveStatus
from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_delivery,
    get_directive_change_status,
    get_directive_status,
)

ALL_STATUSES = [
    EDirectiveStatus.ED_NOT_FOUND,
    EDirectiveStatus.ED_NEW,
    EDirectiveStatus.ED_DELIVERED,
    EDirectiveStatus.ED_DELETED,
    EDirectiveStatus.ED_EXPIRED
]


class TestDirective(MatrixTestBase):

    @pytest.mark.asyncio
    async def test_directive_status(
        self,
        matrix,
        puid,
        device_id,
    ):
        # Test not found
        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, f"random_id_{uuid.uuid4()}")).Status == EDirectiveStatus.ED_NOT_FOUND

        await matrix.register_connection(puid, device_id)

        # Big ttl
        delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id))
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.OK
        big_ttl_id = delivery_push_response.PushId

        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, big_ttl_id)).Status == EDirectiveStatus.ED_NEW

        # Small ttl
        delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id, ttl=3))
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.OK
        small_ttl_id = delivery_push_response.PushId

        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, small_ttl_id)).Status == EDirectiveStatus.ED_NEW
        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, big_ttl_id)).Status == EDirectiveStatus.ED_NEW

        # Sleep time = 4 > 3 ttl seconds
        # So directive with small ttl must be expired
        await asyncio.sleep(4)
        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, small_ttl_id)).Status == EDirectiveStatus.ED_EXPIRED
        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, big_ttl_id)).Status == EDirectiveStatus.ED_NEW

        matrix.perform_directive_change_status_request(get_directive_change_status(puid, device_id, [small_ttl_id, big_ttl_id], EDirectiveStatus.ED_DELIVERED))

        # If the directive status is not ED_NEW we must return original status
        # Even if directive expired
        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, small_ttl_id)).Status == EDirectiveStatus.ED_DELIVERED
        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, big_ttl_id)).Status == EDirectiveStatus.ED_DELIVERED

    @pytest.mark.asyncio
    async def test_directive_change_status(
        self,
        matrix,
        puid,
        device_id,
    ):
        await matrix.register_connection(puid, device_id)

        # Send first push
        delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id))
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.OK
        first_push_id = delivery_push_response.PushId

        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, first_push_id)).Status == EDirectiveStatus.ED_NEW

        # Check change status for first push
        matrix.perform_directive_change_status_request(get_directive_change_status(puid, device_id, [first_push_id], EDirectiveStatus.ED_DELIVERED))
        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, first_push_id)).Status == EDirectiveStatus.ED_DELIVERED

        # Send second push
        delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id))
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.OK
        second_push_id = delivery_push_response.PushId

        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, second_push_id)).Status == EDirectiveStatus.ED_NEW

        assert first_push_id != second_push_id
        push_ids = [first_push_id, second_push_id]

        # Change change status for all pushes
        prev_status = None
        assert EDirectiveStatus.ED_DELETED in ALL_STATUSES
        assert len(ALL_STATUSES) > 2
        for status in ALL_STATUSES:
            change_directive_status_response = matrix.perform_directive_change_status_request(get_directive_change_status(puid, device_id, push_ids, status), raise_for_status=False)
            if status == EDirectiveStatus.ED_DELETED:
                assert change_directive_status_response.status_code == 400
                assert change_directive_status_response.text == "'ED_DELETED' status in request is not supported"
            else:
                assert change_directive_status_response.status_code == 200

            for push_id in push_ids:
                real_status = matrix.perform_directive_status_request(get_directive_status(puid, device_id, second_push_id)).Status
                if status == EDirectiveStatus.ED_DELETED:
                    assert prev_status
                    assert real_status == prev_status
                else:
                    assert real_status == status

            prev_status = status

    @pytest.mark.asyncio
    async def test_directive_change_status_empty_push_ids(
        self,
        matrix,
        puid,
        device_id,
    ):
        await matrix.register_connection(puid, device_id)

        delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id))
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.OK
        push_id = delivery_push_response.PushId

        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, push_id)).Status == EDirectiveStatus.ED_NEW
        matrix.perform_directive_change_status_request(get_directive_change_status(puid, device_id, [], EDirectiveStatus.ED_DELIVERED))
        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, push_id)).Status == EDirectiveStatus.ED_NEW
