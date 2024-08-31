import json
import pytest

from alice.matrix.notificator.tests.library.constants import ServiceHandlers
from alice.matrix.notificator.tests.library.test_base import MatrixTestBase

from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_get_devices,
)


class TestSimple(MatrixTestBase):

    @pytest.mark.asyncio
    async def test_ping(self, matrix):
        assert "pong" == matrix.perform_get_request("/admin?action=ping").text.strip()
        # Check that proxy service still works (more info about this assert in ZION-291)
        assert "pong" == matrix.perform_get_request("/ping").text.strip()

    @pytest.mark.asyncio
    async def test_json_request_success(
        self,
        matrix,
        puid,
        device_id,
    ):
        req = {
            "puid": puid,
            "device_id": device_id,
            "push_id": "my_super_push_id",
            "ttl": 1234,
            "semantic_frame_request_data": {
                "typed_semantic_frame": {
                    "sound_set_level_semantic_frame": {
                        "level": {
                            "num_level_value": 5,
                        }
                    }
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "sound_set_level",
                }
            }
        }
        response = matrix.perform_post_request(ServiceHandlers.HTTP_DELIVERY_PUSH, data=json.dumps(req), headers={"Content-Type": "application/json"})
        json_response = json.loads(response.content)
        assert json_response.get("code", "") == "NoLocations"
        assert json_response.get("id", "") == "my_super_push_id"

    @pytest.mark.asyncio
    @pytest.mark.parametrize("response_format", ["spack", "json"])
    @pytest.mark.parametrize("sensors_handler, sensor_to_check", [
        (ServiceHandlers.HTTP_YDB_SENSORS, "Endpoints/Total"),
        (ServiceHandlers.HTTP_SENSORS, "mlock.self.status"),
    ])
    async def test_sensors(self, matrix, response_format, sensors_handler, sensor_to_check):
        response = matrix.perform_sensors_request(
            sensors_handler,
            response_format,
        )

        assert "Content-Type" in response.headers
        content_type = response.headers["Content-Type"]

        if response_format == "json":
            assert content_type == "application/json"

            json_response = response.json()
            assert len([
                sensor for sensor in json_response['sensors'] if sensor['labels']['sensor'] == sensor_to_check
            ]) != 0
        elif response_format == "spack":
            assert content_type == "application/x-solomon-spack"
            assert len(response.content) != 0
        else:
            assert False, f"Unknown response format: {content_type}"

    @pytest.mark.asyncio
    async def test_json_request_error(self, matrix):
        response = matrix.perform_post_request(
            ServiceHandlers.HTTP_DEVICES,
            data=json.dumps("qwe"),
            headers={
                "Content-Type": "application/json"
            },
            raise_for_status=False
        )
        assert response.status_code == 400
        assert b"Unable to parse proto" in response.content
        assert b"expected json map" in response.content

    @pytest.mark.asyncio
    async def test_proto_request_success(
        self,
        matrix,
        puid,
    ):
        assert len(matrix.perform_get_devices_request(get_get_devices(puid)).Devices) == 0

    @pytest.mark.asyncio
    async def test_proto_request_error(self, matrix):
        response = matrix.perform_post_request(ServiceHandlers.HTTP_DEVICES, data="qwe", raise_for_status=False)
        assert response.status_code == 400
        assert b"Unable to parse proto" in response.content
