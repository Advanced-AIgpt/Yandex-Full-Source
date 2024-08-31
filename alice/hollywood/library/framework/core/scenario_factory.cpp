//
// HOLLYWOOD FRAMEWORK
// Internal class : scenario factory
//

#include "scenario_factory.h"

#include "scene_graph.h"

#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

#include <alice/hollywood/library/dispatcher/common_handles/scenario_handles/scenario.h>
#include <alice/hollywood/library/dispatcher/common_handles/util/util.h>

#include <alice/library/json/json.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <util/generic/vector.h>

namespace NAlice::NHollywoodFw::NPrivate {

namespace {

//
// TMetricsProcessor
// Internal class to work with sensors and scenario timings
//
class TMetricsProcessor {
public:
    TMetricsProcessor(NHollywood::TGlobalContext& globalContext, const TScenario& scenario, const TString& handlerName)
        : GlobalContext_(globalContext)
        , Scenario_(scenario)
        , HandlerName_(handlerName)
        , StartTime_(TInstant::Now())
        , RequestResult_(NHollywood::ERequestResult::ERROR)
    {
        globalContext.Sensors().IncRate(ScenarioResponse(Scenario_, HandlerName_, NHollywood::ERequestResult::INCOMING));
    }

    void SetRequestResult(NHollywood::ERequestResult result) {
        const TInstant endTime = TInstant::Now();
        Duration_ = (endTime - StartTime_).MicroSeconds();
        RequestResult_ = result;
    }
    i64 GetDuration() const {
        return Duration_;
    }

