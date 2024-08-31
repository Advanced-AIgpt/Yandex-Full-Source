import yatest.common

from .constants import ItemTypes, ServiceHandlers
from .proto_builder_helpers import get_manual_sync_request
from .ydb import init_matrix_worker_ydb

from alice.matrix.worker.library.services.worker.protos.private_api_pb2 import (
    TManualSyncResponse,
)

from alice.matrix.library.testing.python.servant_base import ServantBase


class MatrixWorker(ServantBase):
    def __init__(
            self,
            manual_sync_mode=False,
            ydb_init_shard_count=3,
            main_loop_threads=None,
            default_min_loop_interval=None,
            default_max_loop_interval=None,
            min_loop_interval_for_skipped_sync=None,
            max_loop_interval_for_skipped_sync=None,
            matrix_notificator_port=None,
            *args,
            **kwargs,
    ):
        super(MatrixWorker, self).__init__(*args, **kwargs)

        self._manual_sync_mode = manual_sync_mode
        self._ydb_init_shard_count = ydb_init_shard_count
        self._main_loop_threads = main_loop_threads
        self._default_min_loop_interval = default_min_loop_interval
        self._default_max_loop_interval = default_max_loop_interval
        self._min_loop_interval_for_skipped_sync = min_loop_interval_for_skipped_sync
        self._max_loop_interval_for_skipped_sync = max_loop_interval_for_skipped_sync
        self._matrix_notificator_port = matrix_notificator_port

    def _get_config(self):
        config = self._get_default_config()
        config.update({
            "WorkerService": {
                "WorkerLoop": {
                    "YDBClient": self._get_default_ydb_client_config(),
                    "ManualMode": self._manual_sync_mode,
                    "MinEnsureShardLockLeadingAndDoHeartbeatPeriod": "30ms",

                    "MatrixNotificatorClient": {
                        "Host": "http://localhost",
                        "Port": self._matrix_notificator_port or 0,
                    },
                },
            },
        })

        if self._main_loop_threads is not None:
            config["WorkerService"]["WorkerLoop"]["MainLoopThreads"] = self._main_loop_threads

        if self._default_min_loop_interval is not None:
            config["WorkerService"]["WorkerLoop"]["DefaultMinLoopInterval"] = self._default_min_loop_interval

        if self._default_max_loop_interval is not None:
            config["WorkerService"]["WorkerLoop"]["DefaultMaxLoopInterval"] = self._default_max_loop_interval

        if self._min_loop_interval_for_skipped_sync is not None:
            config["WorkerService"]["WorkerLoop"]["MinLoopIntervalForSkippedSync"] = self._min_loop_interval_for_skipped_sync

        if self._max_loop_interval_for_skipped_sync is not None:
            config["WorkerService"]["WorkerLoop"]["MaxLoopIntervalForSkippedSync"] = self._max_loop_interval_for_skipped_sync

        return config

    def _before_start(self):
        init_matrix_worker_ydb(self._ydb_init_shard_count)

    @property
    def name(self):
        return "matrix_worker"

    @property
    def _binary_file_path(self):
        return yatest.common.build_path("alice/matrix/worker/bin/matrix_worker")

    async def perform_manual_sync_request(self, manual_sync_request=None, timeout=10, return_apphost_response_as_is=False):
        if manual_sync_request is None:
            manual_sync_request = get_manual_sync_request()

        response = await self.perform_grpc_request(
            items={
                ItemTypes.MANUAL_SYNC_REQUEST: [
                    manual_sync_request,
                ]
            },
            path=ServiceHandlers.MANUAL_SYNC,
            timeout=timeout,
        )

        if return_apphost_response_as_is:
            return response

        assert not response.has_exception(), f"Manual sync request failed with exception: {response.get_exception()}"

        response_items = list(response.get_item_datas(item_type=ItemTypes.MANUAL_SYNC_RESPONSE, proto_type=TManualSyncResponse))
        assert len(response_items) == 1
        return response_items[0]
