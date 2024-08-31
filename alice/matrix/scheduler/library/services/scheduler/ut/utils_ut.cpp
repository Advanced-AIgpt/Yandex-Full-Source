#include <alice/matrix/scheduler/library/services/scheduler/utils.h>

#include <alice/megamind/protos/speechkit/directives.pb.h>

#include <alice/protos/api/matrix/action.pb.h>
#include <alice/protos/api/matrix/technical_push.pb.h>
#include <alice/protos/api/matrix/user_device.pb.h>

#include <library/cpp/protobuf/interop/cast.h>
#include <library/cpp/testing/gtest/gtest.h>
#include <library/cpp/testing/gtest_protobuf/matcher.h>

#include <google/protobuf/any.pb.h>
#include <google/protobuf/duration.pb.h>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/message_differencer.h>

#include <util/generic/maybe.h>
#include <util/string/cast.h>

namespace {

template <typename T>
TMaybe<TString> GetProtobufDiff(const T& a, const T& b) {
    TString diff;
    google::protobuf::util::MessageDifferencer messageDifferencer;
    messageDifferencer.ReportDifferencesToString(&diff);
    bool compareResult = messageDifferencer.Compare(a, b);
    return compareResult ? Nothing() : TMaybe<TString>(diff);
}

MATCHER(IsSuccess, "") {
    if (!arg.IsSuccess()) {
        *result_listener << "where the error is: '" << arg.Error() << '\'';
        return false;
    }

    return true;
}

MATCHER_P(ErrorMatches, errorMatcher, "") {
    if (!arg.IsError()) {
        *result_listener << "where the result is success";
        return false;
    }

    if (!::testing::ExplainMatchResult(errorMatcher, arg.Error(), result_listener)) {
        if (result_listener->IsInterested()) {
            *result_listener << "where error '" << arg.Error() << "' ";
            static_cast<::testing::Matcher<TString>>(errorMatcher).DescribeNegationTo(result_listener->stream());
        }
        return false;
    }

    return true;
}

MATCHER_P2(TimeAlmostEq, expected, maxDiff, "") {
    TDuration timeDiff = (expected < arg) ? arg - expected : expected - arg;
    if (timeDiff > maxDiff) {
        *result_listener <<
            "where TimeDiff: " << ToString(timeDiff)
            << ", MaxDiff: " << ToString(maxDiff)
            << ", Diff: " << ToString(timeDiff - maxDiff)
        ;
        return false;
    }
    return true;
}

std::pair<NMatrix::NApi::TScheduledActionMeta, NMatrix::NApi::TScheduledActionSpec> CreateBasicMetaAndSpec() {
    NMatrix::NApi::TScheduledActionMeta meta;
    {
        meta.SetId("action_id");
    }
    NMatrix::NApi::TScheduledActionSpec spec;
    {
        spec.MutableStartPolicy()->MutableStartAt()->CopyFrom(
            NProtoInterop::CastToProto(TInstant::Now() + TDuration::Hours(24))
        );
        spec.MutableSendPolicy()->MutableSendOncePolicy()->MutableRetryPolicy()->MutableMinRestartPeriod()->set_seconds(2);
        spec.MutableSendPolicy()->MutableSendOncePolicy()->MutableRetryPolicy()->MutableMaxRestartPeriod()->set_seconds(10);
        spec.MutableAction()->MutableMockAction();
    }

    return std::make_pair(meta, spec);
}

std::pair<NMatrix::NApi::TScheduledActionMeta, NMatrix::NApi::TScheduledActionSpec> CreateMetaAndSpecWithTechnicalPush() {
    auto [meta, spec] = CreateBasicMetaAndSpec();

    auto& sendTechnicalPush = *spec.MutableAction()->MutableSendTechnicalPush();

    {
        auto& userDeviceIdentifier = *sendTechnicalPush.MutableUserDeviceIdentifier();

        userDeviceIdentifier.SetPuid("puid");
        userDeviceIdentifier.SetDeviceId("device_id");
    }

    {
        auto& technicalPush = *sendTechnicalPush.MutableTechnicalPush();

        technicalPush.SetTechnicalPushId("push_id");
        {
            NAlice::NSpeechKit::TDirective skDirective;
            technicalPush.MutableSpeechKitDirective()->PackFrom(skDirective);
        }
    }

    return std::make_pair(meta, spec);
}

} // namespace

