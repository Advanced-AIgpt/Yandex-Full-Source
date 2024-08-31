#include "builder.h"
#include "response.h"

#include <alice/megamind/library/testing/fake_guid_generator.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/utils.h>

#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/property/property.pb.h>

#include <google/protobuf/wrappers.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;

const TString VINS_RESPONSE_TEXT = R"({"type":"vins"})";
constexpr TStringBuf SKR_WITH_EVENT = TStringBuf(R"({"request":{"event":{"type":"text_input"}}})");

Y_UNIT_TEST_SUITE(Response) {
    Y_UNIT_TEST(ScenarioInitialMeansEmpty) {
        TScenarioResponse scenario{"s1", {}, true};
        UNIT_ASSERT_VALUES_EQUAL(scenario.GetScenarioName(), "s1");

        UNIT_ASSERT_VALUES_EQUAL(scenario.GetScenarioAcceptsAnyUtterance(), true);
        UNIT_ASSERT_VALUES_EQUAL(scenario.BuilderIfExists(), nullptr);
    }

    Y_UNIT_TEST(ResponseBuilder) {
        auto skr = TSpeechKitRequestBuilder(SKR_WITH_EVENT).Build();
        TScenarioResponse scenario{"s1", {}, true};

        TResponseBuilder& scenarioBuilder = scenario.ForceBuilder(skr, CreateRequestFromSkr(skr), NMegamind::TFakeGuidGenerator{"GUID"});
        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "s1", storage, NMegamind::TFakeGuidGenerator{"GUID"}};

        UNIT_ASSERT_VALUES_EQUAL(builder.ToProto().ShortUtf8DebugString(), scenarioBuilder.ToProto().ShortDebugString());

        UNIT_ASSERT_VALUES_EQUAL(scenario.GetScenarioAcceptsAnyUtterance(), true);
        UNIT_ASSERT_VALUES_EQUAL(scenario.GetScenarioName(), "s1");

        UNIT_ASSERT(scenario.BuilderIfExists());
        UNIT_ASSERT_VALUES_EQUAL(scenario.BuilderIfExists(), &scenarioBuilder);
    }

    Y_UNIT_TEST(ResponseBuilderMove) {
        auto skr = TSpeechKitRequestBuilder(SKR_WITH_EVENT).Build();
        TScenarioResponse scenario{"s1", {}, true};

        TResponseBuilder& scenarioBuilder = scenario.ForceBuilder(skr, CreateRequestFromSkr(skr), NMegamind::TFakeGuidGenerator{"GUID1"});
        scenarioBuilder.Reset(skr, CreateRequestFromSkr(skr), NMegamind::TFakeGuidGenerator{"GUID2"}, "s2");

        UNIT_ASSERT_VALUES_EQUAL(scenario.GetScenarioName(), "s2");

        UNIT_ASSERT(scenario.BuilderIfExists());
        TResponseBuilderProto storage;
        TResponseBuilder other{skr, CreateRequestFromSkr(skr), "s2", storage, NMegamind::TFakeGuidGenerator{"GUID2"}};
        UNIT_ASSERT_VALUES_EQUAL(scenario.BuilderIfExists()->ToProto().ShortUtf8DebugString(), other.ToProto().ShortUtf8DebugString());
    }

    Y_UNIT_TEST(RestoreBuilderFromSession) {
        const TString SCENARIO_NAME = "scenario.420";
        auto speechKitRequest = TSpeechKitRequestBuilder(SKR_WITH_EVENT).Build();
        TResponseBuilderProto storage;
        TResponseBuilder responseBuilder(speechKitRequest, CreateRequestFromSkr(speechKitRequest), SCENARIO_NAME, storage);

        TSessionProto::TProtocolInfo protocolInfo;
        protocolInfo.MutableApplyArguments()->PackFrom(google::protobuf::StringValue());

        TAnalyticsInfo analyticsInfo;
        analyticsInfo.SetVersion("18.04");

        TUserInfo userInfo;
        auto* userInfoProperties = userInfo.MutableScenarioUserInfo()->AddProperties();
        userInfoProperties->SetId("scenario.kek");
        userInfoProperties->SetName("scenario_kek");
        userInfoProperties->SetHumanReadable("scenario kek");

        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName(SCENARIO_NAME)
            .SetScenarioResponseBuilder(TMaybe<TResponseBuilderProto>(responseBuilder.ToProto()))
            .SetScenarioSession(SCENARIO_NAME, NewScenarioSession(/* state= */ {}))
            .SetProtocolInfo(protocolInfo)
            .Build();

        TScenarioResponse response{SCENARIO_NAME, {}, /* scenarioAcceptsAnyUtterance= */ true};

        response.ForceBuilderFromSession(speechKitRequest, CreateRequestFromSkr(speechKitRequest), NMegamind::TFakeGuidGenerator{"GUID"}, *session);
        const auto* builder = response.BuilderIfExists();
        UNIT_ASSERT(builder);
        UNIT_ASSERT(builder->ToProto().GetResponse().GetMegamindAnalyticsInfo().GetAnalyticsInfo().empty());
        UNIT_ASSERT(builder->ToProto().GetResponse().GetMegamindAnalyticsInfo().GetUsersInfo().empty());
    }
}

} // namespace
