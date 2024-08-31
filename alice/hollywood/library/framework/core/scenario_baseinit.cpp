//
// HOLLYWOOD FRAMEWORK
// Base scenario interface
//

#include "default_renders.h"
#include "node_caller.h"
#include "scenario.h"
#include "scenario_factory.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/metrics/metrics.h>


namespace NAlice::NHollywoodFw {

TScenarioGraphFlow TScenarioBase::ScenarioRequest() {
    auto p = TScenarioGraphFlow(NPrivate::NAME_FOR_INITIAL_REQUEST, "", EStageDeclaration::Undefined);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlow TScenarioBase::ScenarioResponse() {
    auto p = TScenarioGraphFlow(NPrivate::NAME_FOR_FINAL_RESPONSE, "", EStageDeclaration::Undefined);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlowContinue TScenarioBase::ScenarioContinue() {
    auto p = TScenarioGraphFlowContinue(NPrivate::NAME_FOR_CONTINUE, "", EStageDeclaration::Undefined);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlowApply TScenarioBase::ScenarioApply() {
    auto p = TScenarioGraphFlowApply(NPrivate::NAME_FOR_APPLY, "", EStageDeclaration::Undefined);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlowCommit TScenarioBase::ScenarioCommit(){
    auto p = TScenarioGraphFlowCommit(NPrivate::NAME_FOR_COMMIT, "", EStageDeclaration::Undefined);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlow TScenarioBase::NodePreselect(TStringBuf apphostNodeName /*= "preselect"*/, TStringBuf expFlag /*= ""*/) {
    auto p = TScenarioGraphFlow(apphostNodeName, expFlag, EStageDeclaration::DispatchSetup);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlow TScenarioBase::NodeRun(TStringBuf apphostNodeName /*= "run"*/, TStringBuf expFlag /*= ""*/) {
    auto p = TScenarioGraphFlow(apphostNodeName, expFlag, EStageDeclaration::Dispatch);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlow TScenarioBase::NodeMain(TStringBuf apphostNodeName /*= "main"*/, TStringBuf expFlag /*= ""*/) {
    auto p = TScenarioGraphFlow(apphostNodeName, expFlag, EStageDeclaration::SceneMainRun);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlow TScenarioBase::NodeRender(TStringBuf apphostNodeName /*= "render"*/, TStringBuf expFlag /*= ""*/) {
    auto p = TScenarioGraphFlow(apphostNodeName, expFlag, EStageDeclaration::Render);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlowContinue TScenarioBase::NodeContinueSetup(TStringBuf apphostNodeName /*= "continue_setup"*/, TStringBuf expFlag /*= ""*/) {
    auto p = TScenarioGraphFlowContinue(apphostNodeName, expFlag, EStageDeclaration::SceneSetupContinue);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlowContinue TScenarioBase::NodeContinue(TStringBuf apphostNodeName /*= "continue"*/, TStringBuf expFlag /*= ""*/) {
    auto p = TScenarioGraphFlowContinue(apphostNodeName, expFlag, EStageDeclaration::SceneMainContinue);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlowApply TScenarioBase::NodeApplySetup(TStringBuf apphostNodeName /*= "apply_setup"*/, TStringBuf expFlag /*= ""*/) {
    auto p = TScenarioGraphFlowApply(apphostNodeName, expFlag, EStageDeclaration::SceneSetupApply);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlowApply TScenarioBase::NodeApply(TStringBuf apphostNodeName /*= "apply"*/, TStringBuf expFlag /*= ""*/) {
    auto p = TScenarioGraphFlowApply(apphostNodeName, expFlag, EStageDeclaration::SceneMainApply);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlowCommit TScenarioBase::NodeCommitSetup(TStringBuf apphostNodeName /*= "commit_setup"*/, TStringBuf expFlag /*= ""*/) {
    auto p = TScenarioGraphFlowCommit(apphostNodeName, expFlag, EStageDeclaration::SceneSetupCommit);
    NodeCache_.push_back(p);
    return p;
}
TScenarioGraphFlowCommit TScenarioBase::NodeCommit(TStringBuf apphostNodeName /*= "commit"*/, TStringBuf expFlag /*= ""*/) {
    auto p = TScenarioGraphFlowCommit(apphostNodeName, expFlag, EStageDeclaration::SceneMainCommit);
    NodeCache_.push_back(p);
    return p;
}

/*
    Base scenario ctor

    @param name - scenario name (must be lower_case)
*/
TScenarioBase::TScenarioBase(TStringBuf scenarioName)
    : Name_(scenarioName)
    , InitializationStageFlag_(true)
    , DebugGraphFlag_(false)
{
    TString testName(scenarioName);
    Y_ENSURE(!testName.to_lower(), "Scenario name '" << scenarioName << "' must be in 'lower_case' format!");

    // Set default name
    ProductScenarioName_ = scenarioName;
}

void TScenarioBase::SetNlgRegistration(NHollywood::TCompiledNlgComponent::TRegisterFunction registerFunction) {
    Y_ENSURE(InitializationStageFlag_, "This call is allowed in ctor only!");
    NlgRegisterFunction_ = std::move(registerFunction);
}

void TScenarioBase::SetProductScenarioName(TStringBuf productScenarioName) {
    Y_ENSURE(InitializationStageFlag_, "This call is allowed in ctor only!");
    ProductScenarioName_ = productScenarioName;
}

void TScenarioBase::SetApphostGraph(const TScenarioGraphFlow& flow) {
    Y_ENSURE(InitializationStageFlag_, "This call is allowed in ctor only!");
    if (ScenarioGraphs_ == nullptr) {
        ScenarioGraphs_.reset(new NPrivate::TScenarioGraphs);
    }
    ScenarioGraphs_->AttachGraph(flow);
}

NPrivate::TScenarioRenderBaseCaller* TScenarioBase::FindRender(const TString& protoName, const TString& scenePath) const {
    auto p = FindIfPtr(RendererList_, [protoName, scenePath](const auto& node) {
        return node->IsSame(scenePath, protoName);
    });
    if (p == nullptr) {
        return nullptr;
    }
    return p->get();
}

TScenarioBase::THookInputInfo::THookInputInfo(ENodeType nodeType,
                                              NAppHost::IServiceContext& ctx,
                                              const NScenarios::TRequestMeta& meta,
                                              const TProtoHwScene* protoHwScene,
                                              const NHollywood::TScenarioNewContext* newContext)
    : NodeType(nodeType)
    , Ctx(ctx)
    , Meta(meta)
    , NewContext(newContext)
    , SceneName((protoHwScene && protoHwScene->HasSceneArgs()) ? protoHwScene->GetSceneArgs().GetSceneName() : TString{})
    , SceneAruments((protoHwScene && protoHwScene->HasSceneArgs()) ? protoHwScene->GetSceneArgs().GetArgs() : google::protobuf::Any{})
    , RenderArguments((protoHwScene && protoHwScene->HasRendererArgs()) ? protoHwScene->GetRendererArgs().GetArgs() : google::protobuf::Any{})
{
}

} // namespace NAlice::NHollywoodFw
