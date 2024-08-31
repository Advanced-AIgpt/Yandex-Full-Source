#include "schedule_action.h"

#include <alice/megamind/protos/common/atm.pb.h>

namespace NAlice::NHollywood {

TScheduleActionDirectiveBuilder::TScheduleActionDirectiveBuilder()
    : ScheduleAction{*Directive.MutableAddScheduleActionDirective()->MutableScheduleAction()}
    , Delivery{*ScheduleAction.MutableAction()->MutableOldNotificatorRequest()->MutableDelivery()}
{
}

TScheduleActionDirectiveBuilder& TScheduleActionDirectiveBuilder::SetIds(const TStringBuf actionId,
                                                                         const TStringBuf deviceId,
                                                                         const TStringBuf puid)
{
    // Have to make id unique by appending puid and device id
    ScheduleAction.SetId(TString::Join(actionId, "_", puid, "_", deviceId));
    // Have to set puid and device id twice before TOldNotificatorRequest gets deprecated
    ScheduleAction.SetPuid(TString{puid});
    ScheduleAction.SetDeviceId(TString{deviceId});
    Delivery.SetPuid(TString{puid});
    Delivery.SetDeviceId(TString{deviceId});
    return *this;
}

TScheduleActionDirectiveBuilder& TScheduleActionDirectiveBuilder::SetTypedSemanticFrame(NAlice::TTypedSemanticFrame&& frame) {
    *Delivery.MutableSemanticFrameRequestData()->MutableTypedSemanticFrame() = std::move(frame);
    return *this;
}

TScheduleActionDirectiveBuilder& TScheduleActionDirectiveBuilder::SetAnalytics(const TStringBuf purpose) {
    auto& analytics = *Delivery.MutableSemanticFrameRequestData()->MutableAnalytics();
    analytics.SetOrigin(NAlice::TAnalyticsTrackingModule::Scenario);
    analytics.SetPurpose(TString{purpose});
    return *this;
}

TScheduleActionDirectiveBuilder& TScheduleActionDirectiveBuilder::SetStartAtTimestampMs(const size_t startAtTimestampMs) {
    ScheduleAction.MutableStartPolicy()->SetStartAtTimestampMs(startAtTimestampMs);
    return *this;
}

TScheduleActionDirectiveBuilder& TScheduleActionDirectiveBuilder::SetSendOncePolicy(const size_t minRestartPeriodMs,
                                                                                    const size_t maxRestartPeriodMs)
{
    auto& sendPolicy = *ScheduleAction.MutableSendPolicy()->MutableSendOncePolicy();
    sendPolicy.MutableRetryPolicy()->SetMinRestartPeriodMs(minRestartPeriodMs);
    sendPolicy.MutableRetryPolicy()->SetMaxRestartPeriodMs(maxRestartPeriodMs);
    return *this;
}

NScenarios::TServerDirective TScheduleActionDirectiveBuilder::Build() && {
    return std::move(Directive);
}

} // namespace NAlice::NHollywood