    ~TMetricsProcessor() {
        GlobalContext_.Sensors().AddHistogram(ScenarioResponseTime(Scenario_, HandlerName_, RequestResult_), Duration_, NMetrics::TIME_INTERVALS);
        GlobalContext_.Sensors().AddHistogram(ScenarioResponseTime(Scenario_, HandlerName_, NHollywood::ERequestResult::TOTAL), Duration_, NMetrics::TIME_INTERVALS);
        GlobalContext_.Sensors().IncRate(ScenarioResponse(Scenario_, HandlerName_, RequestResult_));
        GlobalContext_.Sensors().IncRate(ScenarioResponse(Scenario_, HandlerName_, NHollywood::ERequestResult::TOTAL));
    }

private:
    NHollywood::TGlobalContext& GlobalContext_;
    const TScenario& Scenario_;
    const TString HandlerName_;
    const TInstant StartTime_;
    i64 Duration_ = 0;
    NHollywood::ERequestResult RequestResult_ = NHollywood::ERequestResult::ERROR;

};

} // anonimous namespace

/*
    Register scenario

    Internal function: called automatically from TScenario ctor
    Use HW_REGISTER() macro in source code to create scenario
*/
void TScenarioFactory::RegisterScenario(TScenario* sc) {
    const auto& it = AllScenarios_.find(sc->GetName());
    Y_ENSURE(it == AllScenarios_.end(), "Scenario with this name already exist in global collection");
    AllScenarios_.insert(THashMap<TString, TScenario*>::value_type({sc->GetName(), sc}));
}

/*
    Unregister scenario

    Internal function: called automatically from TScenario dtor
*/
void TScenarioFactory::UnRegisterScenario(TScenario* sc) {
    const auto it = AllScenarios_.find(sc->GetName());
    if (it != AllScenarios_.end()) {
        // Note scenario can absent in AllScenarios_ if it was already deleted with DestroyAllScenarios() call
        AllScenarios_.erase(it);
    }
}

/*
    Initialize scenario
    Internal function, called from registry
*/
void TScenarioFactory::CreateAllScenarios() {
    for (auto& it : ScenarioRegistrators_) {
        it->CreateScenario();
    }
}

void TScenarioFactory::DestroyAllScenarios() {
    const auto originalSize = AllScenarios_.size();
    for (auto& it : ScenarioRegistrators_) {
        it->DestroyScenario();
    }
    // Usually only scenarios created with CreateAllScenarios() exist in AllScenarios_ map
    // But sometimes (usually for testing purposes) some scenario handles can be created in addition to ScenarioRegistrators_
    // We will remove them from AllScenarios_ map
    const auto leftSize = AllScenarios_.size();
    AllScenarios_.clear();
    Cerr << originalSize << " framework scenario(s) were cleared, " << leftSize << " scenario(s) removed forcibly." << Endl;
}

/*
    Collect all scenarios and handler into the single list
    See THandlerInfo definition for more details
    @param [IN/OUT] allHandlers - array to store all handlers
    @param [IN] apphostNodeOnly - true if need to collect only apphost (non-empty nodes)

    Note this function is called from apphost dispatcher (hollywood server).
    At this moment we decide that scenario factory is working in 'hollywood' mode (instead of 'testing' mode)
*/
void TScenarioFactory::CollectAllScenariosHandlers(TVector<THandlerInfo>& allHandlers) {
    IsHollywoodApp_ = true;
    for (const auto& itScenarios : AllScenarios_) {
        THandlerInfo handlerInfo;
        handlerInfo.Scenario = itScenarios.second;

        // Add apphost handles
        const TScenarioGraphs* graphs = itScenarios.second->GetScenarioGraphs();
        for (const auto& it : graphs->GetAllGraphs()) {
            if (it.ExpFlag.Empty()) {
                handlerInfo.HandlerName = it.NodeName;
                allHandlers.push_back(handlerInfo);
            }
        }
    }
}

/*
    Register fast data for all scenarios who require fastdata objects
    Internal function: called from main hollywood server
*/
void TScenarioFactory::RegisterAllFastData(NHollywood::TFastData& fastData) const {
    for (const auto& it : AllScenarios_) {
        fastData.Register(it.second->FastDataInfo_);
    }
}

/*
    Find scenario by name
    Internal function, used with unittest module
*/
const TScenario* TScenarioFactory::FindScenarioName(const TString& name) const {
    const auto it = AllScenarios_.find(name);
    return it == AllScenarios_.end() ? nullptr : it->second;
}

/*
    Try to link old scenario into the new flow
    In case if both old and new scenario exists in the same server, hollywood will not register grpc for old handle
    Internal function, called from apphost dispatcher
    @return true if new scenario found (old grpc should not be registered)
    @return false if new scenario is not found, continue processing
*/
bool TScenarioFactory::LinkOldScenario(const NHollywood::TScenario& oldScenario, const TString& handleName) {
    auto it = AllScenarios_.find(oldScenario.Name());
    if (it == AllScenarios_.end()) {
        return false;
    }
    const TScenarioGraphs* sg = it->second->GetScenarioGraphs();
    if (sg && sg->FindLocalGraph(handleName, nullptr) != nullptr) {
        // Old and new scenario exists, force using new scenario flow
        it->second->OldScenarioFlow_ = &oldScenario;
        return true;
    }
    return false;
}

/*
    Dump information about all scenarios
    return json string
*/
TString TScenarioFactory::DumpScenarios(const TSet<TString>& oldScenarios, const TSet<TString>& missedScenarios) const {
    NJson::TJsonValue root(NJson::EJsonValueType::JSON_MAP);
    NJson::TJsonValue scenarios(NJson::EJsonValueType::JSON_ARRAY);
    // First add all scenarios from framework
    for (const auto& it : AllScenarios_) {
        scenarios.AppendValue(it.second->DumpScenarioInfo());
    }
    // Add old scenarios (in case if it doesn't eixt in new HWF)
    for (const auto& it : oldScenarios) {
        if (!FindScenarioName(it)) {
            NJson::TJsonValue scenario(NJson::EJsonValueType::JSON_MAP);
            scenario["name"] = it;
            scenario["type"] = "old";
            scenarios.AppendValue(scenario);
        }
    }
    // Add missed scenarios (in case if it doesn't eixt in new HWF)
    for (const auto& it : missedScenarios) {
        if (!FindScenarioName(it)) {
            NJson::TJsonValue scenario(NJson::EJsonValueType::JSON_MAP);
            scenario["name"] = it;
            scenario["type"] = "absent";
            scenarios.AppendValue(scenario);
        }
    }
    root["scenarios"] = scenarios;
    return JsonToString(root);
}

/*
    Dispatch messages coming from apphost
*/
void TScenarioFactory::DispatchScenarioHandle(const TScenario& sc,
                                              const TString& nodeName,
                                              NHollywood::TGlobalContext& globalContext,
                                              NAppHost::IServiceContext& ctx) {
    const auto appHostParams = NHollywood::GetAppHostParams(ctx);
    auto logger = NHollywood::CreateLogger(globalContext, NHollywood::GetRTLogToken(appHostParams, ctx.GetRUID()));
    NScenarios::TRequestMeta meta = NHollywood::GetMeta(ctx, logger);
    logger.UpdateRequestId(meta.GetRequestId(), ctx.GetLocation().Path);

    Y_ENSURE(IsHollywoodApp_, "This function can be used in hollywood app only!");
    const TApphostNode* localGraph = sc.GetScenarioGraphs()->FindLocalGraph(nodeName, nullptr);
    Y_ENSURE(localGraph != nullptr, "Can not find a handler for scenario " << sc.GetName() << "; node: " << nodeName);

    TMetricsProcessor metrics(globalContext, sc, nodeName);

    // WARNING(a-square)! Meta may contain secrets that must not be logged
    // TODO(a-square): consider switching to protobuf attributes to mark sensitive data
    NScenarios::TRequestMeta cleanMeta = meta;
    if (cleanMeta.GetOAuthToken()) {
        cleanMeta.SetOAuthToken("[censored]");
    }
    if (cleanMeta.GetUserTicket()) {
        cleanMeta.SetUserTicket("[censored]");
    }
    LOG_INFO(logger) << "Meta: " << SerializeProtoText(std::move(cleanMeta), /* singleLineMode = */ true, /* expandAny = */ false);

    bool dispatchResult = false;

    //
    // This catch is used to top level exceptions, when something is critically went wrong
    //
    try {
        switch (localGraph->NodeType) {
            case ENodeType::Run:
                dispatchResult = DispatchScenarioHandleRun(sc, nodeName, globalContext, ctx, meta, logger);
                break;
            case ENodeType::Continue:
            case ENodeType::Commit:
            case ENodeType::Apply:
                dispatchResult = DispatchScenarioHandleCca(sc, nodeName, globalContext, ctx, meta, logger, localGraph->NodeType);
                break;
        }
    } catch (const yexception&) {
        LOG_ERROR(logger) << FormatCurrentException();
        throw; // Put the exception to top level (note: these errors are critical errors in HW framework)
               // All scenario-specific errors will be handled below
    }
    metrics.SetRequestResult(dispatchResult ? NHollywood::ERequestResult::ERROR : NHollywood::ERequestResult::SUCCESS);
    LOG_INFO(logger) << "Scenario " << sc.GetName() << '/' << nodeName <<
                        " finished in " << metrics.GetDuration() << " us, result: " << dispatchResult;
}

/*
    Dispatch messages coming from apphost (hanlder /run)
    Internal function, called from DispatchScenarioHandle()
*/
bool TScenarioFactory::DispatchScenarioHandleRun(const TScenario& sc,
                                                 const TString& nodeName,
                                                 NHollywood::TGlobalContext& globalContext,
                                                 NAppHost::IServiceContext& ctx,
                                                 const NScenarios::TRequestMeta& meta,
                                                 TRTLogger& logger) {
    std::unique_ptr<TNodeCaller> caller;
    TMaybe<TProtoHwScene> protoScene = NHollywood::GetMaybeOnlyProto<TProtoHwScene>(ctx, HW_SCENE_ITEM);
    caller.reset(new TNodeCallerRun(sc, nodeName, globalContext, ctx, protoScene.Get(), meta, logger));

    if (!protoScene) {
        // "hw_selected_scene" node not found. This is 'OK' for DispatchSetup/Dispatch stages but not Ok for other stages
        if (caller->GetHwItemSource() == NPrivate::EHwProtoSource::None) {
            // This is ok
        } else if (caller->GetHwItemSource() == NPrivate::EHwProtoSource::HwSceneItem) {
            //
            // Possibe reasons for lost hw_selected_scene:
            // 1. You are in production release and new HWF scenario is replacing old scenario.
            // 2. You apphost graph doesn't have an edge between RUN and MAIN nodes (etc)
            //    Like:         "node_deps": {
            //                      "MY_NODE_MAIN": {
            //                          "input_deps": [
            //                              "MY_NODE_RUN" <-------------------------------- check it!
            //
            LOG_ERROR(logger) << "'hw_selected_scene' item is not found in apphost context";
            Y_ENSURE(false, "'hw_selected_scene' item is not found in apphost context. Node " << nodeName);
        } else if (caller->GetHwItemSource() == NPrivate::EHwProtoSource::Arguments) {
            // Impossible here
            Y_ENSURE(false, "'hw_selected_scene' item is not found in apphost context. Node " << nodeName);
        }
    }

    CallLocalGraph(*caller, meta.GetRandomSeed(), globalContext.Sensors());
    CheckSwitchOldFlow(*caller, globalContext);
    return !caller->Error.Defined();
}

/*
    Dispatch messages coming from apphost (hanlder /continue /commit /apply )
    Internal function, called from DispatchScenarioHandle()
*/
bool TScenarioFactory::DispatchScenarioHandleCca(const TScenario& sc,
                                                 const TString& nodeName,
                                                 NHollywood::TGlobalContext& globalContext,
                                                 NAppHost::IServiceContext& ctx,
                                                 const NScenarios::TRequestMeta& meta,
                                                 TRTLogger& logger,
                                                 ENodeType nodeType) {
    TMaybe<TProtoHwScene> protoScene;
    auto applyRequest = NHollywood::GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx, NHollywood::REQUEST_ITEM);
    const auto localGraph = sc.GetScenarioGraphs()->FindLocalGraph(nodeName, nullptr);
    Y_ENSURE(localGraph != nullptr, "Node '" << nodeName << "' not found");
    switch (localGraph->ProtoSource) {
        case EHwProtoSource::None:
            break;
        case EHwProtoSource::Arguments: {
            // Note Continue/Commit/Apply handlers share NScenarios::TScenarioApplyRequest as an incoming item
            const auto& rawState = applyRequest.GetArguments();
            NHollywoodFw::TProtoHwSceneCCAArguments frameworkArguments;
            if (rawState.Is<NHollywoodFw::TProtoHwSceneCCAArguments>() && rawState.UnpackTo(&frameworkArguments)) {
                // This request is valid and contains both framework arguments and CCA arguments
                protoScene = std::move(frameworkArguments.GetProtoHwScene());
            } else {
                LOG_WARNING(logger) << "Incoming arguments in node '" << nodeName << "' doesn't have framework arguments. "
                                    "Probably this call received from old scenario. Scenario " << sc.GetName();
                *frameworkArguments.MutableScenarioArgs() = std::move(rawState);
            }
            break;
        }
        case EHwProtoSource::HwSceneItem:
            protoScene = NHollywood::GetMaybeOnlyProto<TProtoHwScene>(ctx, HW_SCENE_ITEM);
    }
    Y_ENSURE(protoScene, "'hw_selected_scene' item is not found in apphost context or mm arguments. Node " << nodeName);

