#pragma once

#include "common.h"

// Protobufs.
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/api/matrix/scheduled_action.pb.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NAlice::NRemindersApi {

class TSchedulerReminderBuilder {
public:
    TSchedulerReminderBuilder(const TString& puid);

    /** Server directive with push schedule action data.
     *  It doesn't check fields in proto.
     */
    NScenarios::TServerDirective BuildScheduleReminderDirective(const TReminderProto& reminderProto,
                                                                const TString& deviceId,
                                                                const TString& originDeviceId) const;
    NScenarios::TServerDirective BuildCancelReminderDirective(const TString& reminderId, const TString& deviceId) const;

private:
    TString GenerateScheduleId(const TString& reminderId, const TString& deviceId) const;

private:
    const TString Puid_;

    TString ActionName_ = "reminder_shoot";
    NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TRetryPolicy RetryPolicy_;
};

} // namespace NAlice::NRemindersApi