TEST(TMatrixSchedulerServiceUtilsTest, TestCreateScheduledActionFromMetaAndSpecBasic) {
    auto [meta, spec] = CreateBasicMetaAndSpec();

    auto result = NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec);
    ASSERT_THAT(result, IsSuccess());
    NMatrix::NApi::TScheduledAction scheduledAction = result.Success();

    // Meta
    EXPECT_EQ(meta.GetId(), scheduledAction.GetMeta().GetId());
    EXPECT_NE(scheduledAction.GetMeta().GetGuid(), "");

    // Spec
    EXPECT_THAT(spec, NGTest::EqualsProto(scheduledAction.GetSpec()));

    // Status
    EXPECT_THAT(spec.GetStartPolicy().GetStartAt(), NGTest::EqualsProto(scheduledAction.GetStatus().GetScheduledAt()));
}

TEST(TMatrixSchedulerServiceUtilsTest, TestCreateScheduledActionFromMetaAndSpecDefaultSendOncePolicyRestartPeriod) {
    auto [meta, spec] = CreateBasicMetaAndSpec();
    spec.MutableSendPolicy()->MutableSendOncePolicy()->ClearRetryPolicy();

    auto result = NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec);
    ASSERT_THAT(result, IsSuccess());
    NMatrix::NApi::TScheduledAction scheduledAction = result.Success();

    // Check default retry policy
    EXPECT_EQ(scheduledAction.GetSpec().GetSendPolicy().GetSendOncePolicy().GetRetryPolicy().GetMinRestartPeriod().seconds(), 1);
    EXPECT_EQ(scheduledAction.GetSpec().GetSendPolicy().GetSendOncePolicy().GetRetryPolicy().GetMaxRestartPeriod().seconds(), 1);

    // Check that this is the only diff
    EXPECT_EQ("added: SendPolicy.SendOncePolicy.RetryPolicy: { MinRestartPeriod { seconds: 1 } MaxRestartPeriod { seconds: 1 } }\n", GetProtobufDiff(spec, scheduledAction.GetSpec()));
}

TEST(TMatrixSchedulerServiceUtilsTest, TestCreateScheduledActionFromMetaAndSpecStatusScheduledAt) {
    {
        // StartAt >= TInstant::Now()
        auto [meta, spec] = CreateBasicMetaAndSpec();
        auto result = NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec);
        ASSERT_THAT(result, IsSuccess());
        NMatrix::NApi::TScheduledAction scheduledAction = result.Success();

        EXPECT_THAT(
            NProtoInterop::CastFromProto(scheduledAction.GetStatus().GetScheduledAt()),
            TimeAlmostEq(
                NProtoInterop::CastFromProto(spec.GetStartPolicy().GetStartAt()),
                TDuration::Zero()
            )
        );
    }

    {
        // TInstant::Now() - MAX_ALLOWED_START_AT_CLOCK_DRIFT <= StartAt < TInstant::Now()
        auto [meta, spec] = CreateBasicMetaAndSpec();
        spec.MutableStartPolicy()->MutableStartAt()->CopyFrom(NProtoInterop::CastToProto(TInstant::Now() - TDuration::Minutes(4)));

        auto result = NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec);
        ASSERT_THAT(result, IsSuccess());
        NMatrix::NApi::TScheduledAction scheduledAction = result.Success();

        EXPECT_THAT(
            NProtoInterop::CastFromProto(scheduledAction.GetStatus().GetScheduledAt()),
            TimeAlmostEq(
                TInstant::Now(),
                TDuration::Seconds(10)
            )
        );
    }
}

TEST(TMatrixSchedulerServiceUtilsTest, TestCreateScheduledActionFromMetaAndSpecDefaultTechnicalPushId) {
    auto [meta, spec] = CreateMetaAndSpecWithTechnicalPush();

    {
        auto result = NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec);
        ASSERT_THAT(result, IsSuccess());
        NMatrix::NApi::TScheduledAction scheduledAction = result.Success();

        EXPECT_EQ(scheduledAction.GetSpec().GetAction().GetSendTechnicalPush().GetTechnicalPush().GetTechnicalPushId(), "push_id");
    }

    spec.MutableAction()->MutableSendTechnicalPush()->MutableTechnicalPush()->ClearTechnicalPushId();

    {
        auto result = NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec);
        ASSERT_THAT(result, IsSuccess());
        NMatrix::NApi::TScheduledAction scheduledAction = result.Success();

        EXPECT_NE(scheduledAction.GetSpec().GetAction().GetSendTechnicalPush().GetTechnicalPush().GetTechnicalPushId(), "");
    }
}

TEST(TMatrixSchedulerServiceUtilsTest, TestCreateScheduledActionFromMetaAndSpecMetaValidation) {
    {
        auto [meta, spec] = CreateBasicMetaAndSpec();
        meta.ClearId();
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("Action id must be non-empty"))
        );
    }
    {
        auto [meta, spec] = CreateBasicMetaAndSpec();
        meta.SetGuid("guid");
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("Guid must be empty, but actual value is 'guid'"))
        );
    }
}

