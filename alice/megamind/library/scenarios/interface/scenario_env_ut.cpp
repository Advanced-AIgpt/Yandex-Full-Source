#include <alice/megamind/library/scenarios/interface/scenario_env.h>
#include <alice/megamind/library/scenarios/features/protos/features.pb.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/test.pb.h>

#include <google/protobuf/util/message_differencer.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

TSemanticFrame MakeSemanticFrame(const TString& name) {
    TSemanticFrame frame{};
    frame.SetName(name);
    return frame;
}

Y_UNIT_TEST_SUITE(WithTypedTest) {
    Y_UNIT_TEST(StateTest) {
        const TString expectedStateParam = "Expected state";
        const TString expectedResult = "Hi!";

        TTestRequiredFieldsState expectedRequiredFieldsState;
        expectedRequiredFieldsState.SetS(expectedStateParam);
        expectedRequiredFieldsState.SetD(15);

        google::protobuf::Any expectedState;
        expectedState.PackFrom(expectedRequiredFieldsState);

        TTestRequiredFieldsState customState;
        customState.SetS("Don't worry");
        customState.SetD(12);

        TState state;
        state.MutableState()->PackFrom(customState);

        auto result = WithTypedState<TTestRequiredFieldsState>(state, [&](TTestRequiredFieldsState& s) {
            s.SetS(expectedStateParam);
            s.SetD(15);
            return expectedResult;
        });

        UNIT_ASSERT_EQUAL(result, expectedResult);
        UNIT_ASSERT(google::protobuf::util::MessageDifferencer::Equivalent(state.GetState(), expectedState));
    }

    Y_UNIT_TEST(BadEnvTest) {
        using TTypedEnv = TTypedScenarioEnv<TTestRequiredFieldsState>;

        TMockContext context;
        TState state;
        TFeatures features;
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;

        TVector<TSemanticFrame> frames;
        frames.push_back(MakeSemanticFrame("first"));
        frames.push_back(MakeSemanticFrame("second"));

        const TTestSpeechKitRequest speechKitRequest =
            TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);

        TScenarioEnv env{
            context, request, frames, state, *features.MutableScenarioFeatures(), analyticsInfoBuilder, userInfoBuilder};

        auto badFunc = [&env]() {
            WithTypedEnv<TTestRequiredFieldsState>(
                env, [&](TTypedEnv& /* e */) { return "Hi!"; });
        };

        UNIT_ASSERT_EXCEPTION(badFunc(), yexception);
    }

    Y_UNIT_TEST(GoodEnvTest) {
        using TTypedEnv = TTypedScenarioEnv<TTestRequiredFieldsState>;

        const TString expectedStateParam = "Expected state";
        const TString expectedFeatureParam = "Expected feature";
        const TString expectedResult = "Hi!";

        TTestRequiredFieldsState expectedRequiredFieldsState;
        expectedRequiredFieldsState.SetS(expectedStateParam);
        expectedRequiredFieldsState.SetD(15);

        TState expectedState;
        expectedState.MutableState()->PackFrom(expectedRequiredFieldsState);

        TMockContext context;
        TState state;
        TFeatures features;
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;

        TVector<TSemanticFrame> frames;
        frames.push_back(MakeSemanticFrame("first"));
        frames.push_back(MakeSemanticFrame("second"));

        const TTestSpeechKitRequest speechKitRequest =
            TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);

        TScenarioEnv env{context, request, frames, state, *features.MutableScenarioFeatures(), analyticsInfoBuilder, userInfoBuilder};

        auto resultOfGoodFunc =
            WithTypedEnv<TTestRequiredFieldsState>(env, [&](TTypedEnv& e) {
                e.State.SetS(expectedStateParam);
                e.State.SetD(15);
                return expectedResult;
            });

        UNIT_ASSERT_EQUAL(resultOfGoodFunc, expectedResult);
        UNIT_ASSERT(google::protobuf::util::MessageDifferencer::Equivalent(env.State, expectedState));
    }
}

} // namespace
