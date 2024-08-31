#pragma once

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <alice/protos/api/matrix/delivery.pb.h>
#include <alice/protos/api/matrix/schedule_action.pb.h>

namespace NAlice::NHollywood {

class TScheduleActionDirectiveBuilder {
public:
    TScheduleActionDirectiveBuilder();

    // mandatory
    TScheduleActionDirectiveBuilder& SetIds(TStringBuf actionId, TStringBuf deviceId, TStringBuf puid);

    // mandatory
    TScheduleActionDirectiveBuilder& SetTypedSemanticFrame(NAlice::TTypedSemanticFrame&& frame);

    // mandatory
    TScheduleActionDirectiveBuilder& SetAnalytics(TStringBuf purpose);

    // mandatory
    TScheduleActionDirectiveBuilder& SetStartAtTimestampMs(size_t startAtTimestampMs);

    // mandatory
    TScheduleActionDirectiveBuilder& SetSendOncePolicy(size_t minRestartPeriodMs = 1000, size_t maxRestartPeriodMs = 5000);

    NScenarios::TServerDirective Build() &&;

private:
    NScenarios::TServerDirective Directive;
    NMatrix::NApi::TScheduleAction& ScheduleAction;
    NMatrix::NApi::TDelivery& Delivery;
};

} // namespace NAlice::NHollywood
