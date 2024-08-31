//
// HOLLYWOOD FRAMEWORK
// Base scenario interface
//

#include "scene_base.h"
#include "scene_graph.h"

#include "node_caller.h"
#include "scenario_factory.h"

namespace NAlice::NHollywoodFw {

namespace NPrivate {

/*
    TSceneBaseCaller base ctor
    This class is a base interface to call any possible Scene functions
*/
TSceneBaseCaller::TSceneBaseCaller(const TScenario* owner, EStageDeclaration stage)
    : Owner_(owner)
    , Stage_(stage)
{
}

} // namespace NPrivate

/*
    Standard scene ctor
*/
TSceneBase::TSceneBase(const TScenario* owner, TStringBuf sceneName)
    : Owner_(owner)
    , BaseOwner_(const_cast<TScenario*>(owner))
    , SceneName_(sceneName)
{
    // Note BaseOwner_ need to remove const cast
    // because user-defined scene ctor may call non-const initial operations like RegisterRenderer, etc
}

void TSceneBase::FinishInitialization() {
    Y_ENSURE(!HandlersMap_.empty(), "At least one fuction inside scene must be inited. Scenario: '"<< Owner_->GetName() << "', Scene: " << GetSceneName());
    const auto* callerFn = FindHandlerByStage(EStageDeclaration::SceneMainRun);
    Y_ENSURE(callerFn != nullptr, "MainScene function must exist. Scenario: '"<< Owner_->GetName() << "', Scene: " << GetSceneName());
}

/*
    Save protobuf typename into Scene object and validate that all scene functions
    uses the same protobuf.
    Internal function, called automatically from TSceneBase::Register()
        Called automatically from TScenario::RegisterSceneFn()
*/
void TSceneBase::StoreTypename(const TString& t) {
    if (ProtoClassName_.Empty()) {
        ProtoClassName_ = t;
    } else {
        Y_ENSURE(ProtoClassName_ == t, "Typename for scene functions are different: "
                    << ProtoClassName_ << " vs " << t
                    << ". Scene name: " << SceneName_
                    << "; Scenario: " << Owner_->GetName());
    }
}

/*
*/
const NPrivate::TSceneBaseCaller* TSceneBase::FindHandlerByStage(EStageDeclaration stage) const {
    const auto handler = FindIfPtr(HandlersMap_, [stage](const std::unique_ptr<NPrivate::TSceneBaseCaller>& node) -> bool {
        return node->GetStage() == stage;
    });
    return (handler != nullptr) ? handler->get() : nullptr;
}

/*
    Call a node from Scene object
*/
void TSceneBase::CallSceneNode(NPrivate::TNodeCaller& callerData) const {
    const NPrivate::TSceneBaseCaller* node = FindHandlerByStage(callerData.GetCurrentStage());
    if (node == nullptr) {
        callerData.SetError(TStringBuilder{} << "Can not find a node for scene " << GetSceneName());
        return;
    }
    switch (callerData.GetCurrentStage()) {
        case EStageDeclaration::SceneSetupRun:
            CallSceneRunSetup(callerData, node);
            break;
        case EStageDeclaration::SceneMainRun:
            CallSceneRun(callerData, node);
            break;
        case EStageDeclaration::SceneSetupContinue:
            CallSceneContinueSetup(callerData, node);
            break;
        case EStageDeclaration::SceneMainContinue:
            CallSceneContinue(callerData, node);
            break;
        case EStageDeclaration::SceneSetupApply:
            CallSceneApplySetup(callerData, node);
            break;
        case EStageDeclaration::SceneMainApply:
            CallSceneApply(callerData, node);
            break;
        case EStageDeclaration::SceneSetupCommit:
            CallSceneCommitSetup(callerData, node);
            break;
        case EStageDeclaration::SceneMainCommit:
            CallSceneCommit(callerData, node);
            break;
        default:
            // Impossible here
            callerData.SetError(TStringBuilder{} << "Node for scene has invalid stage id." << GetSceneName());
    }
    return;
}

/*
    Call node EStageDeclaration::SceneSetupRun
*/
void TSceneBase::CallSceneRunSetup(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const {
    TRetSetup res = node->Call(callerData.GetSceneArgs().GetArguments(), callerData.GetRunRequest(), callerData.GetStorage());
    struct Visitor {
        NPrivate::TNodeCaller& CallerData;
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
    } visitor{callerData};
    std::visit(visitor, res);
}

/*
    Call node EStageDeclaration::SceneRun
*/
void TSceneBase::CallSceneRun(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const {
    TRetMain res = node->Call(callerData.GetSceneArgs().GetArguments(), callerData.GetRunRequest(), callerData.GetStorage(), callerData.GetSource());
    struct Visitor {
        NPrivate::TNodeCaller& CallerData;
        void operator()(const NPrivate::TRetRenderSelector& rv) {
            CallerData.RenderArgs.reset(new NPrivate::TRetRenderSelectorBase(rv));
            CallerData.DivRenderData = rv.GetDivRender();
            // Note this version of RenderArgs doesn't contains Features
            if (CallerData.IsNeedDumpRenderArgs()) {
                CallerData.DebugDump(rv.GetArguments(), "RENDER ARGS");
            }

        }
        void operator()(const NPrivate::TRetRenderSelectorFeatures& rv) {
            CallerData.RenderArgs.reset(new NPrivate::TRetRenderSelectorBase(rv));
            if (rv.GetFeatures().Defined()) {
                CallerData.RunFeatures.reset(new TRunFeatures(rv.GetFeatures()));
            }
            CallerData.DivRenderData = rv.GetDivRender();
            if (CallerData.IsNeedDumpRenderArgs()) {
                CallerData.DebugDump(rv.GetArguments(), "RENDER ARGS");
            }
        }
        void operator()(const NPrivate::TReturnValueContinueSelector& rv) {
            CallerData.CcaArguments = NPrivate::TCCAArguments{ENodeType::Continue, rv.GetArguments()};
            if (rv.GetFeatures().Defined()) {
                CallerData.RunFeatures.reset(new TRunFeatures(rv.GetFeatures()));
            }
        }
        void operator()(const NPrivate::TReturnValueApplySelector& rv) {
            CallerData.CcaArguments = NPrivate::TCCAArguments{ENodeType::Apply, rv.GetArguments()};
            if (rv.GetFeatures().Defined()) {
                CallerData.RunFeatures.reset(new TRunFeatures(rv.GetFeatures()));
            }
        }
        void operator()(const NPrivate::TRetCommitSelector& rv) {
            const auto& renderData = rv.GetRenderSelector();
            CallerData.RenderArgs.reset(new NPrivate::TRetRenderSelectorBase(renderData));
            CallerData.CcaArguments = NPrivate::TCCAArguments{ENodeType::Commit, rv.GetArguments()};
            if (renderData.GetFeatures().Defined()) {
                CallerData.RunFeatures.reset(new TRunFeatures(renderData.GetFeatures()));
            }
            CallerData.DivRenderData = renderData.GetDivRender();
            if (CallerData.IsNeedDumpRenderArgs()) {
                CallerData.DebugDump(renderData.GetArguments(), "RENDER ARGS");
            }
        }
        void operator()(const TReturnValueDo&) {
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
    Call node EStageDeclaration::SceneSetupContinue
*/
void TSceneBase::CallSceneContinueSetup(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const {
    TRetSetup res = node->Call(callerData.GetSceneArgs().GetArguments(), callerData.GetContinueRequest(), callerData.GetStorage());
    struct Visitor {
        NPrivate::TNodeCaller& CallerData;
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
    } visitor{callerData};
    std::visit(visitor, res);
}

/*
    Call node EStageDeclaration::SceneContinue
*/
void TSceneBase::CallSceneContinue(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const {
    TRetContinue res = node->Call(callerData.GetSceneArgs().GetArguments(), callerData.GetContinueRequest(), callerData.GetStorage(), callerData.GetSource());
    struct Visitor {
        NPrivate::TNodeCaller& CallerData;
        void operator()(const NPrivate::TRetRenderSelector& rv) {
            CallerData.RenderArgs.reset(new NPrivate::TRetRenderSelector(rv));
            CallerData.DivRenderData = rv.GetDivRender();
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
    } visitor{callerData};
    std::visit(visitor, res);
}

/*
    Call node EStageDeclaration::SceneSetupApply
*/
void TSceneBase::CallSceneApplySetup(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const {
    TRetSetup res = node->Call(callerData.GetSceneArgs().GetArguments(), callerData.GetApplyRequest(), callerData.GetStorage());
    struct Visitor {
        NPrivate::TNodeCaller& CallerData;
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
    } visitor{callerData};
    std::visit(visitor, res);
}

/*
    Call node EStageDeclaration::SceneApply
*/
void TSceneBase::CallSceneApply(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const {
    TRetContinue res = node->Call(callerData.GetSceneArgs().GetArguments(), callerData.GetApplyRequest(), callerData.GetStorage(), callerData.GetSource());
    struct Visitor {
        NPrivate::TNodeCaller& CallerData;
        void operator()(const NPrivate::TRetRenderSelector& rv) {
            CallerData.RenderArgs.reset(new NPrivate::TRetRenderSelector(rv));
            CallerData.DivRenderData = rv.GetDivRender();
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
    } visitor{callerData};
    std::visit(visitor, res);
}

/*
    Call node EStageDeclaration::SceneSetupCommit
*/
void TSceneBase::CallSceneCommitSetup(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const {
    TRetSetup res = node->Call(callerData.GetSceneArgs().GetArguments(), callerData.GetCommitRequest(), callerData.GetStorage());
    struct Visitor {
        NPrivate::TNodeCaller& CallerData;
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
    } visitor{callerData};
    std::visit(visitor, res);
}

/*
    Call node EStageDeclaration::SceneCommit
*/
void TSceneBase::CallSceneCommit(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const {
    TRetCommit res = node->Call(callerData.GetSceneArgs().GetArguments(), callerData.GetCommitRequest(), callerData.GetStorage(), callerData.GetSource());
    struct Visitor {
        NPrivate::TNodeCaller& CallerData;
        void operator()(const TReturnValueSuccess&) {
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
    std::visit(visitor, res);}

} // namespace NAlice::NHollywoodFw
