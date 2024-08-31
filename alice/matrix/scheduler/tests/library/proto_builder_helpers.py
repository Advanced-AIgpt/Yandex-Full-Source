from alice.protos.api.matrix.action_pb2 import TAction
from alice.protos.api.matrix.scheduled_action_pb2 import (
    TScheduledActionMeta,
    TScheduledActionSpec,
    TScheduledActionStatus,
)
from alice.protos.api.matrix.schedule_action_pb2 import (
    TScheduleAction as TOldScheduleAction,
    TRemoveAction as TOldRemoveAction,

    TMockAction as TOldMockAction,
)

from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_speech_kit_directive,
)

from alice.protos.api.matrix.scheduler_api_pb2 import (
    TAddScheduledActionRequest,
    TRemoveScheduledActionRequest,
)

from alice.protos.api.matrix.technical_push_pb2 import (
    TTechnicalPush,
)

from alice.protos.api.matrix.user_device_pb2 import (
    TUserDeviceIdentifier,
)

from google.protobuf.timestamp_pb2 import Timestamp as ProtoTimestamp
from google.protobuf.duration_pb2 import Duration as ProtoDuration
from google.protobuf.any_pb2 import Any as ProtoAny


def get_proto_duration(duration_ns=1228 * 10**6, round_by=1):
    duration_ns = duration_ns - duration_ns % round_by

    return ProtoDuration(
        seconds=(duration_ns // 10**9),
        nanos=(duration_ns % 10**9),
    )


def get_proto_timestamp(timestamp_ns=1337 * 10**6, round_by=1):
    timestamp_ns = timestamp_ns - timestamp_ns % round_by

    return ProtoTimestamp(
        seconds=(timestamp_ns // 10**9),
        nanos=(timestamp_ns % 10**9),
    )


def get_start_policy(start_at=None):
    if start_at is None:
        start_at = get_proto_timestamp()

    return TScheduledActionSpec.TStartPolicy(
        StartAt=start_at,
    )


def get_retry_policy(
    max_retries=0,
    restart_period_scale=None,
    restart_period_back_off=0,
    min_restart_period=None,
    max_restart_period=None,
):
    if restart_period_scale is None:
        restart_period_scale = get_proto_duration(0)

    if min_restart_period is None:
        min_restart_period = get_proto_duration(10**9 + 1)

    if max_restart_period is None:
        max_restart_period = get_proto_duration(10**10 + 1)

    return TScheduledActionSpec.TSendPolicy.TRetryPolicy(
        MaxRetries=max_retries,
        RestartPeriodScale=restart_period_scale,
        RestartPeriodBackOff=restart_period_back_off,
        MinRestartPeriod=min_restart_period,
        MaxRestartPeriod=max_restart_period,
    )


def get_send_once_policy(retry_policy=None):
    if retry_policy is None:
        retry_policy = get_retry_policy()

    return TScheduledActionSpec.TSendPolicy(
        SendOncePolicy=TScheduledActionSpec.TSendPolicy.TSendOncePolicy(
            RetryPolicy=retry_policy,
        ),
    )


def get_send_periodically_policy(period=None):
    if period is None:
        period = get_proto_duration(10**9 + 7)

    return TScheduledActionSpec.TSendPolicy(
        SendPeriodicallyPolicy=TScheduledActionSpec.TSendPolicy.TSendPeriodicallyPolicy(
            Period=period,
        ),
    )


def get_mock_action(
    name="mock_action_name",
    fail_until_consecutive_failures_counter_less_than=0,
):
    return TAction(
        MockAction=TAction.TMockAction(
            Name=name,
            FailUntilConsecutiveFailuresCounterLessThan=fail_until_consecutive_failures_counter_less_than,
        ),
    )


def get_send_technical_push_action(
    puid,
    device_id,
    technikcal_push_id="technical_push_id",
    speech_kit_directive=None,
):
    packed_to_any_speech_kit_directive = ProtoAny()
    packed_to_any_speech_kit_directive.Pack(speech_kit_directive or get_speech_kit_directive())

    return TAction(
        SendTechnicalPush=TAction.TSendTechnicalPush(
            UserDeviceIdentifier=TUserDeviceIdentifier(
                Puid=puid,
                DeviceId=device_id,
            ),
            TechnicalPush=TTechnicalPush(
                TechnicalPushId=technikcal_push_id,
                SpeechKitDirective=packed_to_any_speech_kit_directive,
            )
        )
    )


def get_scheduled_action_meta(action_id, action_guid=""):
    return TScheduledActionMeta(
        Id=action_id,
        Guid=action_guid,
    )


def get_scheduled_action_spec(
    start_policy=None,
    send_policy=None,
    action=None,
):
    if start_policy is None:
        start_policy = get_start_policy()

    if send_policy is None:
        send_policy = get_send_once_policy()

    if action is None:
        action = get_mock_action()

    return TScheduledActionSpec(
        StartPolicy=start_policy,
        SendPolicy=send_policy,
        Action=action,
    )


def get_scheduled_action_attempt_status(
    status=TScheduledActionStatus.TAttemptStatus.EStatus.UNKNOWN,
    error_message="",
):
    return TScheduledActionStatus.TAttemptStatus(
        Status=status,
        ErrorMessage=error_message,
    )


def get_scheduled_action_status(
    scheduled_at=None,

    successful_attempts_counter=0,
    failed_attempts_counter=0,
    interrupted_attempts_counter=0,

    consecutive_failures_counter=0,

    current_attempt_status=None,
    last_attempt_status=None,
):
    if scheduled_at is None:
        scheduled_at = get_proto_timestamp()

    scheduled_action_status = TScheduledActionStatus(
        ScheduledAt=scheduled_at,

        SuccessfulAttemptsCounter=successful_attempts_counter,
        FailedAttemptsCounter=failed_attempts_counter,
        InterruptedAttemptsCounter=interrupted_attempts_counter,

        ConsecutiveFailuresCounter=consecutive_failures_counter,
    )

    if current_attempt_status is not None:
        scheduled_action_status.CurrentAttemptStatus.CopyFrom(current_attempt_status)

    if last_attempt_status is not None:
        scheduled_action_status.LastAttemptStatus.CopyFrom(last_attempt_status)

    return scheduled_action_status


def get_add_scheduled_action_request(
    meta,
    spec,
    override_mode=TAddScheduledActionRequest.EOverrideMode.NONE,
):
    return TAddScheduledActionRequest(
        Meta=meta,
        Spec=spec,
        OverrideMode=override_mode,
    )


def get_remove_scheduled_action_request(action_id):
    return TRemoveScheduledActionRequest(
        ActionId=action_id,
    )


def get_old_start_policy(start_at_timestamp_ms=1337):
    return TOldScheduleAction.TStartPolicy(
        StartAtTimestampMs=start_at_timestamp_ms,
    )


def get_old_retry_policy(
    max_retries=0,
    min_restart_period_ms=1000,
    max_restart_period_ms=10000,
):
    return TOldScheduleAction.TSendPolicy.TRetryPolicy(
        MaxRetries=max_retries,
        MinRestartPeriodMs=min_restart_period_ms,
        MaxRestartPeriodMs=max_restart_period_ms,
    )


def get_old_send_once_policy(retry_policy=None):
    if retry_policy is None:
        retry_policy = get_old_retry_policy()

    return TOldScheduleAction.TSendPolicy(
        SendOncePolicy=TOldScheduleAction.TSendPolicy.TSendOncePolicy(
            RetryPolicy=retry_policy,
        ),
    )


def get_old_send_periodically_policy(period_ms=1000, retry_policy=None):
    if retry_policy is None:
        retry_policy = get_old_retry_policy()

    return TOldScheduleAction.TSendPolicy(
        SendPeriodicallyPolicy=TOldScheduleAction.TSendPolicy.TSendPeriodicallyPolicy(
            PeriodMs=period_ms,
            RetryPolicy=retry_policy,
        ),
    )


def get_old_mock_action(name=""):
    return TOldScheduleAction.TAction(
        MockAction=TOldMockAction(
            Name=name,
        ),
    )


def get_old_schedule_action(
    action_id,
    puid="",
    device_id="",
    start_policy=None,
    send_policy=None,
    action=None,
    override=False,

):
    if start_policy is None:
        start_policy = get_old_start_policy()

    if send_policy is None:
        send_policy = get_old_send_once_policy()

    if action is None:
        action = get_old_mock_action()

    return TOldScheduleAction(
        Id=action_id,
        Puid=puid,
        DeviceId=device_id,

        StartPolicy=start_policy,
        SendPolicy=send_policy,
        Action=action,

        Override=override,
    )


def get_old_remove_action(action_id):
    return TOldRemoveAction(
        ActionId=action_id,
    )
