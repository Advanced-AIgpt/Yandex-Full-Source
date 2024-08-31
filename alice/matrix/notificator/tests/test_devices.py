import pytest

from alice.matrix.notificator.tests.library.test_base import MatrixTestBase

from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_device_locator,
    get_get_devices,
)


def _get_devices(matrix, puid, supported_features=None):
    get_devices_response = matrix.perform_get_devices_request(get_get_devices(puid, supported_features=supported_features))
    for device in get_devices_response.Devices:
        # This field is deprecated
        # More info in ZION-284
        assert not device.SupportedFeatures
    return list(sorted(map(lambda device: device.DeviceId, get_devices_response.Devices)))


class TestDevices(MatrixTestBase):

    @pytest.mark.asyncio
    async def test_simple(
        self,
        matrix,
        puid,
        device_id,
    ):
        # No locations
        assert _get_devices(matrix, puid) == []

        # Add devices
        matrix.perform_locator_add_request(get_device_locator(puid, f"{device_id}_1", supported_features=["a", "b", "c"]))
        matrix.perform_locator_add_request(get_device_locator(puid, f"{device_id}_1", supported_features=["a", "b", "f"]))

        matrix.perform_locator_add_request(get_device_locator(puid, f"{device_id}_2", supported_features=["q"]))
        matrix.perform_locator_add_request(get_device_locator(puid, f"{device_id}_2", supported_features=["a", "audio_client"]))

        matrix.perform_locator_add_request(get_device_locator(puid, f"{device_id}_3", supported_features=["a", "audio_client"]))
        matrix.perform_locator_add_request(get_device_locator(puid, f"{device_id}_3", supported_features=["q"]))

        matrix.perform_locator_add_request(get_device_locator(f"{puid}_other", f"{device_id}_4", supported_features=["a", "b", "c", "audio_client"]))

        assert _get_devices(matrix, puid) == [
            f"{device_id}_1",
            f"{device_id}_2",
            f"{device_id}_3",
        ]

        # Pick with common feature and greater timestamp
        assert _get_devices(matrix, puid, ["audio_client"]) == [
            f"{device_id}_2",
        ]

        # Empty reponse
        assert _get_devices(matrix, puid, ["unknown"]) == []

        # Pick device with different puid
        assert _get_devices(matrix, f"{puid}_other", ["audio_client"]) == [
            f"{device_id}_4",
        ]

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("empty_puid", b"Puid is empty"),
        ("unallowed_supported_feature", b"Supported feature 'bad_feature' is not allowed (more info in ZION-284)"),
    ])
    async def test_invalid_request(
        self,
        matrix,
        puid,
        error_type,
        error_message,
    ):
        get_devices_request = get_get_devices(puid)

        if error_type == "empty_puid":
            get_devices_request.ClearField("Puid")
        elif error_type == "unallowed_supported_feature":
            get_devices_request.SupportedFeatures.extend(["bad_feature"])
        else:
            assert False, f"Unknown error_type {error_type}"

        get_devices_response, raw_response = matrix.perform_get_devices_request(get_devices_request, raise_for_status=False, need_raw_response=True)
        assert raw_response.status_code == 400
        assert error_message in raw_response.content
