//
// HOLLYWOOD FRAMEWORK
// Base scenario interface
//

#include "node_caller_testing.h"
#include <alice/hollywood/library/framework/core/render_impl.h>

namespace NAlice::NHollywoodFw::NPrivate {

/*
    TNodeCaller extension for testing purposes
    Initialize TNodeCaller with node name (version for TAppHostNode)
*/
TNodeCallerTesting::TNodeCallerTesting(const TScenario& sc, const TString& nodeName, TTestEnvironment& env)
    : TNodeCaller(sc, nodeName, TestCtx_, &env.GetProtoHwScene(), TRTLogger::StderrLogger())
    , TestEnv_(env)
    , StageToTest_(EStageDeclaration::Undefined)
    , GlobalCtx_(env)
{
    InitializeTestData();
}

/*
    TNodeCaller extension for testing purposes
    Initialize TNodeCaller with stage (version for TTestDispatch/TTestScene/TTestRender)
*/
TNodeCallerTesting::TNodeCallerTesting(const TScenario& sc, EStageDeclaration stage, TTestEnvironment& env)
    : TNodeCaller(sc, /* nodeName */ "", TestCtx_, &env.GetProtoHwScene(), TRTLogger::StderrLogger())
    , TestEnv_(env)
    , StageToTest_(stage)
    , GlobalCtx_(env)
{
    this->LocalGraph_ = sc.GetScenarioGraphs()->FindLocalGraph(stage);
    Y_ENSURE(this->LocalGraph_ != nullptr, "Can't locate apphost node entry for the required stage");
    this->ApphostNodeName_ = this->LocalGraph_->NodeName;

    InitializeTestData();
}

/*
    Bypass (version for testing)
    Calls original procedure and then update TTestEnvironment with collected data
*/
void TNodeCallerTesting::Bypass() {
    TestEnv_.GetProtoHwScene().Clear();
    TNodeCaller::Bypass();
    SaveAllToTestEnv();
}
/*
    Finalize (version for testing)
    Calls original procedure and then update TTestEnvironment with collected data
*/
void TNodeCallerTesting::Finalize() {
    TestEnv_.GetProtoHwScene().Clear();
    PrepareBaseResponseData();
    SaveAllToTestEnv();
}

/*
    Special version for testing purpose
    Simulate single calls for special testing operations
*/
bool TNodeCallerTesting::HasThisStage() const {
    if (!TNodeCaller::HasThisStage()) {
        return false;
    }
    const EStageDeclaration stage = this->GetCurrentStage();
    switch (stage) {
        case EStageDeclaration::Bypass:
        case EStageDeclaration::Finalize:
            // These nodes are always callable
            return true;
        case EStageDeclaration::Undefined:
            return false;
        default:
            if (StageToTest_ == EStageDeclaration::Undefined) {
                return true;
            }
            if (StageToTest_ == stage) {
                return true;
            }
            if (this->Scenario_.IsDebugLocalGraph()) {
                LOG_DEBUG(this->Logger_) << "Node '" << stage << "' skipped due to test requirements";
            }
            return false;
    }
}

/*
    Initialize additional data from test environment
    This function called from both ctors
*/
void TNodeCallerTesting::InitializeTestData() {
    if (!TestEnv_.IsErrorReporting()) {
        this->DisableErrorReporting();
    }

    for (const auto& it : TestEnv_.SourceAnswersJson) {
        NJson::TJsonValue j = it.second;
        TestCtx_.AddItem(std::move(j), it.first);
    }
    for (const auto& it : TestEnv_.SourceAnswersProto) {
        TestCtx_.AddProtobufItem(*(it.second.get()), it.first, NAppHost::EContextItemKind::Output);
    }
    if (TestEnv_.GetProtoHwScene().HasSceneArgs()) {
        const auto sceneName = TestEnv_.GetProtoHwScene().GetSceneArgs().GetSceneName();
        SelectScene(Scenario_.FindScene(sceneName));
    }
    // Add Div Render results from TestEnv_ to context
    for (const auto& it : TestEnv_.DivRenderResponse) {
        TestCtx_.AddProtobufItem(*it, NHollywood::RENDER_DATA_RESULT);
    }
}

/*
    SaveAllToTestEnv()
    Store all collected data from NodeCaller to TTestEnvironment
*/
void TNodeCallerTesting::SaveAllToTestEnv() {
    TestEnv_.GetProtoHwScene() = ExportToProto(true, true);

    if (RunFeatures) {
        NScenarios::TScenarioRunResponse resp;
        RunFeatures->ExportToResponse(resp);
        TestEnv_.RunResponseFeartures = resp.GetFeatures();
    }

    if (Error.Defined()) {
        TestEnv_.RunResponseFeartures.SetIsIrrelevant(true);
    }

    if (SetupRequests != nullptr) {
        TestEnv_.SetupRequests.reset(new TSetup(*SetupRequests));
    }
    // OBSOLETE, will be removed soon
    if (Render) {
        for (const auto& it : Render->GetImpl()->DivRenderData) {
            TestEnv_.DivRenderData.emplace_back(new NRenderer::TDivRenderData);
            TestEnv_.DivRenderData.back()->CopyFrom(it);
        }
    }
    if (RenderArgs && RenderArgs->IsIrrelevant()) {
        TestEnv_.RunResponseFeartures.SetIsIrrelevant(true);
    }
    if (CcaArguments.ArgType != ENodeType::Run) {
        TestEnv_.ContinueCommitApplyArgs = CcaArguments.Args;
    }
    for (const auto& it : DivRenderData) {
        TestEnv_.DivRenderData.push_back(it);
    }
}

NScenarios::TScenarioApplyRequest& TNodeCallerTesting::ConvertRunRequest(const TTestEnvironment& env) {
    *FakeApplyRequest_.MutableBaseRequest() = TestEnv_.RunRequest.GetBaseRequest();
    // Note: DataSources are present in TScenarioFakeApplyRequest_ but never used
    //*FakeApplyRequest_.MutableDataSources() = TestEnv_.RunRequest.GetDataSources();
    *FakeApplyRequest_.MutableInput() = TestEnv_.RunRequest.GetInput();
    *FakeApplyRequest_.MutableArguments() = env.ContinueCommitApplyArgs;
    return FakeApplyRequest_;
}

TNodeCallerTestingRun::TNodeCallerTestingRun(const TScenario& sc, const TString& nodeName, TTestEnvironment& env)
    : TNodeCallerTesting(sc, nodeName, env)
    , RunRequest_(env.RequestMeta,
                  env.RunRequest,
                  Ctx_,
                  &env.GetProtoHwScene(),
                  GlobalCtx_,
                  TRTLogger::StderrLogger(),
                  {sc.GetName(), nodeName, NJson::TJsonValue::UNDEFINED, ENodeType::Run},
                  sc.GetNlg())
{
    SetInputParameters(&env.GetProtoHwScene());
}

TNodeCallerTestingRun::TNodeCallerTestingRun(const TScenario& sc, EStageDeclaration stage, TTestEnvironment& env)
    : TNodeCallerTesting(sc, stage, env)
    , RunRequest_(env.RequestMeta,
                  env.RunRequest,
                  Ctx_,
                  &env.GetProtoHwScene(),
                  GlobalCtx_,
                  TRTLogger::StderrLogger(),
                  {sc.GetName(), "", NJson::TJsonValue::UNDEFINED, ENodeType::Run},
                  sc.GetNlg())
{
    SetInputParameters(&env.GetProtoHwScene());
}

TNodeCallerTestingContinue::TNodeCallerTestingContinue(const TScenario& sc, const TString& nodeName, TTestEnvironment& env)
    : TNodeCallerTesting(sc, nodeName, env)
    , ContinueRequest_(env.RequestMeta,
                       ConvertRunRequest(env),
                       Ctx_,
                       &env.GetProtoHwScene(),
                       GlobalCtx_,
                       TRTLogger::StderrLogger(),
                       {sc.GetName(), nodeName, NJson::TJsonValue::UNDEFINED, ENodeType::Continue},
                       sc.GetNlg())
{
    SetInputParameters(&env.GetProtoHwScene());
}

TNodeCallerTestingContinue::TNodeCallerTestingContinue(const TScenario& sc, EStageDeclaration stage, TTestEnvironment& env)
    : TNodeCallerTesting(sc, stage, env)
    , ContinueRequest_(env.RequestMeta,
                       ConvertRunRequest(env),
                       Ctx_,
                       &env.GetProtoHwScene(),
                       GlobalCtx_,
                       TRTLogger::StderrLogger(),
                       {sc.GetName(), "", NJson::TJsonValue::UNDEFINED, ENodeType::Continue},
                       sc.GetNlg())
{
    SetInputParameters(&env.GetProtoHwScene());
}
TNodeCallerTestingApply::TNodeCallerTestingApply(const TScenario& sc, const TString& nodeName, TTestEnvironment& env)
    : TNodeCallerTesting(sc, nodeName, env)
    , ApplyRequest_(env.RequestMeta,
                    ConvertRunRequest(env),
                    Ctx_,
                    &env.GetProtoHwScene(),
                    GlobalCtx_,
                    TRTLogger::StderrLogger(),
                    {sc.GetName(), nodeName, NJson::TJsonValue::UNDEFINED, ENodeType::Apply},
                    sc.GetNlg())
{
    SetInputParameters(&env.GetProtoHwScene());
}

TNodeCallerTestingApply::TNodeCallerTestingApply(const TScenario& sc, EStageDeclaration stage, TTestEnvironment& env)
    : TNodeCallerTesting(sc, stage, env)
    , ApplyRequest_(env.RequestMeta,
                    ConvertRunRequest(env),
                    Ctx_,
                    &env.GetProtoHwScene(),
                    GlobalCtx_,
                    TRTLogger::StderrLogger(),
                    {sc.GetName(), "", NJson::TJsonValue::UNDEFINED, ENodeType::Apply},
                    sc.GetNlg())
{
    SetInputParameters(&env.GetProtoHwScene());
}
TNodeCallerTestingCommit::TNodeCallerTestingCommit(const TScenario& sc, const TString& nodeName, TTestEnvironment& env)
    : TNodeCallerTesting(sc, nodeName, env)
    , CommitRequest_(env.RequestMeta,
                     ConvertRunRequest(env),
                     Ctx_,
                     &env.GetProtoHwScene(),
                     GlobalCtx_,
                     TRTLogger::StderrLogger(),
                     {sc.GetName(), nodeName, NJson::TJsonValue::UNDEFINED, ENodeType::Commit},
                     sc.GetNlg())
{
    SetInputParameters(&env.GetProtoHwScene());
}

TNodeCallerTestingCommit::TNodeCallerTestingCommit(const TScenario& sc, EStageDeclaration stage, TTestEnvironment& env)
    : TNodeCallerTesting(sc, stage, env)
    , CommitRequest_(env.RequestMeta,
                     ConvertRunRequest(env),
                     Ctx_,
                     &env.GetProtoHwScene(),
                     GlobalCtx_,
                     TRTLogger::StderrLogger(),
                     {sc.GetName(), "", NJson::TJsonValue::UNDEFINED, ENodeType::Commit},
                     sc.GetNlg())
{
    SetInputParameters(&env.GetProtoHwScene());
}

} // namespace NAlice::NHollywoodFw::NPrivate