    // Create node caller for the current node type
    std::unique_ptr<TNodeCaller> caller;
    switch (nodeType) {
        case ENodeType::Continue: {
            // protoScene comes with arguments from MM
            caller.reset(new TNodeCallerContinue(sc, nodeName, applyRequest, globalContext, ctx, protoScene.Get(), meta, logger));
            break;
        }
        case ENodeType::Commit: {
            caller.reset(new TNodeCallerCommit(sc, nodeName, applyRequest, globalContext, ctx, protoScene.Get(), meta, logger));
            break;
        }
        case ENodeType::Apply: {
            caller.reset(new TNodeCallerApply(sc, nodeName, applyRequest, globalContext, ctx, protoScene.Get(), meta, logger));
            break;
        case ENodeType::Run:
            // This case is handler in DispatchScenarioHandleRun()
            Y_ENSURE(false, "Impossible");
        }
    }
    CallLocalGraph(*caller, meta.GetRandomSeed(), globalContext.Sensors());
    CheckSwitchOldFlow(*caller, globalContext);
    return !caller->Error.Defined();
}

/*
    Call all curently existing local stages in the current graph
    Usually this loop contains (for example):
        - [Dispatch, Main, Render, Finalize] (for one node graph)
        - [Dispatch, SetupMain, Bypass]
        - [Main, Render, Finalize] (for two node graph)
        - etc ...
*/
void TScenarioFactory::CallLocalGraph(TNodeCaller& caller, ui64 seed, NMetrics::ISensors& s) {
    caller.EnterLocalGraph();
    do {
        if (!caller.HasThisStage()) {
            continue;
        }
        caller.AdjustRandomSeed(caller.GetScenario().CustomNodeNames_, seed);

        try {
            switch (caller.GetCurrentStage()) {
                case EStageDeclaration::DispatchSetup:
                case EStageDeclaration::Dispatch:
                    caller.GetScenario().CallMainNode(caller, s);
                    break;
                case EStageDeclaration::SceneSetupRun:
                case EStageDeclaration::SceneMainRun:
                case EStageDeclaration::SceneSetupContinue:
                case EStageDeclaration::SceneMainContinue:
                case EStageDeclaration::SceneSetupApply:
                case EStageDeclaration::SceneMainApply:
                case EStageDeclaration::SceneSetupCommit:
                case EStageDeclaration::SceneMainCommit:
                    caller.GetScenarioScene()->CallSceneNode(caller);
                    break;
                case EStageDeclaration::Render:
                    caller.GetScenario().CallRenderNode(caller);
                    break;
                case EStageDeclaration::Bypass:
                    caller.Bypass();
                    break;
                case EStageDeclaration::Finalize:
                    caller.Finalize();
                    break;
                default:
                    caller.SetError("Undefined stage selector while trying to call node");
            }
        } catch (TFrameworkException e) {
            caller.SetErrorFromScenario(TError(TError::EErrorDefinition::HwError, e.GetFilename().Data(), e.GetLineNumber()));
            caller.Error.Details() << e.GetDiags();
            caller.Error.CollectExceptionInfo(e);
        } catch (yexception exept) {
            caller.SetErrorFromScenario(TError(TError::EErrorDefinition::Exception, __FILE__, __LINE__));
            caller.Error.CollectExceptionInfo();
        } catch (...) {
            caller.SetErrorFromScenario(TError(TError::EErrorDefinition::Exception, __FILE__, __LINE__));
            caller.Error.CollectExceptionInfo();
        }
    } while (caller.GoToNextStage());
    caller.LeaveLocalGraph();
}

