#include "ut.h"

#include <alice/library/scenarios/reminders/matrix.h>
#include <alice/library/scenarios/reminders/schedule.h>

#include <alice/library/proto/protobuf.h>

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>

#include <google/protobuf/any.pb.h>

#include <util/string/cast.h>

namespace {

using namespace NAlice::NRemindersApi;
using namespace NAlice::NRemindersApi::NTesting;

TEST_F(TRemindersApiFixture, ScheduleActionSmoke) {
    static const TString puid{"my-puid"};
    static const TString deviceId{"my-device-id"};

    TReminderProto rp = CreateValidReminderProto("my-reminder-id", 100);
    TSchedulerReminderBuilder builder{puid};
    const auto directive = builder.BuildScheduleReminderDirective(rp, deviceId, "1");
    ASSERT_EQ(directive.HasEnlistScheduledActionDirective(), true);
    const auto& sa = directive.GetEnlistScheduledActionDirective().GetAddScheduledActionRequest();

    ASSERT_EQ(sa.GetMeta().GetId(), "reminder_shoot_my-puid_my-device-id_my-reminder-id");

    NAlice::NSpeechKit::TDirective sd;
    sa.GetSpec().GetAction().GetSendTechnicalPush().GetTechnicalPush().GetSpeechKitDirective().UnpackTo(&sd);
    ASSERT_EQ(sd.GetType(), "server_action");
    ASSERT_EQ(sd.GetName(), "@@mm_semantic_frame");
}

TEST_F(TRemindersApiFixture, CancelScheduleActionSmoke) {
    static const TString puid{"my-puid"};
    static const TString deviceId{"my-device-id"};
    static const TString reminderId{"my-reminder-id"};

    TSchedulerReminderBuilder builder{puid};
    /* const auto directive = */builder.BuildCancelReminderDirective(reminderId, deviceId);
    // FIXME (petrk) implement it when BuildXX is implemented!
}

} // ns
