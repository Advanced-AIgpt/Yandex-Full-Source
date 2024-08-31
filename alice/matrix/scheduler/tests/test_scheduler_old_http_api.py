import pytest
import time

from alice.matrix.scheduler.tests.library.test_base import MatrixSchedulerTestBase

from alice.matrix.scheduler.tests.library.proto_builder_helpers import (
    get_old_schedule_action,
    get_old_remove_action,

    get_old_send_periodically_policy,
    get_old_start_policy,
)
from alice.matrix.scheduler.tests.library.ydb import get_action_rows_from_incoming_queue, get_action_data

from alice.matrix.library.testing.python.helpers import assert_numbers_are_almost_equal

from cityhash import hash64 as CityHash64


class TestSchedulerOldHttpApi(MatrixSchedulerTestBase):

    @pytest.mark.asyncio
    @pytest.mark.parametrize("with_override_as_add", [False, True])
    async def test_simple(
        self,
        matrix_scheduler,
        ydb_session,
        action_id,
        with_override_as_add,
    ):
        assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []
        assert get_action_data(ydb_session, action_id) is None

        start_at_timestamp_ns = time.time_ns() + 1337 * 10**9
        schedule_action = get_old_schedule_action(
            action_id=action_id,
            start_policy=get_old_start_policy(start_at_timestamp_ms=start_at_timestamp_ns // 10**6),
            override=with_override_as_add,
        )
        matrix_scheduler.perform_old_schedule_action_request(schedule_action)

        incoming_queue = get_action_rows_from_incoming_queue(ydb_session, action_id)
        assert len(incoming_queue) == 1
        action_row = incoming_queue[0]
        # Check that the timestamp of the row creation is almost now (with 10 seconds gap)
        assert_numbers_are_almost_equal(action_row["created_at"], time.time_ns() // 1000, 10 * 10**6)
        assert len(action_row["action_guid"]) > 0
        assert action_row == {
            "shard_id": 0,
            "created_at": action_row["created_at"],
            "action_id": action_id,
            "action_guid": action_row["action_guid"],

            "scheduled_at": (start_at_timestamp_ns // 10**6) * 1000,
        }

        action_data = get_action_data(ydb_session, action_id)
        assert action_data["meta"].Guid == action_row["action_guid"]
        assert action_data == {
            "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
            "meta_action_id": action_id,

            "meta": action_data["meta"],
            "meta_action_guid": action_data["meta"].Guid,

            "spec": action_data["spec"],

            "status": action_data["status"],
            "status_scheduled_at": (start_at_timestamp_ns // 10**6) * 1000,
        }

        # Test duplicate
        schedule_action.Override = False
        response = matrix_scheduler.perform_old_schedule_action_request(schedule_action, raise_for_status=False)
        assert response.status_code == 500
        assert f"Action with id {action_id} already exists" in response.text

        # Database not updated
        assert incoming_queue == get_action_rows_from_incoming_queue(ydb_session, action_id)
        assert action_data == get_action_data(ydb_session, action_id)

        for i in range(2):
            # Unschedule twice is ok
            matrix_scheduler.perform_old_remove_action_request(get_old_remove_action(action_id))

            # Unschedule does nothing to incoming queue
            assert incoming_queue == get_action_rows_from_incoming_queue(ydb_session, action_id)
            assert get_action_data(ydb_session, action_id) is None

    @pytest.mark.asyncio
    async def test_override(
        self,
        matrix_scheduler,
        ydb_session,
        action_id,
    ):
        start_at_timestamp_ns = time.time_ns() + 1337 * 10**9
        schedule_action = get_old_schedule_action(
            action_id=action_id,
            start_policy=get_old_start_policy(start_at_timestamp_ms=start_at_timestamp_ns // 10**6),
        )
        matrix_scheduler.perform_old_schedule_action_request(schedule_action)

        incoming_queue = get_action_rows_from_incoming_queue(ydb_session, action_id)
        assert len(incoming_queue) == 1
        old_action_row = incoming_queue[0]
        assert old_action_row == {
            "shard_id": 0,
            "created_at": old_action_row["created_at"],
            "action_id": action_id,
            "action_guid": old_action_row["action_guid"],

            "scheduled_at": (start_at_timestamp_ns // 10**6) * 1000,
        }

        old_action_data = get_action_data(ydb_session, action_id)
        assert old_action_data["meta"].Guid == old_action_row["action_guid"]
        assert old_action_data == {
            "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
            "meta_action_id": action_id,

            "meta": old_action_data["meta"],
            "meta_action_guid": old_action_data["meta"].Guid,

            "spec": old_action_data["spec"],

            "status": old_action_data["status"],
            "status_scheduled_at": (start_at_timestamp_ns // 10**6) * 1000,
        }

        other_start_at_timestamp_ns = start_at_timestamp_ns + 10**18
        schedule_action.StartPolicy.StartAtTimestampMs = other_start_at_timestamp_ns // 10**6
        schedule_action.Override = True
        matrix_scheduler.perform_old_schedule_action_request(schedule_action)

        incoming_queue = get_action_rows_from_incoming_queue(ydb_session, action_id)
        assert len(incoming_queue) == 2
        assert incoming_queue[0] == old_action_row or incoming_queue[1] == old_action_row
        new_action_row = incoming_queue[0] if incoming_queue[1] == old_action_row else incoming_queue[1]

        assert new_action_row["action_guid"] != old_action_row["action_guid"]
        assert new_action_row == {
            "shard_id": 0,
            "created_at": new_action_row["created_at"],
            "action_id": action_id,
            "action_guid": new_action_row["action_guid"],

            "scheduled_at": (start_at_timestamp_ns // 10**6) * 1000,
        }

        new_action_data = get_action_data(ydb_session, action_id)
        assert new_action_data["meta"].Guid == new_action_row["action_guid"]
        assert new_action_data == {
            "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
            "meta_action_id": action_id,

            "meta": new_action_data["meta"],
            "meta_action_guid": new_action_data["meta"].Guid,

            "spec": new_action_data["spec"],

            "status": new_action_data["status"],
            "status_scheduled_at": (start_at_timestamp_ns // 10**6) * 1000,
        }

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_action_id", "Action id must be non-empty"),
        ("no_send_policy", "Send policy type is not specified"),
        ("send_once_min_restart_period_greater_than_max", "SendOncePolicy's min restart period is greater than max restart period (10.001000s > 10.000000s)"),
        ("too_small_send_once_min_restart_period", "SendOncePolicy's min restart period is less than 1.000000s, actual value is 0.001000s"),
        ("too_small_send_periodically_period", "SendPeriodicallyPolicy's period is less than 1.000000s, actual value is 0.001000s"),
        ("no_action", "Action type is not specified"),
    ])
    async def test_invalid_schedule_request(
        self,
        matrix_scheduler,
        ydb_session,
        action_id,
        error_type,
        error_message,
    ):
        schedule_action = get_old_schedule_action(action_id=action_id)

        if error_type == "no_action_id":
            action_id = ""
            schedule_action.Id = ""
        elif error_type == "no_send_policy":
            schedule_action.ClearField("SendPolicy")
        elif error_type == "send_once_min_restart_period_greater_than_max":
            schedule_action.SendPolicy.SendOncePolicy.RetryPolicy.MinRestartPeriodMs = schedule_action.SendPolicy.SendOncePolicy.RetryPolicy.MaxRestartPeriodMs + 1
        elif error_type == "too_small_send_once_min_restart_period":
            schedule_action.SendPolicy.SendOncePolicy.RetryPolicy.MinRestartPeriodMs = 1
        elif error_type == "too_small_send_periodically_period":
            schedule_action = get_old_schedule_action(action_id=action_id, send_policy=get_old_send_periodically_policy(period_ms=1))
        elif error_type == "no_action":
            schedule_action.ClearField("Action")
        else:
            assert False, f"Unknown error_type {error_type}"

        response = matrix_scheduler.perform_old_schedule_action_request(schedule_action, raise_for_status=False)
        assert response.status_code == 400
        assert error_message in response.text

        assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []
        assert get_action_data(ydb_session, action_id) is None

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_action_id", "Action id must be non-empty"),
    ])
    async def test_invalid_unschedule_request(
        self,
        matrix_scheduler,
        ydb_session,
        action_id,
        error_type,
        error_message,
    ):
        remove_action = get_old_remove_action(
            action_id=action_id
        )

        if error_type == "no_action_id":
            action_id = ""
            remove_action.ActionId = ""
        else:
            assert False, f"Unknown error_type {error_type}"

        response = matrix_scheduler.perform_old_remove_action_request(remove_action, raise_for_status=False)
        assert response.status_code == 400
        assert error_message in response.text

        assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []
        assert get_action_data(ydb_session, action_id) is None
