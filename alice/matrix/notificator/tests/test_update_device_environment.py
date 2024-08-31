import pytest

from alice.matrix.notificator.tests.library.constants import ItemTypes, ServiceHandlers
from alice.matrix.notificator.tests.library.test_base import MatrixAppHostSingleItemHandlerTestBase

from alice.protos.api.matrix.device_environment_pb2 import (
    TUpdateDeviceEnvironmentRequest,
    TUpdateDeviceEnvironmentResponse,
)
from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_update_device_environment_request,
)


class TestUpdateDeviceEnvironment(MatrixAppHostSingleItemHandlerTestBase):
    path = ServiceHandlers.UPDATE_DEVICE_ENVIRONMENT
    request_item_type = ItemTypes.DEVICE_ENVIRONMENT_UPDATE_REQUEST
    response_item_type = ItemTypes.DEVICE_ENVIRONMENT_UPDATE_RESPONSE
    simple_request_proto = get_update_device_environment_request()
    response_proto_type = TUpdateDeviceEnvironmentResponse

    @pytest.mark.asyncio
    async def test_device_environment_not_set(self, matrix):
        response = await self._perform_matrix_request(matrix, TUpdateDeviceEnvironmentRequest())
        assert len(self._get_response_item_datas(response)) == 0
        assert response.has_exception()
        assert b"Device environment type is not set" in response.get_exception()
