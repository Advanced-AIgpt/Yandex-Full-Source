import pytest
import time

from alice.matrix.scheduler.tests.library.test_base import MatrixSchedulerTestBase

from alice.protos.api.matrix.scheduler_api_pb2 import (
    TAddScheduledActionRequest,
)
from alice.matrix.scheduler.tests.library.proto_builder_helpers import (
    get_scheduled_action_meta,
    get_scheduled_action_spec,
    get_scheduled_action_status,

    get_add_scheduled_action_request,
    get_remove_scheduled_action_request,

    get_send_periodically_policy,
    get_start_policy,

    get_proto_duration,
    get_proto_timestamp,
)
from alice.matrix.scheduler.tests.library.ydb import get_action_rows_from_incoming_queue, get_action_data

from alice.matrix.library.testing.python.helpers import assert_numbers_are_almost_equal

from cityhash import hash64 as CityHash64


class TestScheduler(MatrixSchedulerTestBase):

    @pytest.mark.asyncio
    @pytest.mark.parametrize("override_mode", [
        TAddScheduledActionRequest.EOverrideMode.NONE,
        TAddScheduledActionRequest.EOverrideMode.META_AND_SPEC_ONLY,
        TAddScheduledActionRequest.EOverrideMode.ALL,
    ])
    async def test_simple_add_remove(
        self,
        matrix_scheduler,
        ydb_session,
        action_id,
        override_mode,
    ):
        assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []
        assert get_action_data(ydb_session, action_id) is None

        start_at_timestamp_ns = time.time_ns() + 1337 * 10**9
        scheduled_action_meta = get_scheduled_action_meta(action_id=action_id)
        scheduled_action_spec = get_scheduled_action_spec(start_policy=get_start_policy(start_at=get_proto_timestamp(start_at_timestamp_ns)))
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
            override_mode=override_mode,
        )
        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)

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

            "scheduled_at": start_at_timestamp_ns // 1000,
        }

        action_data = get_action_data(ydb_session, action_id)
        assert action_data["meta"].Guid == action_row["action_guid"]
        assert action_data == {
            "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
            "meta_action_id": action_id,

            "meta": get_scheduled_action_meta(action_id=action_id, action_guid=action_row["action_guid"]),
            "meta_action_guid": action_data["meta"].Guid,

            "spec": scheduled_action_spec,

            "status": get_scheduled_action_status(scheduled_at=get_proto_timestamp(start_at_timestamp_ns, round_by=1000)),
            "status_scheduled_at": start_at_timestamp_ns // 1000,
        }

        # Test duplicate
        add_scheduled_action_request.OverrideMode = TAddScheduledActionRequest.EOverrideMode.NONE
        response = await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request, return_apphost_response_as_is=True)
        assert response.has_exception()
        assert f"Action with id {action_id} already exists".encode("utf-8") in response.get_exception()

        # Database not updated
        assert incoming_queue == get_action_rows_from_incoming_queue(ydb_session, action_id)
        assert action_data == get_action_data(ydb_session, action_id)

        for i in range(2):
            # Remove same scheduled action twice is ok
            await matrix_scheduler.perform_remove_scheduled_action_request(get_remove_scheduled_action_request(action_id=action_id))

            # Remove scheduled action request does nothing to incoming queue
            assert incoming_queue == get_action_rows_from_incoming_queue(ydb_session, action_id)
            assert get_action_data(ydb_session, action_id) is None

    @pytest.mark.asyncio
    async def test_multi_add_remove(
        self,
        matrix_scheduler,
        ydb_session,
        action_id,
    ):
        start_at_timestamp_ns = time.time_ns() + 1337 * 10**9

        override_modes = [
            TAddScheduledActionRequest.EOverrideMode.NONE,
            TAddScheduledActionRequest.EOverrideMode.META_AND_SPEC_ONLY,
            TAddScheduledActionRequest.EOverrideMode.ALL,
        ]

        add_scheduled_action_requests = []
        for i in range(9):
            add_scheduled_action_requests.append(
                get_add_scheduled_action_request(
                    meta=get_scheduled_action_meta(action_id=f"{action_id}_{i}"),
                    spec=get_scheduled_action_spec(
                        start_policy=get_start_policy(
                            start_at=get_proto_timestamp(start_at_timestamp_ns + i * 10**6),
                        ),
                    ),
                    override_mode=override_modes[i % len(override_modes)],
                ),
            )

        await matrix_scheduler.perform_add_scheduled_action_requests(add_scheduled_action_requests)

        incoming_queues = []
        action_datas = []
        for i in range(len(add_scheduled_action_requests)):
            current_action_id = f"{action_id}_{i}"

            incoming_queue = get_action_rows_from_incoming_queue(ydb_session, current_action_id)
            assert len(incoming_queue) == 1
            action_row = incoming_queue[0]
            # Check that the timestamp of the row creation is almost now (with 10 seconds gap)
            assert_numbers_are_almost_equal(action_row["created_at"], time.time_ns() // 1000, 10 * 10**6)
            assert len(action_row["action_guid"]) > 0

            assert action_row == {
                "shard_id": 0,
                "created_at": action_row["created_at"],
                "action_id": current_action_id,
                "action_guid": action_row["action_guid"],

                "scheduled_at": (start_at_timestamp_ns + i * 10**6) // 1000,
            }

            action_data = get_action_data(ydb_session, current_action_id)
            assert action_data["meta"].Guid == action_row["action_guid"]
            assert action_data == {
                "meta_action_id_hash": CityHash64(current_action_id.encode("utf-8")),
                "meta_action_id": current_action_id,

                "meta": get_scheduled_action_meta(action_id=current_action_id, action_guid=action_row["action_guid"]),
                "meta_action_guid": action_data["meta"].Guid,

                "spec": add_scheduled_action_requests[i].Spec,

                "status": get_scheduled_action_status(scheduled_at=get_proto_timestamp(start_at_timestamp_ns + i * 10**6, round_by=1000)),
                "status_scheduled_at": (start_at_timestamp_ns + i * 10**6) // 1000,
            }

            incoming_queues.append(incoming_queue)
            action_datas.append(action_data)

        # Test duplicate
        response = await matrix_scheduler.perform_add_scheduled_action_requests(add_scheduled_action_requests, return_apphost_response_as_is=True)
        assert response.has_exception()
        assert b"Action with id " in response.get_exception()
        assert b" already exists" in response.get_exception()

        # Database not updated
        for i in range(len(add_scheduled_action_requests)):
            current_action_id = f"{action_id}_{i}"
            assert incoming_queues[i] == get_action_rows_from_incoming_queue(ydb_session, current_action_id)
            assert action_datas[i] == get_action_data(ydb_session, current_action_id)

        for i in range(3):
            remove_scheduled_action_requests = []
            for j in range(len(add_scheduled_action_requests)):
                current_action_id = f"{action_id}_{j}"
                if j == 1 or (i > 0 and j % 2 == 0):
                    remove_scheduled_action_requests.append(
                        get_remove_scheduled_action_request(action_id=current_action_id),
                    )

            await matrix_scheduler.perform_remove_scheduled_action_requests(remove_scheduled_action_requests)

            for j in range(len(add_scheduled_action_requests)):
                current_action_id = f"{action_id}_{j}"

                # Remove scheduled action request does nothing to incoming queue
                assert incoming_queues[j] == get_action_rows_from_incoming_queue(ydb_session, current_action_id)
                if j == 1 or (i > 0 and j % 2 == 0):
                    assert get_action_data(ydb_session, current_action_id) is None
                else:
                    assert action_datas[j] == get_action_data(ydb_session, current_action_id)

    @pytest.mark.asyncio
    @pytest.mark.parametrize("override_mode", [
        TAddScheduledActionRequest.EOverrideMode.META_AND_SPEC_ONLY,
        TAddScheduledActionRequest.EOverrideMode.ALL,
    ])
    async def test_simple_override(
        self,
        matrix_scheduler,
        ydb_session,
        action_id,
        override_mode,
    ):
        start_at_timestamp_ns = time.time_ns() + 1337 * 10**9
        scheduled_action_meta = get_scheduled_action_meta(action_id=action_id)
        scheduled_action_spec = get_scheduled_action_spec(start_policy=get_start_policy(start_at=get_proto_timestamp(start_at_timestamp_ns)))
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
        )
        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)

        incoming_queue = get_action_rows_from_incoming_queue(ydb_session, action_id)
        assert len(incoming_queue) == 1
        old_action_row = incoming_queue[0]
        assert old_action_row == {
            "shard_id": 0,
            "created_at": old_action_row["created_at"],
            "action_id": action_id,
            "action_guid": old_action_row["action_guid"],

            "scheduled_at": start_at_timestamp_ns // 1000,
        }

        old_action_data = get_action_data(ydb_session, action_id)
        assert old_action_data["meta"].Guid == old_action_row["action_guid"]
        assert old_action_data == {
            "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
            "meta_action_id": action_id,

            "meta": get_scheduled_action_meta(action_id=action_id, action_guid=old_action_row["action_guid"]),
            "meta_action_guid": old_action_data["meta"].Guid,

            "spec": scheduled_action_spec,

            "status": get_scheduled_action_status(scheduled_at=get_proto_timestamp(start_at_timestamp_ns, round_by=1000)),
            "status_scheduled_at": start_at_timestamp_ns // 1000,
        }

        other_start_at_timestamp_ns = start_at_timestamp_ns + 10**18
        scheduled_action_spec.StartPolicy.StartAt.CopyFrom(get_proto_timestamp(other_start_at_timestamp_ns))
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
            override_mode=override_mode,
        )
        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)

        incoming_queue = get_action_rows_from_incoming_queue(ydb_session, action_id)
        assert len(incoming_queue) == 2
        assert incoming_queue[0] == old_action_row or incoming_queue[1] == old_action_row
        new_action_row = incoming_queue[0] if incoming_queue[1] == old_action_row else incoming_queue[1]

        if override_mode == TAddScheduledActionRequest.EOverrideMode.META_AND_SPEC_ONLY:
            expected_scheduled_at_ns = start_at_timestamp_ns
        elif override_mode == TAddScheduledActionRequest.EOverrideMode.ALL:
            expected_scheduled_at_ns = other_start_at_timestamp_ns
        else:
            assert False, f"Unknown override_mode {override_mode}"

        assert new_action_row["action_guid"] != old_action_row["action_guid"]
        assert new_action_row == {
            "shard_id": 0,
            "created_at": new_action_row["created_at"],
            "action_id": action_id,
            "action_guid": new_action_row["action_guid"],

            "scheduled_at": expected_scheduled_at_ns // 1000,
        }

        new_action_data = get_action_data(ydb_session, action_id)
        assert new_action_data["meta"].Guid == new_action_row["action_guid"]
        assert new_action_data == {
            "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
            "meta_action_id": action_id,

            "meta": get_scheduled_action_meta(action_id=action_id, action_guid=new_action_row["action_guid"]),
            "meta_action_guid": new_action_data["meta"].Guid,

            "spec": scheduled_action_spec,

            "status": get_scheduled_action_status(scheduled_at=get_proto_timestamp(expected_scheduled_at_ns, round_by=1000)),
            "status_scheduled_at": expected_scheduled_at_ns // 1000,
        }

    @pytest.mark.asyncio
    async def test_multi_override(
        self,
        matrix_scheduler,
        ydb_session,
        action_id,
    ):
        start_at_timestamp_ns = time.time_ns() + 1337 * 10**9

        add_scheduled_action_requests = []
        for i in range(10):
            add_scheduled_action_requests.append(
                get_add_scheduled_action_request(
                    meta=get_scheduled_action_meta(action_id=f"{action_id}_{i}"),
                    spec=get_scheduled_action_spec(
                        start_policy=get_start_policy(
                            start_at=get_proto_timestamp(start_at_timestamp_ns + i * 10**6),
                        ),
                    ),
                ),
            )

        await matrix_scheduler.perform_add_scheduled_action_requests(add_scheduled_action_requests)

        old_incoming_queues = []
        old_action_datas = []
        for i in range(len(add_scheduled_action_requests)):
            current_action_id = f"{action_id}_{i}"

            incoming_queue = get_action_rows_from_incoming_queue(ydb_session, current_action_id)
            assert len(incoming_queue) == 1
            action_row = incoming_queue[0]
            assert action_row == {
                "shard_id": 0,
                "created_at": action_row["created_at"],
                "action_id": current_action_id,
                "action_guid": action_row["action_guid"],

                "scheduled_at": (start_at_timestamp_ns + i * 10**6) // 1000,
            }

            action_data = get_action_data(ydb_session, current_action_id)
            assert action_data["meta"].Guid == action_row["action_guid"]
            assert action_data == {
                "meta_action_id_hash": CityHash64(current_action_id.encode("utf-8")),
                "meta_action_id": current_action_id,

                "meta": get_scheduled_action_meta(action_id=current_action_id, action_guid=action_row["action_guid"]),
                "meta_action_guid": action_data["meta"].Guid,

                "spec": add_scheduled_action_requests[i].Spec,

                "status": get_scheduled_action_status(scheduled_at=get_proto_timestamp(start_at_timestamp_ns + i * 10**6, round_by=1000)),
                "status_scheduled_at": (start_at_timestamp_ns + i * 10**6) // 1000,
            }

            old_incoming_queues.append(incoming_queue)
            old_action_datas.append(action_data)

        override_modes = [
            TAddScheduledActionRequest.EOverrideMode.META_AND_SPEC_ONLY,
            TAddScheduledActionRequest.EOverrideMode.ALL,
        ]

        other_start_at_timestamp_ns = start_at_timestamp_ns + 10**18
        new_add_scheduled_action_requests = []
        for i in range(5, 15):
            new_add_scheduled_action_requests.append(
                get_add_scheduled_action_request(
                    meta=get_scheduled_action_meta(action_id=f"{action_id}_{i}"),
                    spec=get_scheduled_action_spec(
                        start_policy=get_start_policy(
                            start_at=get_proto_timestamp(other_start_at_timestamp_ns + i * 10**6),
                        ),
                    ),
                    override_mode=override_modes[i % len(override_modes)],
                ),
            )

        await matrix_scheduler.perform_add_scheduled_action_requests(new_add_scheduled_action_requests)

        for i in range(5):
            current_action_id = f"{action_id}_{i}"
            assert old_incoming_queues[i] == get_action_rows_from_incoming_queue(ydb_session, current_action_id)
            assert old_action_datas[i] == get_action_data(ydb_session, current_action_id)

        for i in range(5, 10):
            current_action_id = f"{action_id}_{i}"

            incoming_queue = get_action_rows_from_incoming_queue(ydb_session, current_action_id)
            assert len(incoming_queue) == 2
            assert incoming_queue[0] == old_incoming_queues[i][0] or incoming_queue[1] == old_incoming_queues[i][0]
            action_row = incoming_queue[0] if incoming_queue[1] == old_incoming_queues[i][0] else incoming_queue[1]

            override_mode = override_modes[i % len(override_modes)]
            if override_mode == TAddScheduledActionRequest.EOverrideMode.META_AND_SPEC_ONLY:
                expected_scheduled_at_ns = start_at_timestamp_ns + i * 10**6
            elif override_mode == TAddScheduledActionRequest.EOverrideMode.ALL:
                expected_scheduled_at_ns = other_start_at_timestamp_ns + i * 10**6
            else:
                assert False, f"Unknown override_mode {override_mode}"

            assert action_row["action_guid"] != old_incoming_queues[i][0]["action_guid"]
            assert action_row == {
                "shard_id": 0,
                "created_at": action_row["created_at"],
                "action_id": current_action_id,
                "action_guid": action_row["action_guid"],

                "scheduled_at": expected_scheduled_at_ns // 1000,
            }

            action_data = get_action_data(ydb_session, current_action_id)
            assert action_data["meta"].Guid == action_row["action_guid"]
            assert action_data == {
                "meta_action_id_hash": CityHash64(current_action_id.encode("utf-8")),
                "meta_action_id": current_action_id,

                "meta": get_scheduled_action_meta(action_id=current_action_id, action_guid=action_row["action_guid"]),
                "meta_action_guid": action_data["meta"].Guid,

                "spec": new_add_scheduled_action_requests[i - 5].Spec,

                "status": get_scheduled_action_status(scheduled_at=get_proto_timestamp(expected_scheduled_at_ns, round_by=1000)),
                "status_scheduled_at": expected_scheduled_at_ns // 1000,
            }

        for i in range(10, 15):
            current_action_id = f"{action_id}_{i}"

            incoming_queue = get_action_rows_from_incoming_queue(ydb_session, current_action_id)
            assert len(incoming_queue) == 1
            action_row = incoming_queue[0]
            assert action_row == {
                "shard_id": 0,
                "created_at": action_row["created_at"],
                "action_id": current_action_id,
                "action_guid": action_row["action_guid"],

                "scheduled_at": (other_start_at_timestamp_ns + i * 10**6) // 1000,
            }

            action_data = get_action_data(ydb_session, current_action_id)
            assert action_data["meta"].Guid == action_row["action_guid"]
            assert action_data == {
                "meta_action_id_hash": CityHash64(current_action_id.encode("utf-8")),
                "meta_action_id": current_action_id,

                "meta": get_scheduled_action_meta(action_id=current_action_id, action_guid=action_row["action_guid"]),
                "meta_action_guid": action_data["meta"].Guid,

                "spec": new_add_scheduled_action_requests[i - 5].Spec,

                "status": get_scheduled_action_status(scheduled_at=get_proto_timestamp(other_start_at_timestamp_ns + i * 10**6, round_by=1000)),
                "status_scheduled_at": (other_start_at_timestamp_ns + i * 10**6) // 1000,
            }

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_action_id", b"Action id must be non-empty"),
        ("with_guid", b"Guid must be empty, but actual value is 'some_guid'"),
        ("no_start_policy", b"Start policy type is not specified"),
        ("start_at_from_past", b"StartPolicy's start at is less than min allowed start at"),
        ("no_send_policy", b"Send policy type is not specified"),
        ("send_once_min_restart_period_greater_than_max", b"SendOncePolicy's min restart period is greater than max restart period (2000000000.000000s > 10.000000s)"),
        ("too_small_send_once_min_restart_period", b"SendOncePolicy's min restart period is less than 1.000000s, actual value is 0.003000s"),
        ("too_small_send_periodically_period", b"SendPeriodicallyPolicy's period is less than 1.000000s, actual value is 0.004000s"),
        ("no_action", b"Action type is not specified"),
        ("two_requests_with_same_id", b"Two or more scheduled actions with id"),
        ("no_requests", b"Add scheduled action requests are not provided"),
        ("too_many_requests", b"Too many scheduled actions to add in one apphost request, actual number of scheduled actions to add is 11, max allowed number is 10"),
    ])
    async def test_invalid_add_scheduled_action_request(
        self,
        matrix_scheduler,
        ydb_session,
        action_id,
        error_type,
        error_message,
    ):
        start_at_timestamp_ns = time.time_ns() + 1337 * 10**9

        add_scheduled_action_requests = []
        for i in range(10):
            add_scheduled_action_requests.append(
                get_add_scheduled_action_request(
                    meta=get_scheduled_action_meta(action_id=f"{action_id}_{i}"),
                    spec=get_scheduled_action_spec(
                        start_policy=get_start_policy(
                            start_at=get_proto_timestamp(start_at_timestamp_ns + i * 10**6),
                        ),
                    ),
                ),
            )

        add_scheduled_action_request_to_patch = add_scheduled_action_requests[5]

        if error_type == "no_action_id":
            action_id = ""
            add_scheduled_action_request_to_patch.Meta.Id = ""
        elif error_type == "with_guid":
            add_scheduled_action_request_to_patch.Meta.Guid = "some_guid"
        elif error_type == "no_start_policy":
            add_scheduled_action_request_to_patch.Spec.ClearField("StartPolicy")
        elif error_type == "start_at_from_past":
            add_scheduled_action_request_to_patch.Spec.StartPolicy.StartAt.CopyFrom(get_proto_timestamp(0))
        elif error_type == "no_send_policy":
            add_scheduled_action_request_to_patch.Spec.ClearField("SendPolicy")
        elif error_type == "send_once_min_restart_period_greater_than_max":
            add_scheduled_action_request_to_patch.Spec.SendPolicy.SendOncePolicy.RetryPolicy.MinRestartPeriod.CopyFrom(get_proto_duration(2 * 10**18))
        elif error_type == "too_small_send_once_min_restart_period":
            add_scheduled_action_request_to_patch.Spec.SendPolicy.SendOncePolicy.RetryPolicy.MinRestartPeriod.CopyFrom(get_proto_duration(3 * 10**6))
        elif error_type == "too_small_send_periodically_period":
            add_scheduled_action_request_to_patch.CopyFrom(
                get_add_scheduled_action_request(
                    meta=get_scheduled_action_meta(action_id=action_id),
                    spec=get_scheduled_action_spec(
                        start_policy=get_start_policy(start_at=get_proto_timestamp(time.time_ns())),
                        send_policy=get_send_periodically_policy(period=get_proto_duration(4 * 10**6)),
                    ),
                )
            )
        elif error_type == "no_action":
            add_scheduled_action_request_to_patch.Spec.ClearField("Action")
        elif error_type == "two_requests_with_same_id":
            add_scheduled_action_request_to_patch.Meta.Id = add_scheduled_action_requests[1].Meta.Id
        elif error_type == "no_requests":
            add_scheduled_action_requests = []
        elif error_type == "too_many_requests":
            add_scheduled_action_requests.append(
                get_add_scheduled_action_request(
                    meta=get_scheduled_action_meta(action_id=f"{action_id}_{len(add_scheduled_action_requests)}"),
                    spec=get_scheduled_action_spec(
                        start_policy=get_start_policy(
                            start_at=get_proto_timestamp(start_at_timestamp_ns + len(add_scheduled_action_requests))
                        ),
                    ),
                ),
            )
        else:
            assert False, f"Unknown error_type {error_type}"

        response = await matrix_scheduler.perform_add_scheduled_action_requests(add_scheduled_action_requests, return_apphost_response_as_is=True)
        assert response.has_exception()
        assert error_message in response.get_exception()

        assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []
        for i in range(len(add_scheduled_action_requests)):
            assert get_action_data(ydb_session, f"{action_id}_{i}") is None

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message", [
        ("no_action_id", b"Action id must be non-empty"),
        ("two_requests_with_same_id", b"Two or more scheduled actions with id"),
        ("no_requests", b"Remove scheduled action requests are not provided"),
        ("too_many_requests", b"Too many scheduled actions to remove in one apphost request, actual number of scheduled actions to remove is 11, max allowed number is 10"),
    ])
    async def test_invalid_remove_scheduled_action_request(
        self,
        matrix_scheduler,
        ydb_session,
        action_id,
        error_type,
        error_message,
    ):
        remove_scheduled_action_requests = []
        for i in range(10):
            remove_scheduled_action_requests.append(
                get_remove_scheduled_action_request(action_id=f"{action_id}_{i}"),
            )

        remove_scheduled_action_request_to_patch = remove_scheduled_action_requests[5]

        if error_type == "no_action_id":
            action_id = ""
            remove_scheduled_action_request_to_patch.ActionId = ""
        elif error_type == "two_requests_with_same_id":
            remove_scheduled_action_request_to_patch.ActionId = remove_scheduled_action_requests[1].ActionId
        elif error_type == "no_requests":
            remove_scheduled_action_requests = []
        elif error_type == "too_many_requests":
            remove_scheduled_action_requests.append(
                get_remove_scheduled_action_request(action_id=f"{action_id}_{len(remove_scheduled_action_requests)}"),
            )
        else:
            assert False, f"Unknown error_type {error_type}"

        response = await matrix_scheduler.perform_remove_scheduled_action_requests(remove_scheduled_action_requests, return_apphost_response_as_is=True)
        assert response.has_exception()
        assert error_message in response.get_exception()

        assert get_action_rows_from_incoming_queue(ydb_session, action_id) == []
        for i in range(len(remove_scheduled_action_requests)):
            assert get_action_data(ydb_session, f"{action_id}_{i}") is None
