#include "test_nodes.h"
#include "node_caller_testing.h"

#include <alice/hollywood/library/framework/core/node_caller.h>
#include <alice/hollywood/library/framework/core/render_impl.h>
#include <alice/hollywood/library/framework/core/scenario_factory.h>

#include <util/string/join.h>

namespace NAlice::NHollywoodFw {

/*
    Prepare data for initial test call (TTestEnvironment >> TestFn() >> TTestEnvironment)
                                                                    ^^^^
    Where TestFn is an any test class based on TTestBase
*/
bool NPrivate::TTestBase::operator >> (TTestEnvironment& rhs) const {
    Y_ENSURE(TestEnvironment, "Test Environment must be set before this call");

    const TString& scenarioName = TestEnvironment->GetScenarioName();
    const TScenario* sc = NPrivate::TScenarioFactory::Instance().FindScenarioName(scenarioName);
    Y_ENSURE(sc != nullptr, "Test scenario '" << scenarioName << "' must exist");
    NPrivate::TScenarioFactory::Instance().EnsureScenarioInitialization(scenarioName);

    if (&rhs != TestEnvironment) {
        // Move all data from source Environment to target, but clear some output specific protos
        rhs = *TestEnvironment;
        rhs.DivRenderData.clear();
        rhs.ResponseBody.Clear();
        rhs.RunResponseFeartures.Clear();
        rhs.RunRequestUserInfo.Clear();
    }
    // Call TTestXxx specific functions
    return CallTest(*sc, rhs);
}

/*
    Call unit test for Dispatch/DispatchSetup functions
*/
bool TTestDispatch::CallTest(const TScenario& sc, TTestEnvironment& te) const {
    NPrivate::TNodeCallerTestingRun caller(sc, InitialStage_, te);
    NPrivate::TScenarioFactory::Instance().DispatchScenarioHandleUt(te, caller, caller.GetGlobalCtx());
    return !caller.Error.Defined();
}

/*
    Call unit test for Render functions
*/
bool TTestRender::CallTest(const TScenario& sc, TTestEnvironment& te) const {
    NPrivate::TNodeCallerTestingRun caller(sc, EStageDeclaration::Render, te);

    // Prepare render arguments from TTestRender data
    google::protobuf::Any copy;
    copy.CopyFrom(RenderProto_);
    TString scenePath = "";
    switch (RenderPath_) {
        case ERenderPath::Standalone:
            break; // Keep as empty string
        case ERenderPath::Scenario:
            scenePath = sc.GetName();
            break;
        case ERenderPath::Scene:
            scenePath = Join("/", sc.GetName(), te.GetSelectedSceneName());
    }
    caller.RenderArgs.reset(new NPrivate::TRetRenderSelector(std::move(copy), scenePath));

    NPrivate::TScenarioFactory::Instance().DispatchScenarioHandleUt(te, caller, caller.GetGlobalCtx());
    return !caller.Error.Defined();
}

/*
    Call unit test for SceneSetup/Scene functions
*/
bool TTestScene::CallTest(const TScenario& sc, TTestEnvironment& te) const {
    NPrivate::TNodeCallerTestingRun caller(sc, EStageDeclaration::SceneMainRun, te);

    // Prepare scene arguments from TTestScene
    caller.SceneArgs.reset(new NPrivate::TRetSceneSelector(SceneArgs_));
    NPrivate::TScenarioFactory::Instance().DispatchScenarioHandleUt(te, caller, caller.GetGlobalCtx());
    return !caller.Error.Defined();
}

/*
    Call unit test for apphost node
*/
bool TTestApphost::CallTest(const TScenario& sc, TTestEnvironment& te) const {
    Y_ENSURE(sc.GetScenarioGraphs()->FindLocalGraph(NodeName_, nullptr), "Test scenario '" << sc.GetName() <<
             "' doesn't have incoming node '" << NodeName_ << '\'');

    const NPrivate::TApphostNode* localGraph = sc.GetScenarioGraphs()->FindLocalGraph(NodeName_, nullptr);
    Y_ENSURE(localGraph != nullptr, "Can not find a handler for scenario " << sc.GetName() << "; node: " << NodeName_);

    // Select what kind of request will be used to set - TScenarioRunRequest or TScenarioApplyRequest
    switch (localGraph->NodeType) {
        case ENodeType::Run: {
            NPrivate::TNodeCallerTestingRun caller(sc, NodeName_, te);
            NPrivate::TScenarioFactory::Instance().DispatchScenarioHandleUt(te, caller, caller.GetGlobalCtx());
            return !caller.Error.Defined();
        }
        case ENodeType::Continue: {
            NPrivate::TNodeCallerTestingContinue caller(sc, NodeName_, te);
            // For direct call 'Continue' node scene must be selected
            // exept case when error occured on previous stage (for example, in Dispatch or DispatchSetup)
            Y_ENSURE(caller.Error.Defined() || caller.GetScenarioScene() != nullptr, "Scene must be selected in TestEnvironment. Node " << NodeName_);
            NPrivate::TScenarioFactory::Instance().DispatchScenarioHandleUt(te, caller, caller.GetGlobalCtx());
            return !caller.Error.Defined();
        }
        case ENodeType::Apply: {
            NPrivate::TNodeCallerTestingApply caller(sc, NodeName_, te);
            // For direct call 'Continue' node scene must be selected
            // exept case when error occured on previous stage (for example, in Dispatch or DispatchSetup)
            Y_ENSURE(caller.Error.Defined() || caller.GetScenarioScene() != nullptr, "Scene must be selected in TestEnvironment. Node " << NodeName_);
            NPrivate::TScenarioFactory::Instance().DispatchScenarioHandleUt(te, caller, caller.GetGlobalCtx());
            return !caller.Error.Defined();
        }
        case ENodeType::Commit: {
            NPrivate::TNodeCallerTestingCommit caller(sc, NodeName_, te);
            // For direct call 'Continue' node scene must be selected
            // exept case when error occured on previous stage (for example, in Dispatch or DispatchSetup)
            Y_ENSURE(caller.Error.Defined() || caller.GetScenarioScene() != nullptr, "Scene must be selected in TestEnvironment. Node " << NodeName_);
            NPrivate::TScenarioFactory::Instance().DispatchScenarioHandleUt(te, caller, caller.GetGlobalCtx());
            return !caller.Error.Defined();
        }
    }
}

/*
    Set text or voice answer directly to render instead of NLG
*/
void TTestRender::SetTextAnswer(TRender& render, const TString& answer) {
    render.GetImpl()->TextResponse = answer;
}

void TTestRender::SetVoiceAnswer(TRender& render, const TString& answer) {
    render.GetImpl()->VoiceResponse = answer;
}

} // namespace NAlice::NHollywoodFw
