#include "applicability_wrapper.h"

#include <alice/hollywood/library/testing/mock_scenario_run_request_wrapper.h>

#include <alice/library/proto/proto.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NHollywood;
using namespace NAlice::NHollywood::NImpl;
using namespace testing;

namespace {

class TFixture : public NUnitTest::TBaseFixture {
public:
    TFixture() {
    }

    TMockScenarioRunRequestWrapper& RunRequestWrapper() {
        return RunRequestWrapper_;
    }

private:
    TMockScenarioRunRequestWrapper RunRequestWrapper_;
};

Y_UNIT_TEST_SUITE_F(Applicability, TFixture) {
    Y_UNIT_TEST(EncloseRegexp) {
        UNIT_ASSERT_EQUAL("(?:)", EncloseRegexp(""));
        UNIT_ASSERT_EQUAL("(?:abc)", EncloseRegexp("abc"));
    }
    Y_UNIT_TEST(AddBeginEndToRegexp) {
        UNIT_ASSERT_EQUAL("^$", AddBeginEndToRegexp(""));
        UNIT_ASSERT_EQUAL("^(?:abc)$", AddBeginEndToRegexp("(?:abc)"));
    }
    Y_UNIT_TEST(TestApplicable) {
        auto& request = RunRequestWrapper();
        NAlice::NHollywood::TRequestApplicabilityWrapper wrapper{THardcodedResponseFastDataProto::TApplicabilityInfo{}};
        UNIT_ASSERT(wrapper.IsApplicable(request, NAlice::TRTLogger::NullLogger()));
    }
    Y_UNIT_TEST(TestDisabledChildren) {
        auto& request = RunRequestWrapper();
        const auto applicability = ParseProtoText<THardcodedResponseFastDataProto::TApplicabilityInfo>(R"(
            DisabledForChildren: true
        )");
        const auto requestProto = ParseProtoText<TScenarioRunRequestWrapper::TProto>(R"(
            BaseRequest {
                UserClassification {
                    Age: Child
                }
            }
        )");

        NAlice::NHollywood::TRequestApplicabilityWrapper wrapper{applicability};
        EXPECT_CALL(request, Proto()).WillRepeatedly(ReturnRef(requestProto));
        UNIT_ASSERT(!wrapper.IsApplicable(request, NAlice::TRTLogger::NullLogger()));
    }
    Y_UNIT_TEST(TestSupportedFeatureApplicable) {
        auto& request = RunRequestWrapper();
        const auto applicability = ParseProtoText<THardcodedResponseFastDataProto::TApplicabilityInfo>(R"(
            SupportedFeature: "CanOpenLink"
        )");
        const auto baseRequestProto = ParseProtoText<NScenarios::TScenarioBaseRequest>(R"(
            Interfaces {
                CanOpenLink: true
            }
        )");
        NAlice::NHollywood::TRequestApplicabilityWrapper wrapper{applicability};
        EXPECT_CALL(request, BaseRequestProto()).WillRepeatedly(ReturnRef(baseRequestProto));
        UNIT_ASSERT(!wrapper.IsApplicable(request, NAlice::TRTLogger::NullLogger()));
    }
    Y_UNIT_TEST(TestSupportedFeatureFalse) {
        auto& request = RunRequestWrapper();
        const auto applicability = ParseProtoText<THardcodedResponseFastDataProto::TApplicabilityInfo>(R"(
            SupportedFeature: "CanOpenLink"
        )");
        const auto baseRequestProto = ParseProtoText<NScenarios::TScenarioBaseRequest>(R"(
            Interfaces {
                CanOpenLink: false
            }
        )");
        EXPECT_CALL(request, BaseRequestProto()).WillRepeatedly(ReturnRef(baseRequestProto));
        NAlice::NHollywood::TRequestApplicabilityWrapper wrapper{applicability};
        UNIT_ASSERT(!wrapper.IsApplicable(request, NAlice::TRTLogger::NullLogger()));
    }
    Y_UNIT_TEST(TestSupportedFeatureIsNotPresented) {
        auto& request = RunRequestWrapper();
        const auto applicability = ParseProtoText<THardcodedResponseFastDataProto::TApplicabilityInfo>(R"(
            SupportedFeature: "CanOpenKek"
        )");
        NAlice::NHollywood::TRequestApplicabilityWrapper wrapper{applicability};
        UNIT_ASSERT(!wrapper.IsApplicable(request, NAlice::TRTLogger::NullLogger()));
    }
    Y_UNIT_TEST(TestAppIdRegexpApplicable) {
        auto& request = RunRequestWrapper();
        const auto requestProto = ParseProtoText<TScenarioRunRequestWrapper::TProto>(R"(
            BaseRequest {
                ClientInfo {
                    AppId: "kolonka"
                }
            }
        )");
        EXPECT_CALL(request, Proto()).WillRepeatedly(ReturnRef(requestProto));

        {
            const auto applicability = ParseProtoText<THardcodedResponseFastDataProto::TApplicabilityInfo>(R"(
                AppIdRegexp: ".*lonk.*"
            )");
            TRequestApplicabilityWrapper wrapper{applicability};

            UNIT_ASSERT(wrapper.IsApplicable(request, TRTLogger::NullLogger()));
        }
        {
            const auto applicability = ParseProtoText<THardcodedResponseFastDataProto::TApplicabilityInfo>(R"(
                AppIdRegexp: ".*kek.*"
            )");
            TRequestApplicabilityWrapper wrapper{applicability};

            UNIT_ASSERT(!wrapper.IsApplicable(request, TRTLogger::NullLogger()));
        }
    }
    Y_UNIT_TEST(TestExperimentApplicable) {
        auto& request = RunRequestWrapper();

        const auto applicability = ParseProtoText<THardcodedResponseFastDataProto::TApplicabilityInfo>(R"(
            Experiment: "mm_enable_disable"
        )");

        TRequestApplicabilityWrapper wrapper{applicability};

        UNIT_ASSERT(!wrapper.IsApplicable(request, TRTLogger::NullLogger()));

        EXPECT_CALL(request, HasExpFlag("mm_enable_disable")).WillRepeatedly(Return(true));

        UNIT_ASSERT(wrapper.IsApplicable(request, TRTLogger::NullLogger()));
    }
    Y_UNIT_TEST(TestPromoType) {
        auto& request = RunRequestWrapper();

        const auto requestProto = ParseProtoText<TScenarioRunRequestWrapper::TProto>(R"(
            BaseRequest {
                Options {
                    PromoType: PT_GREEN_PERSONALITY
                }
            }
        )");
        EXPECT_CALL(request, Proto()).WillRepeatedly(ReturnRef(requestProto));
        {
            const auto applicability = ParseProtoText<THardcodedResponseFastDataProto::TApplicabilityInfo>(R"(
                EnableForPromoTypes: [PT_RED_PERSONALITY, PT_GREEN_PERSONALITY]
            )");
            TRequestApplicabilityWrapper wrapper{applicability};

            UNIT_ASSERT(wrapper.IsApplicable(request, TRTLogger::NullLogger()));
        }
        {
            const auto applicability = ParseProtoText<THardcodedResponseFastDataProto::TApplicabilityInfo>(R"(
                EnableForPromoTypes: [PT_RED_PERSONALITY]
            )");
            TRequestApplicabilityWrapper wrapper{applicability};

            UNIT_ASSERT(!wrapper.IsApplicable(request, TRTLogger::NullLogger()));
        }
    }
}

} // namespace
