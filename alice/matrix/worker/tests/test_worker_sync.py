import pytest
import asyncio
import time

from alice.matrix.worker.tests.library.test_base import MatrixWorkerTestBase

from alice.protos.api.matrix.scheduled_action_pb2 import (
    TScheduledActionStatus,
)
from alice.protos.api.matrix.scheduler_api_pb2 import (
    TAddScheduledActionRequest,
)
from alice.matrix.scheduler.tests.library.proto_builder_helpers import (
    get_scheduled_action_meta,
    get_scheduled_action_spec,
    get_scheduled_action_status,

    get_scheduled_action_attempt_status,

    get_add_scheduled_action_request,
    get_remove_scheduled_action_request,

    get_retry_policy,
    get_send_once_policy,
    get_send_periodically_policy,
    get_start_policy,

    get_mock_action,

    get_proto_duration,
    get_proto_timestamp,
)

from alice.matrix.scheduler.tests.library.ydb import (
    get_action_rows_from_incoming_queue,
    get_action_data,
)
from alice.matrix.worker.tests.library.ydb import (
    get_action_rows_from_processing_queue,
    get_shard_lock_rows,

    unlock_all_shard_locks_and_set_shard_count,
)

from alice.matrix.library.testing.python.helpers import assert_numbers_are_almost_equal

from cityhash import hash64 as CityHash64


