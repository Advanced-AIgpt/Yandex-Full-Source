import pytest
import asyncio
import time

from alice.matrix.worker.tests.library.test_base import MatrixWorkerTestBase

from alice.matrix.scheduler.tests.library.proto_builder_helpers import (
    get_scheduled_action_meta,
    get_scheduled_action_spec,
    get_scheduled_action_status,

    get_add_scheduled_action_request,

    get_start_policy,

    get_proto_timestamp,
)

from alice.matrix.scheduler.tests.library.ydb import (
    get_action_rows_from_incoming_queue,
    get_action_data,
)
from alice.matrix.worker.tests.library.ydb import (
    get_action_rows_from_processing_queue,
    get_shard_lock_rows,
)

from alice.matrix.library.testing.python.helpers import assert_numbers_are_almost_equal

from cityhash import hash64 as CityHash64


class TestWorkerLoopMode(MatrixWorkerTestBase):
    worker_ydb_init_shard_count = 8
    worker_main_loop_threads = 4
    worker_default_min_loop_interval = "100ms"
    worker_default_max_loop_interval = "200ms"
    worker_min_loop_interval_for_skipped_sync = "50ms"
    worker_max_loop_interval_for_skipped_sync = "100ms"

    @pytest.mark.asyncio
    async def test_simple(
        self,
        matrix_scheduler,
        matrix_worker,
        ydb_session,
        action_id,
    ):
        # First action
        scheduled_action_meta = get_scheduled_action_meta(action_id=action_id)
        # Don't process this action
        start_at_timestamp_ns = time.time_ns() + 10**18
        scheduled_action_spec = get_scheduled_action_spec(start_policy=get_start_policy(start_at=get_proto_timestamp(start_at_timestamp_ns)))
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
        )
        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)

        # Second action
        action_id_other = action_id + "_other"
        scheduled_action_other_meta = get_scheduled_action_meta(action_id=action_id_other)
        # Process this action asap
        scheduled_action_other_spec = get_scheduled_action_spec(start_policy=get_start_policy(start_at=get_proto_timestamp(time.time_ns())))
        add_scheduled_action_other_request = get_add_scheduled_action_request(
            meta=scheduled_action_other_meta,
            spec=scheduled_action_other_spec,
        )
        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_other_request)

        # Await sync
        await asyncio.sleep(4)

        current_time = time.time_ns() // 1000
        # Check that the timestamps of everything are almost now with 10 seconds gap
        max_diff = 10 * 10**6

        # After sync
        shard_lock_rows = get_shard_lock_rows(ydb_session)
        assert len(shard_lock_rows) == 8

        locked_shards = 0
        for shard_lock_row in shard_lock_rows:
            assert 0 <= shard_lock_row["shard_id"] and shard_lock_row["shard_id"] < 8

            assert_numbers_are_almost_equal(shard_lock_row["last_processing_start_at"], current_time, max_diff)
            assert_numbers_are_almost_equal(shard_lock_row["last_heartbeat_at"], current_time, max_diff)

            locked_shards += shard_lock_row["locked"]

        assert locked_shards <= 4

        # Incoming queue is empty
        assert get_action_rows_from_incoming_queue(ydb_session, action_id_other) == []
        assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []

        # Only first action in processing queue
        assert get_action_rows_from_processing_queue(ydb_session, action_id_other) == []
        processing_queue = get_action_rows_from_processing_queue(ydb_session, action_id)
        assert len(processing_queue) == 1
        processing_action_row = processing_queue[0]

        assert_numbers_are_almost_equal(processing_action_row["added_to_incoming_queue_at"], current_time, max_diff)
        assert_numbers_are_almost_equal(processing_action_row["moved_from_incoming_to_processing_queue_at"], current_time, max_diff)
        assert processing_action_row["moved_from_incoming_to_processing_queue_at"] == processing_action_row["last_reschedule_at"]
        assert processing_action_row["moved_from_incoming_to_processing_queue_by_sync_with_guid"] == processing_action_row["last_reschedule_by_sync_with_guid"]
        assert processing_action_row == {
            "shard_id": 0,
            "scheduled_at": start_at_timestamp_ns // 1000,
            "action_id": action_id,
            "action_guid": processing_action_row["action_guid"],

            "added_to_incoming_queue_at": processing_action_row["added_to_incoming_queue_at"],

            "moved_from_incoming_to_processing_queue_by_sync_with_guid": processing_action_row["moved_from_incoming_to_processing_queue_by_sync_with_guid"],
            "moved_from_incoming_to_processing_queue_at": processing_action_row["moved_from_incoming_to_processing_queue_at"],

            "last_reschedule_by_sync_with_guid": processing_action_row["last_reschedule_by_sync_with_guid"],
            "last_reschedule_at": processing_action_row["last_reschedule_at"],
        }

        # Check action data
        assert get_action_data(ydb_session, action_id_other) is None
        action_data = get_action_data(ydb_session, action_id)
        assert action_data["meta"].Guid == processing_action_row["action_guid"]
        assert action_data == {
            "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
            "meta_action_id": action_id,

            "meta": get_scheduled_action_meta(action_id=action_id, action_guid=processing_action_row["action_guid"]),
            "meta_action_guid": action_data["meta"].Guid,

            "spec": scheduled_action_spec,

            "status": get_scheduled_action_status(get_proto_timestamp(start_at_timestamp_ns, round_by=1000)),
            "status_scheduled_at": start_at_timestamp_ns // 1000,
        }

    @pytest.mark.asyncio
    async def test_manual_sync_is_disabled(
        self,
        matrix_scheduler,
        matrix_worker,
    ):
        response = await matrix_worker.perform_manual_sync_request(return_apphost_response_as_is=True)

        assert response.has_exception()
        assert b"Manual sync mode is disabled" in response.get_exception()
