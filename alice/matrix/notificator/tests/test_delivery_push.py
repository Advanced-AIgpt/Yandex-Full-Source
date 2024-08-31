import pytest

import uuid

from alice.matrix.notificator.tests.library.test_base import MatrixTestBase

from alice.protos.api.matrix.delivery_pb2 import TDeliveryResponse
from alice.protos.api.notificator.api_pb2 import EDirectiveStatus
from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_delivery,
    get_directive_status,
    get_speech_kit_directive,
)

from google.protobuf import json_format


class TestDeliveryPush(MatrixTestBase):

    @pytest.mark.asyncio
    @pytest.mark.parametrize("push_id", [None, "custom_push_id"])
    @pytest.mark.parametrize("speech_kit_directive", [None, get_speech_kit_directive()])
    async def test_simple(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
        push_id,
        speech_kit_directive,
    ):
        register_connection_count = 5

        for i in range(register_connection_count):
            await matrix.register_connection(puid, device_id, ip=f"127.0.0.{i}")

        delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id, push_id=push_id, speech_kit_directive=speech_kit_directive))
        assert delivery_push_response.Code == TDeliveryResponse.EResponseCode.OK
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.OK

        if push_id is None:
            # If push_id is None, a random push id expected
            push_id = delivery_push_response.PushId
        assert push_id == delivery_push_response.PushId

        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, push_id)).Status == EDirectiveStatus.ED_NEW
        subway_request, subway_requests_count = subway_mock.get_requests_info()
        # Only one request expected
        assert subway_requests_count == 1

        assert len(subway_request.Destinations) == 1
        assert subway_request.Destinations[0].DeviceId == device_id

        assert len(subway_request.QuasarMsg.PushIds) == 1
        assert subway_request.QuasarMsg.PushIds[0] == push_id

        assert subway_request.QuasarMsg.Uid == puid
        assert subway_request.QuasarMsg.DeviceId == device_id
        assert subway_request.QuasarMsg.StartTime
        assert len(subway_request.QuasarMsg.SkDirectives) == 1

        if speech_kit_directive is not None:
            assert subway_request.QuasarMsg.SkDirectives[0] == speech_kit_directive
        else:
            # Compare with default semantic frame
            assert json_format.MessageToDict(subway_request.QuasarMsg.SkDirectives[0]) == {
                "type": "server_action",
                "name": "@@mm_semantic_frame",
                "payload": {
                    "analytics": {
                        "origin": "Scenario",
                        "purpose": "sound_set_level",
                    },
                    "typed_semantic_frame": {
                        "sound_set_level_semantic_frame": {
                            "level": {
                                "num_level_value": 0.0,
                            }
                        }
                    }
                }
            }

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_puid", "Puid is empty"),
        ("no_device_id", "Device id is empty"),
        ("no_semantic_frame_request_data", "TRequestDirective not supported or not set"),
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

        delivery_push_request = get_delivery(puid, device_id)
        if error_type == "no_puid":
            delivery_push_request.ClearField("Puid")
        elif error_type == "no_device_id":
            delivery_push_request.ClearField("DeviceId")
        elif error_type == "no_semantic_frame_request_data":
            delivery_push_request.ClearField("SemanticFrameRequestData")
        else:
            assert False, f"Unknown error_type {error_type}"

        delivery_push_response, raw_http_response = matrix.perform_delivery_push_request(delivery_push_request, raise_for_status=False, need_raw_response=True)
        assert raw_http_response.status_code == 400

        assert delivery_push_response.Code == TDeliveryResponse.EResponseCode.Unknown
        assert error_message in delivery_push_response.AddPushToDatabaseStatus.ErrorMessage
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.ERROR
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.UNKNOWN

        assert subway_mock.get_requests_count() == 0

    @pytest.mark.asyncio
    async def test_location_not_found(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id))
        assert delivery_push_response.Code == TDeliveryResponse.EResponseCode.NoLocations
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.LOCATION_NOT_FOUND
        push_id = delivery_push_response.PushId

        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, push_id)).Status == EDirectiveStatus.ED_NEW
        assert subway_mock.get_requests_count() == 0

    @pytest.mark.asyncio
    async def test_outdated_location(
        self,
        matrix,
        subway_mock,
        puid,
        device_id,
    ):
        subway_mock.set_missing_devices([device_id])

        await matrix.register_connection(puid, device_id)

        delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id))
        assert delivery_push_response.Code == TDeliveryResponse.EResponseCode.NoLocations
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.OUTDATED_LOCATION
        push_id = delivery_push_response.PushId

        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, push_id)).Status == EDirectiveStatus.ED_NEW
        assert subway_mock.get_requests_count() == 1


class TestDeliveryPushWhiteList(MatrixTestBase):
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

        delivery_push_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id))
        assert delivery_push_response.Code == TDeliveryResponse.EResponseCode.OK
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.OK
        push_id = delivery_push_response.PushId

        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, push_id)).Status == EDirectiveStatus.ED_NEW
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

        delivery_push_response, raw_http_response = matrix.perform_delivery_push_request(get_delivery(puid, device_id), raise_for_status=False, need_raw_response=True)
        assert raw_http_response.status_code == 403

        assert delivery_push_response.Code == TDeliveryResponse.EResponseCode.Unknown
        assert "is not in white list" in delivery_push_response.AddPushToDatabaseStatus.ErrorMessage
        assert delivery_push_response.AddPushToDatabaseStatus.Status == TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.ERROR
        assert delivery_push_response.SubwayRequestStatus.Status == TDeliveryResponse.TSubwayRequestStatus.EStatus.UNKNOWN

        assert subway_mock.get_requests_count() == 0


class TestDeliveryPushMockMode(MatrixTestBase):
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
        push_id = delivery_push_response.PushId

        assert matrix.perform_directive_status_request(get_directive_status(puid, device_id, push_id)).Status == EDirectiveStatus.ED_NEW
        assert subway_mock.get_requests_count() == 0