class TestWorkerSync(MatrixWorkerTestBase):
    worker_manual_sync_mode = True

    @pytest.mark.asyncio
    async def test_no_free_shards(
        self,
        matrix_scheduler,
        matrix_worker,
        ydb_session,
    ):
        unlock_all_shard_locks_and_set_shard_count(ydb_session, 0)
        await matrix_worker.perform_manual_sync_request()
        assert get_shard_lock_rows(ydb_session) == []

    @pytest.mark.asyncio
    async def test_fair_distribution(
        self,
        matrix_scheduler,
        matrix_worker,
        ydb_session,
    ):
        shard_locks_count = 5
        unlock_all_shard_locks_and_set_shard_count(ydb_session, shard_locks_count)

        # Check that the timestamps of everything are almost now with 10 seconds gap
        max_diff = 10 * 10**6
        current_time = time.time_ns() // 1000
        for i in range(shard_locks_count):
            await matrix_worker.perform_manual_sync_request()

            shard_lock_rows = get_shard_lock_rows(ydb_session)
            assert len(shard_lock_rows) == shard_locks_count

            synced_shards = 0
            for shard_lock_row in shard_lock_rows:
                assert 0 <= shard_lock_row["shard_id"] and shard_lock_row["shard_id"] < shard_locks_count

                if shard_lock_row["last_lock_guid"] != "none":
                    synced_shards += 1
                    assert_numbers_are_almost_equal(shard_lock_row["last_processing_start_at"], current_time, max_diff)
                    assert_numbers_are_almost_equal(shard_lock_row["last_heartbeat_at"], current_time, max_diff)

                assert shard_lock_row == {
                    "shard_id": shard_lock_row["shard_id"],

                    "locked": False,
                    "last_lock_guid": shard_lock_row["last_lock_guid"],
                    "last_locked_by": shard_lock_row["last_locked_by"],

                    "last_processing_start_at": shard_lock_row["last_processing_start_at"],
                    "last_heartbeat_at": shard_lock_row["last_heartbeat_at"],
                }

            assert synced_shards == i + 1

        shard_lock_rows = get_shard_lock_rows(ydb_session)
        assert len(shard_lock_rows) == shard_locks_count
        shard_id_to_shard_lock_info = [None for i in range(shard_locks_count)]
        for shard_lock_row in shard_lock_rows:
            shard_id_to_shard_lock_info[shard_lock_row["shard_id"]] = shard_lock_row

        for i in range(shard_locks_count):
            await matrix_worker.perform_manual_sync_request()

            shard_lock_rows = get_shard_lock_rows(ydb_session)
            assert len(shard_lock_rows) == shard_locks_count

            resynced_shards = 0
            for shard_lock_row in shard_lock_rows:
                assert 0 <= shard_lock_row["shard_id"] and shard_lock_row["shard_id"] < shard_locks_count
                resynced_shards += shard_lock_row["last_lock_guid"] != shard_id_to_shard_lock_info[shard_lock_row["shard_id"]]["last_lock_guid"]

            assert resynced_shards == i + 1

    @pytest.mark.asyncio
    async def test_simple_move_from_incoming_to_processing(
        self,
        matrix_scheduler,
        matrix_worker,
        ydb_session,
        action_id,
    ):
        unlock_all_shard_locks_and_set_shard_count(ydb_session, 1)

        scheduled_action_meta = get_scheduled_action_meta(action_id=action_id)
        # Don't process this action
        start_at_timestamp_ns = time.time_ns() + 10**18
        scheduled_action_spec = get_scheduled_action_spec(start_policy=get_start_policy(start_at=get_proto_timestamp(start_at_timestamp_ns)))
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
        )
        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)

        # Before sync
        # Action in incoming queue
        incoming_queue = get_action_rows_from_incoming_queue(ydb_session, action_id)
        assert len(incoming_queue) == 1
        incoming_action_row = incoming_queue[0]
        # Processing queue is empty
        assert get_action_rows_from_processing_queue(ydb_session, action_id) == []
        old_action_data = get_action_data(ydb_session, action_id)

        # Second sync must change nothing
        for i in range(2):
            await matrix_worker.perform_manual_sync_request()

            # After sync
            # Incoming queue is empty
            assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []

            # Action in processing queue
            processing_queue = get_action_rows_from_processing_queue(ydb_session, action_id)
            assert len(processing_queue) == 1
            processing_action_row = processing_queue[0]
            # Check that the timestamp of reschedule is almost now with 10 seconds gap
            assert_numbers_are_almost_equal(processing_action_row["last_reschedule_at"], time.time_ns() // 1000, 10 * 10**6)
            assert processing_action_row["last_reschedule_at"] == processing_action_row["moved_from_incoming_to_processing_queue_at"]
            assert processing_action_row["last_reschedule_by_sync_with_guid"] == processing_action_row["moved_from_incoming_to_processing_queue_by_sync_with_guid"]
            assert processing_action_row == {
                "shard_id": 0,
                "scheduled_at": incoming_action_row["scheduled_at"],
                "action_id": action_id,
                "action_guid": incoming_action_row["action_guid"],

                "added_to_incoming_queue_at": incoming_action_row["created_at"],

                "moved_from_incoming_to_processing_queue_by_sync_with_guid": processing_action_row["moved_from_incoming_to_processing_queue_by_sync_with_guid"],
                "moved_from_incoming_to_processing_queue_at": processing_action_row["moved_from_incoming_to_processing_queue_at"],

                "last_reschedule_by_sync_with_guid": processing_action_row["last_reschedule_by_sync_with_guid"],
                "last_reschedule_at": processing_action_row["last_reschedule_at"],
            }

            # Action data does not change
            assert old_action_data == get_action_data(ydb_session, action_id)

    @pytest.mark.asyncio
    @pytest.mark.parametrize("restart_period_ns", [2 * 10**9, 10**3 * 10**9])
    @pytest.mark.parametrize("max_retries, fail_until_consecutive_failures_counter_less_than", [
        # Remove by SendOncePolicySucceed
        (10, 1),
        # Remove by MaxAttemptsReached
        (1, 10),
    ])
    async def test_simple_send_once_policy(
        self,
        matrix_scheduler,
        matrix_worker,
        ydb_session,
        action_id,
        restart_period_ns,
        max_retries,
        fail_until_consecutive_failures_counter_less_than,
    ):
        unlock_all_shard_locks_and_set_shard_count(ydb_session, 1)

        scheduled_action_meta = get_scheduled_action_meta(action_id=action_id)
        # Start ASAP
        start_at_timestamp_ns = time.time_ns()
        scheduled_action_spec = get_scheduled_action_spec(
            start_policy=get_start_policy(
                start_at=get_proto_timestamp(start_at_timestamp_ns),
            ),
            send_policy=get_send_once_policy(
                retry_policy=get_retry_policy(
                    max_retries=max_retries,
                    min_restart_period=get_proto_duration(restart_period_ns),
                    max_restart_period=get_proto_duration(restart_period_ns),
                ),
            ),
            action=get_mock_action(
                fail_until_consecutive_failures_counter_less_than=fail_until_consecutive_failures_counter_less_than,
            ),
        )
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
        )
        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)

        # Second sync must change nothing (min_restart_period >= 2s)
        for i in range(2):
            await matrix_worker.perform_manual_sync_request()

            # Incoming queue is empty
            assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []

            # Processing queue is not empty
            processing_queue = get_action_rows_from_processing_queue(ydb_session, action_id)
            assert len(processing_queue) == 1
            processing_action_row = processing_queue[0]
            assert processing_action_row == {
                "shard_id": 0,
                "scheduled_at": processing_action_row["scheduled_at"],
                "action_id": action_id,
                "action_guid": processing_action_row["action_guid"],

                "added_to_incoming_queue_at": processing_action_row["added_to_incoming_queue_at"],

                "moved_from_incoming_to_processing_queue_by_sync_with_guid": processing_action_row["moved_from_incoming_to_processing_queue_by_sync_with_guid"],
                "moved_from_incoming_to_processing_queue_at": processing_action_row["moved_from_incoming_to_processing_queue_at"],

                "last_reschedule_by_sync_with_guid": processing_action_row["last_reschedule_by_sync_with_guid"],
                "last_reschedule_at": processing_action_row["last_reschedule_at"],
            }

            action_data = get_action_data(ydb_session, action_id)
            assert action_data == {
                "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
                "meta_action_id": action_id,

                "meta": get_scheduled_action_meta(action_id=action_id, action_guid=action_data["meta"].Guid),
                "meta_action_guid": action_data["meta"].Guid,

                "spec": scheduled_action_spec,

                "status": get_scheduled_action_status(
                    scheduled_at=get_proto_timestamp(processing_action_row["scheduled_at"] * 10**3),
                    failed_attempts_counter=1,
                    consecutive_failures_counter=1,
                    last_attempt_status=get_scheduled_action_attempt_status(
                        status=TScheduledActionStatus.TAttemptStatus.EStatus.ERROR,
                        error_message="Mock action failed: ConsecutiveFailuresCounter is 0"
                                      f", FailUntilConsecutiveFailuresCounterLessThan is {fail_until_consecutive_failures_counter_less_than}",
                    ),
                ),
                "status_scheduled_at": processing_action_row["scheduled_at"],
            }

        if restart_period_ns == 2 * 10**9:
            # Await restart and check that scheduled action is removed
            await asyncio.sleep(2)

            # Second sync must change nothing
            for i in range(2):
                await matrix_worker.perform_manual_sync_request()

                # Incoming queue is empty
                assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []

                # Processing queue is empty
                assert get_action_rows_from_processing_queue(ydb_session, action_id) == []

                # Action is deleted from database
                assert get_action_data(ydb_session, action_id) is None
        else:
            action_data = get_action_data(ydb_session, action_id)
            # Check that the timestamp of next scheduled_at is almost now + restart_period with 10 seconds gap
            assert_numbers_are_almost_equal(
                action_data["status_scheduled_at"],
                (time.time_ns() + restart_period_ns) // 1000,
                10 * 10**6,
            )

    @pytest.mark.asyncio
    @pytest.mark.parametrize("period_ns", [2 * 10**9, 10**3 * 10**9])
    async def test_simple_send_periodically_policy(
        self,
        matrix_scheduler,
        matrix_worker,
        ydb_session,
        action_id,
        period_ns,
    ):
        unlock_all_shard_locks_and_set_shard_count(ydb_session, 1)

        scheduled_action_meta = get_scheduled_action_meta(action_id=action_id)
        # Start ASAP
        start_at_timestamp_ns = time.time_ns()
        scheduled_action_spec = get_scheduled_action_spec(
            start_policy=get_start_policy(
                start_at=get_proto_timestamp(start_at_timestamp_ns),
            ),
            send_policy=get_send_periodically_policy(
                period=get_proto_duration(period_ns),
            ),
            action=get_mock_action(
                fail_until_consecutive_failures_counter_less_than=1,
            ),
        )
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
        )
        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)

        # Second sync must change nothing (period >= 2s)
        for i in range(2):
            await matrix_worker.perform_manual_sync_request()

            # Incoming queue is empty
            assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []

            # Processing queue is not empty
            processing_queue = get_action_rows_from_processing_queue(ydb_session, action_id)
            assert len(processing_queue) == 1
            processing_action_row = processing_queue[0]
            assert processing_action_row == {
                "shard_id": 0,
                "scheduled_at": processing_action_row["scheduled_at"],
                "action_id": action_id,
                "action_guid": processing_action_row["action_guid"],

                "added_to_incoming_queue_at": processing_action_row["added_to_incoming_queue_at"],

                "moved_from_incoming_to_processing_queue_by_sync_with_guid": processing_action_row["moved_from_incoming_to_processing_queue_by_sync_with_guid"],
                "moved_from_incoming_to_processing_queue_at": processing_action_row["moved_from_incoming_to_processing_queue_at"],

                "last_reschedule_by_sync_with_guid": processing_action_row["last_reschedule_by_sync_with_guid"],
                "last_reschedule_at": processing_action_row["last_reschedule_at"],
            }

            action_data = get_action_data(ydb_session, action_id)
            assert action_data == {
                "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
                "meta_action_id": action_id,

                "meta": get_scheduled_action_meta(action_id=action_id, action_guid=action_data["meta"].Guid),
                "meta_action_guid": action_data["meta"].Guid,

                "spec": scheduled_action_spec,

                "status": get_scheduled_action_status(
                    scheduled_at=get_proto_timestamp(processing_action_row["scheduled_at"] * 10**3),
                    failed_attempts_counter=1,
                    consecutive_failures_counter=1,
                    last_attempt_status=get_scheduled_action_attempt_status(
                        status=TScheduledActionStatus.TAttemptStatus.EStatus.ERROR,
                        error_message="Mock action failed: ConsecutiveFailuresCounter is 0"
                                      ", FailUntilConsecutiveFailuresCounterLessThan is 1",
                    ),
                ),
                "status_scheduled_at": processing_action_row["scheduled_at"],
            }

        if period_ns == 2 * 10**9:
            # Await restart and check status
            await asyncio.sleep(2)

            # Second sync must change nothing
            for i in range(2):
                await matrix_worker.perform_manual_sync_request()

                # Incoming queue is empty
                assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []

                # Processing queue is not empty
                processing_queue = get_action_rows_from_processing_queue(ydb_session, action_id)
                assert len(processing_queue) == 1
                processing_action_row = processing_queue[0]
                assert processing_action_row == {
                    "shard_id": 0,
                    "scheduled_at": processing_action_row["scheduled_at"],
                    "action_id": action_id,
                    "action_guid": processing_action_row["action_guid"],

                    "added_to_incoming_queue_at": processing_action_row["added_to_incoming_queue_at"],

                    "moved_from_incoming_to_processing_queue_by_sync_with_guid": processing_action_row["moved_from_incoming_to_processing_queue_by_sync_with_guid"],
                    "moved_from_incoming_to_processing_queue_at": processing_action_row["moved_from_incoming_to_processing_queue_at"],

                    "last_reschedule_by_sync_with_guid": processing_action_row["last_reschedule_by_sync_with_guid"],
                    "last_reschedule_at": processing_action_row["last_reschedule_at"],
                }

                action_data = get_action_data(ydb_session, action_id)
                assert action_data == {
                    "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
                    "meta_action_id": action_id,

                    "meta": get_scheduled_action_meta(action_id=action_id, action_guid=action_data["meta"].Guid),
                    "meta_action_guid": action_data["meta"].Guid,

                    "spec": scheduled_action_spec,

                    "status": get_scheduled_action_status(
                        scheduled_at=get_proto_timestamp(processing_action_row["scheduled_at"] * 10**3),
                        successful_attempts_counter=1,
                        failed_attempts_counter=1,
                        last_attempt_status=get_scheduled_action_attempt_status(
                            status=TScheduledActionStatus.TAttemptStatus.EStatus.SUCCESS,
                        ),
                    ),
                    "status_scheduled_at": processing_action_row["scheduled_at"],
                }
        else:
            action_data = get_action_data(ydb_session, action_id)
            # Check that the timestamp of next scheduled_at is almost now + period with 10 seconds gap
            assert_numbers_are_almost_equal(
                action_data["status_scheduled_at"],
                (time.time_ns() + period_ns) // 1000,
                10 * 10**6,
            )

    @pytest.mark.asyncio
    @pytest.mark.parametrize("deadline_type", [
        "in_the_past",
        "in_the_future",
    ])
    async def test_skip_and_delete_action_with_overdue_deadline(
        self,
        matrix_scheduler,
        matrix_worker,
        ydb_session,
        action_id,
        deadline_type,
    ):
        unlock_all_shard_locks_and_set_shard_count(ydb_session, 1)

        send_policy=get_send_periodically_policy(
            # Nerever restart
            period=get_proto_duration(10**3 * 10**9),
        )
        if deadline_type == "in_the_future":
            send_policy.Deadline.FromNanoseconds(time.time_ns() + 10**3 * 10**9)
        else:
            send_policy.Deadline.FromNanoseconds(time.time_ns() - 10**3 * 10**9)

        scheduled_action_meta = get_scheduled_action_meta(action_id=action_id)
        # Start ASAP
        start_at_timestamp_ns = time.time_ns()
        scheduled_action_spec = get_scheduled_action_spec(
            start_policy=get_start_policy(
                start_at=get_proto_timestamp(start_at_timestamp_ns),
            ),
            send_policy=send_policy,
        )
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
        )

        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)

        await matrix_worker.perform_manual_sync_request()

        if deadline_type == "in_the_past":
            # The action was not executed and was deleted from the incoming_queue, the processing_queue and sheduled_actions
            assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []
            assert get_action_rows_from_processing_queue(ydb_session, action_id) == []
            assert get_action_data(ydb_session, action_id) is None
        else:
            # The action was executed and was kept only in the processing_queue and sheduled_actions
            assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []

            processing_queue = get_action_rows_from_processing_queue(ydb_session, action_id)
            assert len(processing_queue) == 1
            processing_action_row = processing_queue[0]
            assert processing_action_row["shard_id"] == 0
            assert processing_action_row["action_id"] == action_id

            action_data = get_action_data(ydb_session, action_id)
            assert action_data == {
                "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
                "meta_action_id": action_id,

                "meta": get_scheduled_action_meta(action_id=action_id, action_guid=action_data["meta"].Guid),
                "meta_action_guid": action_data["meta_action_guid"],

                "spec": scheduled_action_spec,

                "status": get_scheduled_action_status(
                    scheduled_at=get_proto_timestamp(processing_action_row["scheduled_at"] * 10**3),
                    # Must be exactly one attempt
                    successful_attempts_counter=1,
                    last_attempt_status=get_scheduled_action_attempt_status(
                        status=TScheduledActionStatus.TAttemptStatus.EStatus.SUCCESS,
                    ),
                ),
                "status_scheduled_at": processing_action_row["scheduled_at"],
            }

    @pytest.mark.asyncio
    async def test_skip_actions_with_mismatched_guid(
        self,
        matrix_scheduler,
        matrix_worker,
        ydb_session,
        action_id,
    ):
        unlock_all_shard_locks_and_set_shard_count(ydb_session, 1)

        scheduled_action_meta = get_scheduled_action_meta(action_id=action_id)
        # Start ASAP
        start_at_timestamp_ns = time.time_ns()
        scheduled_action_spec = get_scheduled_action_spec(
            start_policy=get_start_policy(
                start_at=get_proto_timestamp(start_at_timestamp_ns),
            ),
            send_policy=get_send_periodically_policy(
                # Nerever restart
                period=get_proto_duration(10**3 * 10**9),
            ),
        )
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
            override_mode=TAddScheduledActionRequest.EOverrideMode.META_AND_SPEC_ONLY,
        )
        for i in range(5):
            await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)

        # Before sync
        # Action in incoming queue
        incoming_queue = get_action_rows_from_incoming_queue(ydb_session, action_id)
        assert len(incoming_queue) == 5
        assert set(incoming_action_row["action_id"] for incoming_action_row in incoming_queue) == {action_id}
        assert len(set(incoming_action_row["action_guid"] for incoming_action_row in incoming_queue)) == 5

        # Remember the actual guid of the action to compare it after sync
        action_guid_before_sync = get_action_data(ydb_session, action_id)["meta"].Guid

        # Processing queue is empty
        assert get_action_rows_from_processing_queue(ydb_session, action_id) == []

        await matrix_worker.perform_manual_sync_request()

        # After sync
        # Incoming queue is empty
        assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []

        # Processing queue is not empty
        processing_queue = get_action_rows_from_processing_queue(ydb_session, action_id)
        assert len(processing_queue) == 1
        processing_action_row = processing_queue[0]
        assert processing_action_row == {
            "shard_id": 0,
            "scheduled_at": processing_action_row["scheduled_at"],
            "action_id": action_id,
            "action_guid": action_guid_before_sync,

            "added_to_incoming_queue_at": processing_action_row["added_to_incoming_queue_at"],

            "moved_from_incoming_to_processing_queue_by_sync_with_guid": processing_action_row["moved_from_incoming_to_processing_queue_by_sync_with_guid"],
            "moved_from_incoming_to_processing_queue_at": processing_action_row["moved_from_incoming_to_processing_queue_at"],

            "last_reschedule_by_sync_with_guid": processing_action_row["last_reschedule_by_sync_with_guid"],
            "last_reschedule_at": processing_action_row["last_reschedule_at"],
        }

        action_data = get_action_data(ydb_session, action_id)
        assert action_data == {
            "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
            "meta_action_id": action_id,

            "meta": get_scheduled_action_meta(action_id=action_id, action_guid=action_guid_before_sync),
            "meta_action_guid": action_guid_before_sync,

            "spec": scheduled_action_spec,

            "status": get_scheduled_action_status(
                scheduled_at=get_proto_timestamp(processing_action_row["scheduled_at"] * 10**3),
                # Must be exactly one attempt
                successful_attempts_counter=1,
                last_attempt_status=get_scheduled_action_attempt_status(
                    status=TScheduledActionStatus.TAttemptStatus.EStatus.SUCCESS,
                ),
            ),
            "status_scheduled_at": processing_action_row["scheduled_at"],
        }

    @pytest.mark.asyncio
    async def test_skip_removed_action(
        self,
        matrix_scheduler,
        matrix_worker,
        ydb_session,
        action_id,
    ):
        unlock_all_shard_locks_and_set_shard_count(ydb_session, 1)

        scheduled_action_meta = get_scheduled_action_meta(action_id=action_id)
        # Start ASAP
        start_at_timestamp_ns = time.time_ns()
        scheduled_action_spec = get_scheduled_action_spec(
            start_policy=get_start_policy(
                start_at=get_proto_timestamp(start_at_timestamp_ns),
            ),
            send_policy=get_send_periodically_policy(
                # Nerever restart
                period=get_proto_duration(10**3 * 10**9),
            ),
        )
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
        )
        remove_scheduled_action_request = get_remove_scheduled_action_request(action_id)

        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)
        await matrix_scheduler.perform_remove_scheduled_action_request(remove_scheduled_action_request)

        assert len(get_action_rows_from_incoming_queue(ydb_session, action_id)) == 1
        assert get_action_rows_from_processing_queue(ydb_session, action_id) == []
        # The action was removed from the scheduled_actions
        assert get_action_data(ydb_session, action_id) is None

        await matrix_worker.perform_manual_sync_request()

        # The action was not executed; it was removed and thus not kept in the processing_queue
        assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []
        assert get_action_rows_from_processing_queue(ydb_session, action_id) == []
        assert get_action_data(ydb_session, action_id) is None
