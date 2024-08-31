#include "schedule.h"

#include "api.h"
#include "matrix.h"

#include <alice/megamind/protos/common/frame.pb.h>

#include <alice/library/frame/directive_builder.h>

#include <google/protobuf/any.pb.h>
#include <google/protobuf/duration.pb.h>
#include <google/protobuf/timestamp.pb.h>

#include <library/cpp/protobuf/interop/cast.h>

#include <util/generic/guid.h>
#include <util/string/cast.h>

namespace NAlice::NRemindersApi {
namespace {

// TODO (petrk) Theese defaults must be tuned (now the values have been taken from the air!).
NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TRetryPolicy RetryPolicyDefaults() {
    NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TRetryPolicy retryPolicy;

    retryPolicy.SetMaxRetries(10);
    *retryPolicy.MutableMinRestartPeriod() = NProtoInterop::CastToProto(TDuration::Minutes(1));
    *retryPolicy.MutableMaxRestartPeriod() = NProtoInterop::CastToProto(TDuration::Minutes(15));
    retryPolicy.SetRestartPeriodBackOff(2);
    return retryPolicy;
}

} // namespace

TSchedulerReminderBuilder::TSchedulerReminderBuilder(const TString& puid)
    : Puid_{puid}
    , RetryPolicy_{RetryPolicyDefaults()}
{
}

NScenarios::TServerDirective TSchedulerReminderBuilder::BuildScheduleReminderDirective(
    const TReminderProto& reminderProto, const TString& deviceId, const TString& originDeviceId) const
{
    const TInstant shootAt = TInstant::Seconds(reminderProto.GetShootAt());
    const TString actionId = GenerateScheduleId(reminderProto.GetId(), deviceId);

    NScenarios::TServerDirective sd;
    auto& scheduledAction = *sd.MutableEnlistScheduledActionDirective()->MutableAddScheduledActionRequest();
    scheduledAction.SetOverrideMode(NMatrix::NScheduler::NApi::TAddScheduledActionRequest_EOverrideMode_ALL);

    // Meta.
    auto& meta = *scheduledAction.MutableMeta();
    meta.SetId(actionId);

    // Spec.
    auto& spec = *scheduledAction.MutableSpec();

    // Spec->Start Policy.
    auto& startPolicy = *spec.MutableStartPolicy();
    *startPolicy.MutableStartAt() = NProtoInterop::CastToProto(shootAt);

    // Spec->SendPolicy
    auto& sendPolicy = *spec.MutableSendPolicy();
    *sendPolicy.MutableSendOncePolicy()->MutableRetryPolicy() = RetryPolicy_;
    *sendPolicy.MutableDeadline() = NProtoInterop::CastToProto(shootAt + TDuration::Minutes(15));

    // Spec->Action.
    auto& action = *spec.MutableAction();

    auto& stp = *action.MutableSendTechnicalPush();
    auto& udi = *stp.MutableUserDeviceIdentifier();
    udi.SetPuid(Puid_);
    udi.SetDeviceId(deviceId);

    TTypedSemanticFrameDirectiveBuilder tsfBuilder;
    auto& tsf = *tsfBuilder.MutableTypedSemanticFrame().MutableRemindersOnShootSemanticFrame();
    tsf.MutableId()->SetStringValue(reminderProto.GetId());
    tsf.MutableText()->SetStringValue(reminderProto.GetText());
    tsf.MutableEpoch()->SetStringValue(ToString(reminderProto.GetShootAt()));
    tsf.MutableTimeZone()->SetStringValue(reminderProto.GetTimeZone());
    tsf.MutableOriginDeviceId()->SetStringValue(originDeviceId);

    tsfBuilder.SetScenarioAnalyticsInfo("reminders", "on_shoot", TString::Join(Puid_, '@', deviceId));
    stp.MutableTechnicalPush()->MutableSpeechKitDirective()->PackFrom(tsfBuilder.BuildSpeechKitDirective());

    return sd;
}

NScenarios::TServerDirective TSchedulerReminderBuilder::BuildCancelReminderDirective(
    const TString& reminderId, const TString& deviceId) const
{
    NScenarios::TServerDirective sd;
    sd.MutableCancelScheduledActionDirective()
        ->MutableRemoveScheduledActionRequest()
        ->SetActionId(GenerateScheduleId(reminderId, deviceId));
    return sd;
}

TString TSchedulerReminderBuilder::GenerateScheduleId(const TString& reminderId, const TString& deviceId) const {
    return TString::Join(ActionName_, '_', Puid_, '_', deviceId, '_', reminderId);
}

} // namespace NAlice::NRemindersApi
