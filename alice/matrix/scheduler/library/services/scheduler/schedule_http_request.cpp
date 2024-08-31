#include "schedule_http_request.h"

#include "utils.h"

#include <alice/megamind/api/utils/directives.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>

#include <alice/protos/api/matrix/action.pb.h>
#include <alice/protos/api/matrix/delivery.pb.h>
#include <alice/protos/api/matrix/technical_push.pb.h>
#include <alice/protos/api/matrix/user_device.pb.h>

#include <library/cpp/protobuf/interop/cast.h>

#include <google/protobuf/duration.pb.h>
#include <google/protobuf/timestamp.pb.h>

#include <util/random/random.h>

namespace NMatrix::NScheduler {

NMatrix::NApi::TScheduledActionMeta CreateScheduledActionMetaFromOldApiRequest(const NMatrix::NApi::TScheduleAction& scheduleAction) {
    NMatrix::NApi::TScheduledActionMeta meta;

    meta.SetId(scheduleAction.GetId());

    return meta;
}

NMatrix::NApi::TScheduledActionSpec CreateScheduledActionSpecFromOldApiRequest(const NMatrix::NApi::TScheduleAction& scheduleAction) {
    NMatrix::NApi::TScheduledActionSpec spec;

    // StartPolicy
    {
        auto& startPolicy = *spec.MutableStartPolicy();

        startPolicy.MutableStartAt()->CopyFrom(
            NProtoInterop::CastToProto(
                // Max is here for backward compatibility with old api: ZION-229
                Max(TInstant::Now(), TInstant::MilliSeconds(scheduleAction.GetStartPolicy().GetStartAtTimestampMs()))
            )
        );
    }

    // SendPolicy
    {
        auto& sendPolicy = *spec.MutableSendPolicy();

        switch (scheduleAction.GetSendPolicy().GetPolicyCase()) {
            case NMatrix::NApi::TScheduleAction::TSendPolicy::kSendOncePolicy: {
                auto& sendOncePolicy = *sendPolicy.MutableSendOncePolicy();
                if (scheduleAction.GetSendPolicy().GetSendOncePolicy().HasRetryPolicy()) {
                    auto& retryPolicy = *sendOncePolicy.MutableRetryPolicy();
                    const auto& scheduleActionRetryPolocy = scheduleAction.GetSendPolicy().GetSendOncePolicy().GetRetryPolicy();

                    retryPolicy.SetMaxRetries(scheduleActionRetryPolocy.GetMaxRetries());

                    retryPolicy.MutableRestartPeriodScale()->CopyFrom(
                        NProtoInterop::CastToProto(TDuration::MilliSeconds(scheduleActionRetryPolocy.GetRestartPeriodScaleMs()))
                    );
                    retryPolicy.SetRestartPeriodBackOff(scheduleActionRetryPolocy.GetRestartPeriodBackOff());

                    retryPolicy.MutableMinRestartPeriod()->CopyFrom(
                        NProtoInterop::CastToProto(TDuration::MilliSeconds(scheduleActionRetryPolocy.GetMinRestartPeriodMs()))
                    );
                    retryPolicy.MutableMaxRestartPeriod()->CopyFrom(
                        NProtoInterop::CastToProto(TDuration::MilliSeconds(scheduleActionRetryPolocy.GetMaxRestartPeriodMs()))
                    );
                }
                break;
            }
            case NMatrix::NApi::TScheduleAction::TSendPolicy::kSendPeriodicallyPolicy: {
                auto& sendPeriodicallyPolicy = *sendPolicy.MutableSendPeriodicallyPolicy();
                sendPeriodicallyPolicy.MutablePeriod()->CopyFrom(
                    NProtoInterop::CastToProto(TDuration::MilliSeconds(scheduleAction.GetSendPolicy().GetSendPeriodicallyPolicy().GetPeriodMs()))
                );
                break;
            }
            default: {
                break;
            }
        }

        if (scheduleAction.GetSendPolicy().GetDeadlineTimestampMs()) {
            sendPolicy.MutableDeadline()->CopyFrom(
                NProtoInterop::CastToProto(TInstant::MilliSeconds(scheduleAction.GetSendPolicy().GetDeadlineTimestampMs()))
            );
        }
    }

    // Action
    {
        auto& action = *spec.MutableAction();

        switch (scheduleAction.GetAction().GetActionCase()) {
            case NMatrix::NApi::TScheduleAction::TAction::kMockAction: {
                action.MutableMockAction()->SetName(scheduleAction.GetAction().GetMockAction().GetName());
                action.MutableMockAction()->SetFailUntilConsecutiveFailuresCounterLessThan(
                    scheduleAction.GetAction().GetMockAction().GetFailUntilConsecutiveFailuresCounterLessThan()
                );
                break;
            }
            case NMatrix::NApi::TScheduleAction::TAction::kOldNotificatorRequest: {
                const auto& oldNotificatorRequest = scheduleAction.GetAction().GetOldNotificatorRequest();
                auto& sendTechnicalPush = *spec.MutableAction()->MutableSendTechnicalPush();

                sendTechnicalPush.MutableUserDeviceIdentifier()->SetPuid(oldNotificatorRequest.GetDelivery().GetPuid());
                sendTechnicalPush.MutableUserDeviceIdentifier()->SetDeviceId(oldNotificatorRequest.GetDelivery().GetDeviceId());

                sendTechnicalPush.MutableTechnicalPush()->SetTechnicalPushId(oldNotificatorRequest.GetDelivery().GetPushId());

                switch (oldNotificatorRequest.GetDelivery().GetTRequestDirectiveCase()) {
                    case NMatrix::NApi::TDelivery::kSemanticFrameRequestData: {
                        sendTechnicalPush.MutableTechnicalPush()->MutableSpeechKitDirective()->PackFrom(
                            NAlice::NMegamindApi::MakeDirectiveWithTypedSemanticFrame(oldNotificatorRequest.GetDelivery().GetSemanticFrameRequestData())
                        );

                        break;
                    }
                    case NMatrix::NApi::TDelivery::kSpeechKitDirective: {
                        NAlice::NSpeechKit::TDirective skDirective;
                        if (skDirective.ParseFromString(oldNotificatorRequest.GetDelivery().GetSpeechKitDirective())) {
                            sendTechnicalPush.MutableTechnicalPush()->MutableSpeechKitDirective()->PackFrom(skDirective);
                        } else {
                            // Leave work to validator
                        }

                        break;
                    }
                    default: {
                        // Leave work to validator
                        break;
                    }
                }

                break;
            }
            default: {
                break;
            }
        }
    }

    return spec;
}

TScheduleHttpRequest::TScheduleHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TSchedulerStorage& schedulerStorage,
    const ui64 shardCount
)
    : TProtoHttpRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Post == method;
        }
    )
    , SchedulerStorage_(schedulerStorage)
    , ShardCount_(shardCount)
    , ScheduledAction_(
        CreateScheduledActionFromMetaAndSpec(
            CreateScheduledActionMetaFromOldApiRequest(*Request_),
            CreateScheduledActionSpecFromOldApiRequest(*Request_)
        )
    )
{
    if (IsFinished()) {
        return;
    }

    if (ScheduledAction_.IsError()) {
        LogContext_.LogEventInfoCombo<NEvClass::TMatrixSchedulerAddScheduledActionValidationError>(
            Request_->GetId(),
            ScheduledAction_.Error()
        );

        SetError(
            TString::Join("Failed to build scheduled action: ", ScheduledAction_.Error()),
            400
        );
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TScheduleHttpRequest::ServeAsync() {
    const auto& scheduledAction = ScheduledAction_.Success();
    const ui64 shardId = RandomNumber<ui64>(ShardCount_);
    NApi::TAddScheduledActionRequest::EOverrideMode overrideMode = Request_->GetOverride()
        ? NApi::TAddScheduledActionRequest::META_AND_SPEC_ONLY
        : NApi::TAddScheduledActionRequest::NONE
    ;

    LogContext_.LogEventInfoCombo<NEvClass::TMatrixSchedulerAddScheduledAction>(
        scheduledAction.GetMeta().GetId(),
        scheduledAction.GetMeta().GetGuid(),
        scheduledAction,
        shardId,
        overrideMode
    );

    return SchedulerStorage_.AddScheduledActions(
        {
            TSchedulerStorage::TScheduledActionToAdd({
                .ShardId = shardId,
                .OverrideMode = overrideMode,
                .ScheduledAction = scheduledAction,
            })
        },
        LogContext_,
        Metrics_
    ).Apply(
        [this](const NThreading::TFuture<TExpected<void, TString>>& fut) {
            if (const auto& res = fut.GetValueSync(); !res) {
                SetError(res.Error(), 500);
            }
        }
   );
}

TScheduleHttpRequest::TReply TScheduleHttpRequest::GetReply() const {
    return TReply(200);
}

} // namespace NMatrix::NScheduler
