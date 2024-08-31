#include "framework_migration.h"

#include <alice/hollywood/library/framework/proto/default_render.pb.h>
#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

constexpr TStringBuf NLG_NAME = "NlgName";
constexpr TStringBuf PHRASE = "Phrase";
constexpr TStringBuf CONTEXT = "Context";

// Will use 'TProtoRenderDefaultNlg' as a valid scenario state proto
TProtoRenderDefaultNlg MakeGoodScenarioState() {
    TProtoRenderDefaultNlg state;

    state.SetNlgName(TString(NLG_NAME));
    state.SetPhrase(TString(PHRASE));
    state.SetContext(TString(CONTEXT));
    return state;
}

bool ValidateScenarioState(const TProtoRenderDefaultNlg& state) {
    return state.GetNlgName() == NLG_NAME &&
           state.GetPhrase() == PHRASE &&
           state.GetContext() == CONTEXT;
}

} // anonimous namespace

Y_UNIT_TEST_SUITE(FrameMigrationTest) {

    Y_UNIT_TEST(MigrationTestOld) {

        // Make old style scenario state
        NScenarios::TScenarioBaseRequest baseRequest;
        baseRequest.MutableState()->PackFrom(MakeGoodScenarioState());

        TProtoRenderDefaultNlg state;
        UNIT_ASSERT(NAlice::NHollywood::ReadScenarioState(baseRequest, state));
        UNIT_ASSERT(ValidateScenarioState(state));
    }

    Y_UNIT_TEST(MigrationTestNew) {

        // Make new style scenario state
        NScenarios::TScenarioBaseRequest baseRequest;
        TProtoHwFramework frameworkState;
        frameworkState.MutableScenarioState()->PackFrom(MakeGoodScenarioState());
        baseRequest.MutableState()->PackFrom(frameworkState);

        TProtoRenderDefaultNlg state;
        UNIT_ASSERT(NAlice::NHollywood::ReadScenarioState(baseRequest, state));
        UNIT_ASSERT(ValidateScenarioState(state));
    }

    Y_UNIT_TEST(MigrationTestArgumentsOld) {

        // Make old style scenario state
        NScenarios::TScenarioApplyRequest applyRequest;
        applyRequest.MutableArguments()->PackFrom(MakeGoodScenarioState());

        TProtoRenderDefaultNlg state;
        UNIT_ASSERT(NAlice::NHollywood::ReadArguments(applyRequest, state));
        UNIT_ASSERT(ValidateScenarioState(state));
    }

    Y_UNIT_TEST(MigrationTestArgumentsNew) {

        // Make new style scenario state
        NScenarios::TScenarioApplyRequest applyRequest;
        TProtoHwSceneCCAArguments frameworkArguments;
        frameworkArguments.MutableScenarioArgs()->PackFrom(MakeGoodScenarioState());
        applyRequest.MutableArguments()->PackFrom(frameworkArguments);

        TProtoRenderDefaultNlg state;
        UNIT_ASSERT(NAlice::NHollywood::ReadArguments(applyRequest, state));
        UNIT_ASSERT(ValidateScenarioState(state));
    }
}

} // namespace NAlice::NHollywoodFw
