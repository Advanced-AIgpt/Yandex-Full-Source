import pytest
import time

from alice.matrix.worker.tests.library.test_base import MatrixWorkerTestBase

from alice.protos.api.matrix.scheduled_action_pb2 import (
    TScheduledActionStatus,
)

from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_speech_kit_directive,
)

from alice.matrix.scheduler.tests.library.proto_builder_helpers import (
    get_scheduled_action_meta,
    get_scheduled_action_spec,
    get_scheduled_action_status,
    get_scheduled_action_attempt_status,

    get_add_scheduled_action_request,

    get_send_periodically_policy,
    get_start_policy,

    get_send_technical_push_action,

    get_proto_duration,
    get_proto_timestamp,
)

from alice.matrix.scheduler.tests.library.ydb import (
    get_action_data,
)

from alice.matrix.worker.tests.library.ydb import (
    get_action_rows_from_processing_queue,

    unlock_all_shard_locks_and_set_shard_count,
)

from alice.protos.api.matrix.delivery_pb2 import TDeliveryResponse

from cityhash import hash64 as CityHash64


class TestWorkerDoSendTechnicalPush(MatrixWorkerTestBase):
    worker_manual_sync_mode = True

    def _assert_matrix_notificator_mock_state(
        self,
        matrix_notificator_mock,
        scheduled_action_spec,
        speech_kit_directive,
        expected_request_count,
    ):
        notificator_request, request_count = matrix_notificator_mock.get_requests_info()
        assert request_count == expected_request_count
        assert notificator_request.Puid == scheduled_action_spec.Action.SendTechnicalPush.UserDeviceIdentifier.Puid
        assert notificator_request.DeviceId == scheduled_action_spec.Action.SendTechnicalPush.UserDeviceIdentifier.DeviceId
        assert scheduled_action_spec.Action.SendTechnicalPush.TechnicalPush.TechnicalPushId in notificator_request.PushId
        assert notificator_request.SpeechKitDirective == speech_kit_directive.SerializeToString()

    def _assert_ydb_state(
        self,
        ydb_session,
        scheduled_action_meta,
        scheduled_action_spec,
        expected_successful_attempts_counter,
        expected_failed_attempts_counter,
        expected_status,
        expected_error_message_substring=None,
    ):
        action_id = scheduled_action_meta.Id

        processing_queue = get_action_rows_from_processing_queue(ydb_session, action_id)
        assert len(processing_queue) == 1

        processing_action_row = processing_queue[0]
        assert processing_action_row['action_id'] == action_id

        action_data = get_action_data(ydb_session, action_id)
        assert action_data == {
            "meta_action_id_hash": CityHash64(action_id.encode("utf-8")),
            "meta_action_id": action_id,

            "meta": get_scheduled_action_meta(action_id=action_id, action_guid=action_data["meta"].Guid),
            "meta_action_guid": action_data["meta"].Guid,

            "spec": scheduled_action_spec,

            "status": get_scheduled_action_status(
                scheduled_at=get_proto_timestamp(processing_action_row["scheduled_at"] * 10**3),
                successful_attempts_counter=expected_successful_attempts_counter,
                failed_attempts_counter=expected_failed_attempts_counter,
                consecutive_failures_counter=expected_failed_attempts_counter,
                last_attempt_status=get_scheduled_action_attempt_status(
                    status=expected_status,
                    error_message=action_data["status"].LastAttemptStatus.ErrorMessage,
                ),
            ),
            "status_scheduled_at": processing_action_row["scheduled_at"],
        }

        if expected_error_message_substring is not None:
            assert expected_error_message_substring in action_data["status"].LastAttemptStatus.ErrorMessage
        else:
            assert action_data["status"].LastAttemptStatus.ErrorMessage == ""

    @pytest.mark.asyncio
    async def test_do_send_technical_push_success(
        self,
        matrix_scheduler,
        matrix_notificator_mock,
        matrix_worker,
        ydb_session,
        action_id,
        puid,
        device_id,
    ):
        matrix_notificator_mock.set_response_config(
            http_status_code=200,
            add_push_to_database_status=TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK,
            subway_request_status=TDeliveryResponse.TSubwayRequestStatus.EStatus.OK,
        )

        unlock_all_shard_locks_and_set_shard_count(ydb_session, 1)

        scheduled_action_meta = get_scheduled_action_meta(action_id=action_id)
        # Start ASAP
        start_at_timestamp_ns = time.time_ns()
        speech_kit_directive = get_speech_kit_directive()

        scheduled_action_spec = get_scheduled_action_spec(
            start_policy=get_start_policy(
                start_at=get_proto_timestamp(start_at_timestamp_ns),
            ),
            send_policy=get_send_periodically_policy(
                # Leave the action in the database to check its status
                period=get_proto_duration(10**3 * 10**9),
            ),
            action=get_send_technical_push_action(
                puid=puid,
                device_id=device_id,
                speech_kit_directive=speech_kit_directive,
            ),
        )
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
        )

        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)

        await matrix_worker.perform_manual_sync_request()

        scheduled_action_spec.Action.SendTechnicalPush.UserDeviceIdentifier.Puid

        self._assert_matrix_notificator_mock_state(
            matrix_notificator_mock=matrix_notificator_mock,
            scheduled_action_spec=scheduled_action_spec,
            speech_kit_directive=speech_kit_directive,
            expected_request_count=1,
        )
        self._assert_ydb_state(
            ydb_session=ydb_session,
            scheduled_action_meta=scheduled_action_meta,
            scheduled_action_spec=scheduled_action_spec,
            expected_successful_attempts_counter=1,
            expected_failed_attempts_counter=0,
            expected_status=TScheduledActionStatus.TAttemptStatus.EStatus.SUCCESS,
        )

    @pytest.mark.asyncio
    @pytest.mark.parametrize("error_type, error_message_substring", [
        ("device_not_connected", "Device not connected"),
        ("bad_http_response_code", "Bad http response code"),
    ])
    async def test_do_send_technical_push_error(
        self,
        matrix_scheduler,
        matrix_notificator_mock,
        matrix_worker,
        ydb_session,
        action_id,
        puid,
        device_id,
        error_type,
        error_message_substring,
    ):
        if error_type == "device_not_connected":
            matrix_notificator_mock.set_response_config(
                http_status_code=200,
                add_push_to_database_status=TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.OK,
                subway_request_status=TDeliveryResponse.TSubwayRequestStatus.EStatus.LOCATION_NOT_FOUND,
            )
        elif error_type == "bad_http_response_code":
            matrix_notificator_mock.set_response_config(
                http_status_code=400,
                add_push_to_database_status=TDeliveryResponse.TAddPushToDatabaseStatus.EStatus.ERROR,
                subway_request_status=TDeliveryResponse.TSubwayRequestStatus.EStatus.UNKNOWN,
            )
        else:
            assert False, f"Unknown error_type {error_type}"

        unlock_all_shard_locks_and_set_shard_count(ydb_session, 1)

        scheduled_action_meta = get_scheduled_action_meta(action_id=action_id)
        # Start ASAP
        start_at_timestamp_ns = time.time_ns()
        speech_kit_directive = get_speech_kit_directive()

        scheduled_action_spec = get_scheduled_action_spec(
            start_policy=get_start_policy(
                start_at=get_proto_timestamp(start_at_timestamp_ns),
            ),
            send_policy=get_send_periodically_policy(
                # Leave the action in the database to check its status
                period=get_proto_duration(10**3 * 10**9),
            ),
            action=get_send_technical_push_action(
                puid=puid,
                device_id=device_id,
                speech_kit_directive=speech_kit_directive,
            ),
        )
        add_scheduled_action_request = get_add_scheduled_action_request(
            meta=scheduled_action_meta,
            spec=scheduled_action_spec,
        )

        await matrix_scheduler.perform_add_scheduled_action_request(add_scheduled_action_request)

        await matrix_worker.perform_manual_sync_request()

        self._assert_matrix_notificator_mock_state(
            matrix_notificator_mock=matrix_notificator_mock,
            scheduled_action_spec=scheduled_action_spec,
            speech_kit_directive=speech_kit_directive,
            expected_request_count=1,
        )
        self._assert_ydb_state(
            ydb_session=ydb_session,
            scheduled_action_meta=scheduled_action_meta,
            scheduled_action_spec=scheduled_action_spec,
            expected_successful_attempts_counter=0,
            expected_failed_attempts_counter=1,
            expected_status=TScheduledActionStatus.TAttemptStatus.EStatus.ERROR,
            expected_error_message_substring=error_message_substring,
        )
