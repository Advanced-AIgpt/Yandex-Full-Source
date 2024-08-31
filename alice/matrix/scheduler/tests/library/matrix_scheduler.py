import yatest.common

from .constants import ItemTypes, ServiceHandlers
from .ydb import init_matrix_scheduler_ydb

from alice.protos.api.matrix.scheduler_api_pb2 import (
    TAddScheduledActionResponse,
    TRemoveScheduledActionResponse,
)

from alice.matrix.library.testing.python.servant_base import ServantBase


class MatrixScheduler(ServantBase):
    def __init__(self, *args, **kwargs):
        super(MatrixScheduler, self).__init__(*args, **kwargs)

    def _get_config(self):
        config = self._get_default_config()
        config.update({
            "SchedulerService": {
                "YDBClient": self._get_default_ydb_client_config(),
                "ShardCount": 1,
            },
        })

        return config

    def _before_start(self):
        init_matrix_scheduler_ydb()

    @property
    def name(self):
        return "matrix_scheduler"

    @property
    def _binary_file_path(self):
        return yatest.common.build_path("alice/matrix/scheduler/bin/matrix_scheduler")

    async def perform_add_scheduled_action_requests(self, add_scheduled_action_requests, timeout=10, return_apphost_response_as_is=False):
        response = await self.perform_grpc_request(
            items={
                ItemTypes.ADD_SCHEDULED_ACTION_REQUEST: add_scheduled_action_requests,
            },
            path=ServiceHandlers.ADD_SCHEDULED_ACTION,
            timeout=timeout,
        )

        if return_apphost_response_as_is:
            return response

        assert not response.has_exception(), f"Add scheduled action request failed with exception: {response.get_exception()}"

        response_items = list(response.get_item_datas(item_type=ItemTypes.ADD_SCHEDULED_ACTION_RESPONSE, proto_type=TAddScheduledActionResponse))
        assert len(response_items) == len(add_scheduled_action_requests)
        return response_items

    async def perform_add_scheduled_action_request(self, add_scheduled_action_request, **kwargs):
        return await self.perform_add_scheduled_action_requests([add_scheduled_action_request], **kwargs)

    async def perform_remove_scheduled_action_requests(self, remove_scheduled_action_requests, timeout=10, return_apphost_response_as_is=False):
        response = await self.perform_grpc_request(
            items={
                ItemTypes.REMOVE_SCHEDULED_ACTION_REQUEST: remove_scheduled_action_requests,
            },
            path=ServiceHandlers.REMOVE_SCHEDULED_ACTION,
            timeout=timeout,
        )

        if return_apphost_response_as_is:
            return response

        assert not response.has_exception(), f"Remove scheduled action request failed with exception: {response.get_exception()}"

        response_items = list(response.get_item_datas(item_type=ItemTypes.REMOVE_SCHEDULED_ACTION_RESPONSE, proto_type=TRemoveScheduledActionResponse))
        assert len(response_items) == len(remove_scheduled_action_requests)
        return response_items

    async def perform_remove_scheduled_action_request(self, remove_scheduled_action_request, **kwargs):
        return await self.perform_remove_scheduled_action_requests([remove_scheduled_action_request], **kwargs)

    def perform_old_schedule_action_request(self, schedule_action, raise_for_status=True):
        return self.perform_post_request(
            ServiceHandlers.HTTP_SCHEDULE,
            data=schedule_action.SerializeToString(),
            raise_for_status=raise_for_status,
        )

    def perform_old_remove_action_request(self, remove_action, raise_for_status=True):
        return self.perform_post_request(
            ServiceHandlers.HTTP_UNSCHEDULE,
            data=remove_action.SerializeToString(),
            raise_for_status=raise_for_status,
        )
