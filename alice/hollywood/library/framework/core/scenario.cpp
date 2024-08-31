//
// HOLLYWOOD FRAMEWORK
// Base scenario interface
//

#include "scenario.h"

#include "default_renders.h"
#include "node_caller.h"
#include "scenario_factory.h"

#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/metrics/metrics.h>

#include <util/string/join.h>

namespace NAlice::NHollywoodFw {

IScenarioRegistratorBase::IScenarioRegistratorBase() {
    NPrivate::TScenarioFactory::Instance().AttachRegitrator(this);
}

/*
    Base scenario ctor
    @param name - scenario name (must be lower_case)
    Use HW_REGISTER to create scenario
*/
TScenario::TScenario(TStringBuf name)
    : TScenarioBase(name)
{
    NPrivate::TScenarioFactory::Instance().RegisterScenario(this);
}

/*
    Base scenario dtor
*/
TScenario::~TScenario() {
    NPrivate::TScenarioFactory::Instance().UnRegisterScenario(this);
}

/*
    Finalize initilaization of scenario
    Check all possible cases, conflicts and dependencies
*/
void TScenario::FinishInitialization(const NNlg::ITranslationsContainerPtr& nlgTranslations,
                                     const TFsPath& resourcesBasePath)
{
    Y_ENSURE(InitializationStageFlag_, "This call is allowed in ctor only!");

    // Attach Irrelevant renderer from factory
    RegisterRenderer(&NPrivate::RenderIrrelevant);
    // Attach standard renderer from factory
    RegisterRenderer(&NPrivate::RenderDefaultNlg);

    // Initialize scenario data and parameters
    if (resourcesBasePath.IsDefined() && Resources_) {
        Cerr << "Loading resources for scenario " << GetName() << Endl;
        Resources_->LoadFromPath(resourcesBasePath / GetName()); // TODO(a-square): load in parallel?
    }

    // Check and attach default apphost graphs
    if (ScenarioGraphs_ == nullptr) {
        SetApphostGraph(ScenarioRequest() >> NodeRun() >> NodeMain() >> ScenarioResponse());
        if (HasNode(EStageDeclaration::SceneMainContinue)) {
            SetApphostGraph(ScenarioContinue() >> NodeContinueSetup() >> NodeContinue() >> ScenarioResponse());
        }
        if (HasNode(EStageDeclaration::SceneMainApply)) {
            SetApphostGraph(ScenarioApply() >> NodeApplySetup() >> NodeApply() >> ScenarioResponse());
        }
        if (HasNode(EStageDeclaration::SceneMainCommit)) {
            SetApphostGraph(ScenarioCommit() >> NodeCommitSetup() >> NodeCommit() >> ScenarioResponse());
        }
    }

    // Collect information about all registered stages
    TVector<EStageDeclaration> allStagesInScenario;
    for (const auto& it : HandlersMap_) {
        allStagesInScenario.push_back(it->GetStage());
    }
    for (const auto& it : AllScenes_) {
        for (const auto& itScene : it->HandlersMap_) {
            allStagesInScenario.push_back(itScene->GetStage());
        }
    }
    allStagesInScenario.push_back(EStageDeclaration::Render);
    if (IsDebugLocalGraph()) {
        Cout << "Scenario '" << Name_ << "' graph information:" << Endl;
    }
    ScenarioGraphs_->BuildAllGraphs(allStagesInScenario, IsDebugLocalGraph());

    // Checking main functions
    int dispatchCount = 0;
    int dispatchSetupCount = 0;
    for (const auto& it : HandlersMap_) {
        switch (it->GetStage()) {
            case EStageDeclaration::Dispatch:
                dispatchCount++;
                break;
            case EStageDeclaration::DispatchSetup:
                dispatchSetupCount++;
                break;
            default:
                Y_ENSURE(false, "Undefined function inside scenario " << GetName());
        }
    }
    Y_ENSURE(dispatchCount == 1, "One Dispatch() function must be inited inside scenario " << GetName());
    Y_ENSURE(dispatchSetupCount <= 1, "DispatchSetup() function can be registered only one time. Scenario " << GetName());

    // Check entities
    Y_ENSURE(!AllScenes_.empty(), "At least one scene must be inited inside scenario "<< GetName());
    for (const auto& it : AllScenes_) {
        it->FinishInitialization();
    }

    // Check that all scenes has different names
    TSet<TString> allSceneNames;
    for (const auto& it : AllScenes_) {
        Y_ENSURE(!allSceneNames.contains(it->GetSceneName()), "Two scenes has the same name: '" << it->GetSceneName() << "'. Scenario "<< GetName());
        allSceneNames.insert(it->GetSceneName());
    }

    // Check that all apphost graphs are present in scenario functions
    for (const auto& itGraphs : ScenarioGraphs_->GetAllGraphs()) {
        for (const auto& it : itGraphs.LocalGraphs) {
            switch (NPrivate::TScenarioGraphs::GetStageType(it)) {
                case NPrivate::TScenarioGraphs::EStageType::ScenarioSetup:
                case NPrivate::TScenarioGraphs::EStageType::SceneSetup:
                    // These stages are optional don't need to check it
                    break;
                case NPrivate::TScenarioGraphs::EStageType::SceneMain:
                case NPrivate::TScenarioGraphs::EStageType::Renderer:
                case NPrivate::TScenarioGraphs::EStageType::ScenarioDispatch:
                    Y_ENSURE(HasNode(it), "Apphost graph node '" << itGraphs.NodeName <<
                        "' requires missing function '" << it << "'. Scenario " << GetName());
                    break;
                default:
                    // Hidden and auxilary stages (not present in scenario functions)
                    break;
            }
        }
    }

    // Check that the all renderer functions has different protos/typeids
    for (const auto& it1 : RendererList_) {
        for (const auto& it2 : RendererList_) {
            if (it1.get() == it2.get()) {
                // holders it1 == it2 - ignore
                continue;
            }
            Y_ENSURE(*it1 != *it2,
                "Two render functions has the same protobuf '" << it1->GetMsgName() << "'. Object path: '" <<
                it1->GetScenePath() << "'. You may use the same proto in render functions only if they are placed in different classes");
        }
    }

    if (NlgRegisterFunction_) {
        auto nlgInitRng = TRng(4);
        Nlg_ = NHollywood::TCompiledNlgComponent(nlgInitRng, nlgTranslations, NlgRegisterFunction_);
    }

    InitializationStageFlag_ = false;
}

const NPrivate::TScenarioBaseCaller* TScenario::FindScenarioNodeByStage(EStageDeclaration stage) const {
    const auto* handler = FindIfPtr(HandlersMap_, [stage](const auto& node) -> bool {
        return node->GetStage() == stage;
    });
    return (handler != nullptr) ? handler->get() : nullptr;
}

const TSceneBase* TScenario::FindScene(const TString& sceneName) const {
    for (const auto& it : AllScenes_) {
        if (it->GetSceneName() == sceneName) {
            return it.get();
        }
    }
    return nullptr;
}

bool TScenario::HasNode(EStageDeclaration stage) const {
    if (stage == EStageDeclaration::Render) {
        return !RendererList_.empty();
    }
    if (FindScenarioNodeByStage(stage)) {
        return true;
    }
    for (const auto& it : AllScenes_) {
        if (it->FindHandlerByStage(stage)) {
            return true;
        }
    }
    return false;
}

NPrivate::TRetRenderSelectorIrrelevant TScenario::TReturnValueRenderIrrelevant(const TString& nlgName,
                                                                               const TString& phrase,
                                                                               TRunFeatures&& features /*= TRunFeatures{}*/) const {
    return NPrivate::TRetRenderSelectorIrrelevant(nlgName, phrase, TProtoHwfEmptyMessage{}, std::move(features));
}
NPrivate::TRetRenderSelectorIrrelevant TScenario::TReturnValueRenderIrrelevant(const TString& nlgName,
                                                                               const TString& phrase,
                                                                               const google::protobuf::Message& context,
                                                                               TRunFeatures&& features /*= TRunFeatures{}*/) const {
    return NPrivate::TRetRenderSelectorIrrelevant(nlgName, phrase, context, std::move(features));
}

/*
    Call a Dispatch/DispatchSetup node from a current scenario (main nodes)
*/
void TScenario::CallMainNode(NPrivate::TNodeCaller& callerData, NMetrics::ISensors& s) const {
    const NPrivate::TScenarioBaseCaller* node = FindScenarioNodeByStage(callerData.GetCurrentStage());
    if (node == nullptr) {
        callerData.SetError("Can not find a handler in scenario map.");
        return;
    }

    switch (callerData.GetCurrentStage()) {
        case EStageDeclaration::DispatchSetup:
            CallDispatchSetup(callerData, node);
            break;
        case EStageDeclaration::Dispatch:
            CallDispatch(callerData, node, s);
            break;
        default:
            callerData.SetError("Undefined stage selector while trying a call node");
    }
    return;
}

/*
*/
void TScenario::CallDispatchSetup(NPrivate::TNodeCaller& callerData, const NPrivate::TScenarioBaseCaller* node) const {
    TRetSetup res = node->Call(callerData.GetRunRequest(), callerData.GetStorage());

    struct Visitor {
        NPrivate::TNodeCaller& CallerData;
        const TScenario* Scenario;
        void operator()(const TSetup& setup) {
            CallerData.SetupRequests.reset(new TSetup(setup));
        }
        void operator()(const TReturnValueDo&) {
            // Force scenario switch to old flow
            CallerData.SetSwitchToOldFlow();
        }
        void operator()(const TError& err) {
            CallerData.SetErrorFromScenario(err);
        }
    } visitor{callerData, this};
    std::visit(visitor, res);
}

/*
*/
void TScenario::CallDispatch(NPrivate::TNodeCaller& callerData, const NPrivate::TScenarioBaseCaller* node, NMetrics::ISensors& s) const {
    TRetScene res = node->Call(callerData.GetRunRequest(), callerData.GetStorage(), callerData.GetSource());

    struct Visitor {
        NPrivate::TNodeCaller& CallerData;
        NMetrics::ISensors& Sensors;

        void operator()(const NPrivate::TRetSceneSelector& rv) {
            const TSceneBase* scene = CallerData.GetScenario().FindScene(rv.GetSceneName());
            if (scene == nullptr) {
                CallerData.SetError(TStringBuilder{} << "Scene is not found in scenario. Scene name: " << rv.GetSceneName());
                return;
            }
            if (scene->GetProtoClassName() != rv.GetArgumentsClassName()) {
                CallerData.SetError(TStringBuilder{} << "Scene protobuf is different with requested. Scene name: " << rv.GetSceneName());
                return;
            }
            if (NPrivate::TScenarioFactory::Instance().IsHollywoodApp()) {
                // Add information for Setrace tagger only of we are in real Hollywood application (not in UnitTests)
                const TString tag = TString::Join("HWF scene: ", CallerData.GetScenario().GetName(), ':', rv.GetSceneName());
                LOG_INFO(CallerData.Logger()) << TLogMessageTag{tag} << tag;

                Sensors.IncRate(NHollywood::SceneLabels(CallerData.GetScenario(), rv.GetSceneName()));
            }

            CallerData.SelectScene(scene);
            CallerData.SceneArgs.reset(new NPrivate::TRetSceneSelector(rv));
            // TODO [DD] This code must be refactored after next release
            //  - need to remove SelectedIntent from scene protobuf
            //  - need to replace CallerData.SceneArgs with another structure
            //  - always use intent from AI()
            CallerData.GetRunRequest().AI().OverrideIntentIfEmpty(rv.GetSelectedIntent());
            if (CallerData.IsNeedDumpSceneArgs()) {
                CallerData.DebugDump(rv.GetArguments(), "SCENE ARGS");
            }
        }
        void operator()(const NPrivate::TRetRenderSelectorIrrelevant& rv) {
            // rv may contains either ReturnType::Scene or ReturnType::Renderer
            CallerData.RenderArgs.reset(new NPrivate::TRetRenderSelectorIrrelevant(rv));
            if (rv.GetFeatures().Defined()) {
                CallerData.RunFeatures.reset(new TRunFeatures(rv.GetFeatures()));
            }
            if (CallerData.IsNeedDumpRenderArgs()) {
                CallerData.DebugDump(rv.GetArguments(), "RENDER ARGS");
            }
        }
        void operator()(const TReturnValueDo&) {
            // Force scenario switch to old flow
            CallerData.SetSwitchToOldFlow();
        }
        void operator()(const TError& err) {
            CallerData.SetErrorFromScenario(err);
        }
    } visitor{callerData, s};
    std::visit(visitor, res);
}

/*
    Call a Render node
*/
void TScenario::CallRenderNode(NPrivate::TNodeCaller& callerData) const {
    if (callerData.Error.Defined()) {
        // Ignore rendering function if error occured
        return;
    }
    //
    // Create Render object with BaseResponseProto only if we are in /run node
    //
    callerData.Render.reset(new TRender(callerData, Nlg_));
    const TString protoClassName = callerData.GetRenderArgs().GetArgumentsClassName();
    NPrivate::TScenarioRenderBaseCaller* rendererFn = FindRender(protoClassName, callerData.GetRenderArgs().GetScenePath());
    if (rendererFn == nullptr) {
        callerData.SetError(TStringBuilder{} << "Rendering for protobuf not found. Protobuf: " <<
                            protoClassName << "; path: " << callerData.GetRenderArgs().GetScenePath());
        return;
    }
    TRetResponse res = rendererFn->Call(callerData.GetRenderArgs().GetArguments(), *callerData.Render);

    struct Visitor {
        NPrivate::TNodeCaller& CallerData;
        void operator()(TReturnValueSuccess&) {
            // Does nothing
        }
        void operator()(TReturnValueDo&) {
            // Force scenario switch to old flow
            CallerData.SetSwitchToOldFlow();
        }
        void operator()(const TError& err) {
            CallerData.SetErrorFromScenario(err);
        }
    } visitor{callerData};
    std::visit(visitor, res);
}

/*
    Dump scenario information to JSON output
    This function called when --hwf-dump flag is used
*/
NJson::TJsonValue TScenario::DumpScenarioInfo() const {
    NJson::TJsonValue scenario(NJson::EJsonValueType::JSON_MAP);
    // Base info
    scenario["name"] = Name_;
    scenario["type"] = OldScenarioFlow_ ? "linked" : "hwf";
    // All nodes
    if (ScenarioGraphs_) {
        NJson::TJsonValue nodesRoot(NJson::EJsonValueType::JSON_ARRAY);
        for (const auto& it : ScenarioGraphs_->GetAllGraphs()) {
            NJson::TJsonValue nodeInfo(NJson::EJsonValueType::JSON_MAP);
            nodeInfo["path"] = TString::Join(Name_, "/", it.NodeName);
            if (!it.ExpFlag.Empty()) {
                nodeInfo["experiment"] = TString::Join(Name_, "/", it.ExpFlag);
            }
            NJson::TJsonValue localGraphRoot(NJson::EJsonValueType::JSON_ARRAY);
            for (const auto& st : it.LocalGraphs) {
                TStringBuilder builder;
                builder << st;
                localGraphRoot.AppendValue(NJson::TJsonValue(TString{builder}));
            }
            nodeInfo["local_graph"] = localGraphRoot;
            nodesRoot.AppendValue(nodeInfo);
        }
        scenario["nodes"] = nodesRoot;
    }
    // All scenes
    NJson::TJsonValue scenesRoot(NJson::EJsonValueType::JSON_ARRAY);
    for (const auto& it : AllScenes_) {
        NJson::TJsonValue sceneInfo(NJson::EJsonValueType::JSON_MAP);
        sceneInfo["name"] = it->GetSceneName();
        sceneInfo["arguments"] = it->GetProtoClassName();
        if (!it->GetProductScenarioName().Empty()) {
            sceneInfo["psn"] = it->GetProductScenarioName();
        }
        // TODO: Add functions
        scenesRoot.AppendValue(sceneInfo);
    }
    scenario["scenes"] = scenesRoot;
    // All renders
    NJson::TJsonValue renderRoot(NJson::EJsonValueType::JSON_ARRAY);
    for (const auto& it : RendererList_) {
        NJson::TJsonValue renderInfo(NJson::EJsonValueType::JSON_MAP);
        renderInfo["path"] = it->GetScenePath();
        renderInfo["arguments"] = it->GetMsgName();
        renderRoot.AppendValue(renderInfo);
    }
    scenario["renders"] = renderRoot;
    return std::move(scenario);
}

} // namespace NAlice::NHollywoodFw