/*
    Check if scenario wants to switch to old flow
    HWF scenario can return `TReturnValueDo()` to switch to old Hollywood function (if present)
    HWF scenario must be licked with old scenario (i.e. has the same scenario name and handler for the same node)
*/
void TScenarioFactory::CheckSwitchOldFlow(TNodeCaller& caller, NHollywood::TGlobalContext& globalContext) {
    if (!caller.IsSwitchToOldFlow()) {
        return;
    }

    Y_ENSURE(caller.GetScenario().OldScenarioFlow_, "Scenario '" << caller.GetScenario().GetName() <<
                "' want to switch to old scenario flow but old scenario not found");
    const TString& nodeName = caller.GetApphostNodeName();
    const auto* handle = FindIfPtr(caller.GetScenario().OldScenarioFlow_->Handles(), [nodeName](const auto& handle) {
        return handle->Name() == nodeName;
    });
    Y_ENSURE(handle, "Scenario '" << caller.GetScenario().GetName() <<
                        "' want to switch to old scenario flow but node '" << nodeName << "' not found");
    NHollywood::TScenarioNewContext newContext;
    caller.PrepareOldScenario(newContext);
    NHollywood::DispatchScenarioHandle(*caller.GetScenario().OldScenarioFlow_,
                                       *(handle->get()),
                                       globalContext,
                                       caller.GetApphostCtx(),
                                       &newContext);

    // Note if new scenario returns request to switch to old flow, Bypass/Finalize nodes will not be called
    // a few lines above. But framework must be initialized properly for next stages.
    // This call will add FrameworkProtobuf to context for future apphost graph handlers
    caller.Bypass();
}

} // namespace NAlice::NHollywoodFw::NPrivate