TEST(TMatrixSchedulerServiceUtilsTest, TestCreateScheduledActionFromMetaAndSpecStartPolicyValidation) {
    {
        // StartAt < TInstant::Now() - MAX_ALLOWED_START_AT_CLOCK_DRIFT
        auto [meta, spec] = CreateBasicMetaAndSpec();
        spec.MutableStartPolicy()->ClearStartAt();
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("Start policy type is not specified"))
        );
    }

    {
        // StartAt < TInstant::Now() - MAX_ALLOWED_START_AT_CLOCK_DRIFT
        auto [meta, spec] = CreateBasicMetaAndSpec();
        spec.MutableStartPolicy()->MutableStartAt()->CopyFrom(NProtoInterop::CastToProto(TInstant::Now() - TDuration::Hours(24)));
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::HasSubstr("StartPolicy's start at is less than min allowed start at"))
        );
    }
}

TEST(TMatrixSchedulerServiceUtilsTest, TestCreateScheduledActionFromMetaAndSpecSendPolicyValidation) {
    {
        auto [meta, spec] = CreateBasicMetaAndSpec();
        spec.ClearSendPolicy();
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("Send policy type is not specified"))
        );
    }

    {
        auto [meta, spec] = CreateBasicMetaAndSpec();
        spec.MutableSendPolicy()->MutableSendOncePolicy()->MutableRetryPolicy()->MutableMinRestartPeriod()->set_seconds(10);
        spec.MutableSendPolicy()->MutableSendOncePolicy()->MutableRetryPolicy()->MutableMaxRestartPeriod()->set_seconds(9);
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("SendOncePolicy's min restart period is greater than max restart period (10.000000s > 9.000000s)"))
        );
    }

    {
        auto [meta, spec] = CreateBasicMetaAndSpec();
        spec.MutableSendPolicy()->MutableSendOncePolicy()->MutableRetryPolicy()->MutableMinRestartPeriod()->set_seconds(0);
        spec.MutableSendPolicy()->MutableSendOncePolicy()->MutableRetryPolicy()->MutableMinRestartPeriod()->set_nanos(1000000);
        spec.MutableSendPolicy()->MutableSendOncePolicy()->MutableRetryPolicy()->MutableMaxRestartPeriod()->set_seconds(0);
        spec.MutableSendPolicy()->MutableSendOncePolicy()->MutableRetryPolicy()->MutableMaxRestartPeriod()->set_nanos(1000000);
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("SendOncePolicy's min restart period is less than 1.000000s, actual value is 0.001000s"))
        );
    }

    {
        auto [meta, spec] = CreateBasicMetaAndSpec();
        spec.MutableSendPolicy()->MutableSendPeriodicallyPolicy()->MutablePeriod()->set_nanos(1000000);
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("SendPeriodicallyPolicy's period is less than 1.000000s, actual value is 0.001000s"))
        );
    }
}

TEST(TMatrixSchedulerServiceUtilsTest, TestCreateScheduledActionFromMetaAndSpecActionValidation) {
    {
        auto [meta, spec] = CreateBasicMetaAndSpec();
        spec.ClearAction();
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("Action type is not specified"))
        );
    }

    {
        auto [meta, spec] = CreateMetaAndSpecWithTechnicalPush();
        spec.MutableAction()->MutableSendTechnicalPush()->MutableUserDeviceIdentifier()->ClearPuid();
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("SendTechnicalPush user device identifier puid must be non-empty"))
        );
    }

    {
        auto [meta, spec] = CreateMetaAndSpecWithTechnicalPush();
        spec.MutableAction()->MutableSendTechnicalPush()->MutableUserDeviceIdentifier()->ClearDeviceId();
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("SendTechnicalPush user device identifier device id must be non-empty"))
        );
    }

    {
        auto [meta, spec] = CreateMetaAndSpecWithTechnicalPush();
        spec.MutableAction()->MutableSendTechnicalPush()->MutableTechnicalPush()->ClearSpeechKitDirective();
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("SendTechnicalPush action SpeechKitDirective not specified"))
        );
    }

    {
        auto [meta, spec] = CreateMetaAndSpecWithTechnicalPush();
        spec.MutableAction()->MutableSendTechnicalPush()->MutableTechnicalPush()->MutableSpeechKitDirective()->PackFrom(meta);
        EXPECT_THAT(
            NMatrix::NScheduler::CreateScheduledActionFromMetaAndSpec(meta, spec),
            ErrorMatches(::testing::Eq("SendTechnicalPush action SpeechKitDirective type must be: NAlice.NSpeechKit.TDirective, actual type is type.googleapis.com/NMatrix.NApi.TScheduledActionMeta"))
        );
    }
}
