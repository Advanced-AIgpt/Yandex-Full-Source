//
// HOLLYWOOD FRAMEWORK
// Base scenario interface
//

#include "node_caller.h"
#include "scenario_factory.h"
#include "scene_graph.h"

#include <alice/hollywood/library/dispatcher/common_handles/util/util.h>

#include <alice/library/scenarios/data_sources/data_sources.h>
#include <alice/library/version/version.h>

#include <alice/protos/api/renderer/api.pb.h>

#include <util/generic/string.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/type.h>

namespace NAlice::NHollywoodFw::NPrivate {

namespace {

void AddObjectsToFlagsMap(TMap<TString, bool>& map, TStringBuf subval) {
    TVector<TStringBuf> splittedObj;
    Split(subval, ",", splittedObj);
    for (const auto& it : splittedObj) {
        map[TString{it}] = true;
    }
}

} // anonymous namespace

/*
    TNodeCaller ctor
*/
TNodeCaller::TNodeCaller(const TScenario& sc,
                         const TString& nodeName,
                         NAppHost::IServiceContext& ctx,
                         const TProtoHwScene* protoHwScene,
                         TRTLogger& logger)
    : Scenario_(sc)
    , Logger_(logger)
    , Ctx_(ctx)
    , ScenarioScene_(nullptr)
    , LocalGraph_(sc.GetScenarioGraphs()->FindLocalGraph(nodeName, nullptr))
    , ApphostNodeName_(nodeName)
    , CurrentStageIndex_(0)
{
    CcaArguments.ArgType = ENodeType::Run;
    if (protoHwScene != nullptr) {
        if (protoHwScene->HasSceneArgs()) {
            SceneArgs.reset(new NPrivate::TRetSceneSelector(protoHwScene->GetSceneArgs()));
            ScenarioScene_ = Scenario_.FindScene(protoHwScene->GetSceneArgs().GetSceneName());
        }
        if (protoHwScene->HasRendererArgs()) {
            RenderArgs.reset(new NPrivate::TRetRenderSelectorBase(protoHwScene->GetRendererArgs()));
        }
        if (protoHwScene->HasError()) {
            Error = TError(protoHwScene->GetError());
        }
        if (protoHwScene->GetOldFlowSelected()) {
            SetSwitchToOldFlow();
        }
        if (protoHwScene->HasRunFeatures()) {
            // Import data from run features
            RunFeatures.reset(new TRunFeatures(protoHwScene->GetRunFeatures()));
        }
        if (protoHwScene->HasCcaArguments()) {
            const auto& args = protoHwScene->GetCcaArguments();
            if (args.GetArgumentType() == TProtoHwScene_TCcaArguments_ECcaType_Commit) {
                CcaArguments.ArgType = ENodeType::Commit;
                CcaArguments.Args.CopyFrom(args.GetScenarioArgs());
            } else if (args.GetArgumentType() == TProtoHwScene_TCcaArguments_ECcaType_Apply) {
                CcaArguments.ArgType = ENodeType::Apply;
                CcaArguments.Args.CopyFrom(args.GetScenarioArgs());
            } else if (args.GetArgumentType() == TProtoHwScene_TCcaArguments_ECcaType_Continue) {
                CcaArguments.ArgType = ENodeType::Continue;
                CcaArguments.Args.CopyFrom(args.GetScenarioArgs());
            }
        }
    }
}

void TNodeCaller::SetSwitchToOldFlow() {
    NeedSwitchToOldFlow_ = true;
    if (LocalGraphExecutionFlag_ && Scenario_.IsDebugLocalGraph()) {
        LOG_DEBUG(Logger_) << "Node caller has incoming request to switch to old scenario flow";
    }
}


/*
    Initialize additional input parameters from source context
*/
void TNodeCaller::SetInputParameters(const TProtoHwScene* protoScene) {
    const google::protobuf::Any& scenarioState = GetBaseRequestProto().GetState();
    if (scenarioState.Is<TProtoHwFramework>()) {
        if (!scenarioState.UnpackTo(&FrameworkState_)) {
            // Can not extract data to frameworkState, use defaults
            // Note this is a normal situation for the first requests after long absence
            if (GetCurrentStage() != EStageDeclaration::DispatchSetup &&
                GetCurrentStage() != EStageDeclaration::Dispatch) {
                LOG_WARNING(Logger_) << "BaseRequest::State doesn't contains Framework object! use defaults";
            }
        }
    } else if (!scenarioState.type_url().Empty()) {
        // Current baseRequestProto->GetState() contains unknown data (possible old scenario state)
        // Move it inside frameworkState
        FrameworkState_.MutableScenarioState()->CopyFrom(scenarioState);
    }

    Storage_.reset(new TStorage(GetBaseRequestProto(), FrameworkState_, protoScene, Logger_));
    Source_.reset(new TSource(Ctx_, GetBaseRequest()));

    // Extract protobufs to source
    if (protoScene != nullptr) {
        for (const auto& it : protoScene->GetNetworkRequests()) {
            const NAppHost::NService::TProtobufItem* item = nullptr;
            const NAppHost::TContextProtobufItemRefArray& items = Ctx_.GetProtobufItemRefs(it.GetIncomingName());
            if (items.size() == 1) {
                item = &items.front();
            }
            Source_->AddSourceResponse(it.GetOutgoingName(), it.GetIncomingName(), item);
        }
        if (protoScene->GetOldFlowSelected()) {
            SetSwitchToOldFlow();
        }
    }

    // Analyze hwf_debug_dump_xxx experiments
    GetBaseRequest().Flags().ForEachSubval([this](const TString& key, const TString& subval, const TMaybe<TString>& value) -> bool {
        // Check that subval contains actual scenario name
        if (!subval.Contains(Scenario_.GetName())) {
            return true;
        }
        if (key.StartsWith("hwf_debug_dump_") && (!value || IsTrue(*value))) {
            if (key == "hwf_debug_dump_run_request") {
                HwfDebugDumpRunRequest_ = true;
            }
            if (key == "hwf_debug_dump_run_response") {
                HwfDebugDumpRunResponse_ = true;
            }
            if (key == "hwf_debug_dump_apply_request") {
                HwfDebugDumpApplyRequest_ = true;
            }
            if (key == "hwf_debug_dump_apply_response") {
                HwfDebugDumpApplyResponse_ = true;
            }
            if (key == "hwf_debug_dump_continue_request") {
                HwfDebugDumpContinueRequest_ = true;
            }
            if (key == "hwf_debug_dump_continue_response") {
                HwfDebugDumpContinueResponse_ = true;
            }
            if (key == "hwf_debug_dump_commit_request") {
                HwfDebugDumpCommitRequest_ = true;
            }
            if (key == "hwf_debug_dump_commit_response") {
                HwfDebugDumpCommitResponse_ = true;
            }
            if (key == "hwf_debug_dump_render_data") {
                AddObjectsToFlagsMap(HwfDebugDumpRenderData_, subval);
            }
            if (key == "hwf_debug_dump_scene_arguments") {
                AddObjectsToFlagsMap(HwfDebugDumpSceneArguments_, subval);
            }
            if (key == "hwf_debug_dump_render_arguments") {
                AddObjectsToFlagsMap(HwfDebugDumpRenderArguments_, subval);
            }
            if (key == "hwf_debug_dump_continue_arguments") {
                AddObjectsToFlagsMap(HwfDebugDumpContinueArguments_, subval);
            }
            if (key == "hwf_debug_dump_commit_arguments") {
                AddObjectsToFlagsMap(HwfDebugDumpCommitArguments_, subval);
            }
            if (key == "hwf_debug_dump_apply_arguments") {
                AddObjectsToFlagsMap(HwfDebugDumpApplyArguments_, subval);
            }
            if (key == "hwf_debug_dump_datasource") {
                AddObjectsToFlagsMap(HwfDebugDumpDatasource_, subval);
            }
            if (key == "hwf_debug_dump_local_graph") {
                HwfDebugDumpLocalGraph_ = true;
            }
            if (key == "hwf_debug_dump_format") {
                if (subval == "json") {
                    HwfDebugDumpAsJson_ = true;
                } else if (subval == "proto") {
                    HwfDebugDumpAsJson_ = false;
                }
            }
        }
        if (!HwfDebugDumpLocalGraph_) {
            HwfDebugDumpLocalGraph_ = Scenario_.IsDebugLocalGraph();
        }
        return true;
    });
}

/*
    Re-initialize random seed using multihash
    Framework re-initializes random seed for each local node
    It use either standard or custom-scenario node names to keep compatibility with old scenarios
*/
void TNodeCaller::AdjustRandomSeed(const TMap<EStageDeclaration, TString>& customNodeNames, ui64 seed) {
    // Check a custom node name in scenario data
    TStringBuf stageNameHash;
    const auto& it = customNodeNames.find(GetCurrentStage());
    if (it != customNodeNames.end()) {
        stageNameHash = it->second;
    } else {
        // Custom name not found, using standard data
        switch (GetCurrentStage()) {
            case EStageDeclaration::DispatchSetup:
                stageNameHash = "dispatchsetup";
                break;
            case EStageDeclaration::Dispatch:
                stageNameHash = "dispatch";
                break;
            case EStageDeclaration::SceneSetupRun:
                stageNameHash = "setupmain";
                break;
            case EStageDeclaration::SceneSetupContinue:
                stageNameHash = "setupcontinue";
                break;
            case EStageDeclaration::SceneSetupApply:
                stageNameHash = "setupapply";
                break;
            case EStageDeclaration::SceneSetupCommit:
                stageNameHash = "setupcommit";
                break;
            case EStageDeclaration::SceneMainRun:
                stageNameHash = "run"; // To keep compatibility with simple onenode graph
                break;
            case EStageDeclaration::SceneMainContinue:
                stageNameHash = "maincontinue";
                break;
            case EStageDeclaration::SceneMainApply:
                stageNameHash = "mainapply";
                break;
            case EStageDeclaration::SceneMainCommit:
                stageNameHash = "maincommit";
                break;
            case EStageDeclaration::Render:
                stageNameHash = "render";
                break;
            case EStageDeclaration::Bypass:
            case EStageDeclaration::Finalize:
            case EStageDeclaration::Undefined:
                stageNameHash = "";
                break;
        }
    }
    if (!stageNameHash.Empty()) {
        GetBaseRequest().System().Random() = TRng{MultiHash(seed, Scenario_.GetName(), stageNameHash)};
    }
}

/*
    Enter to local graph loop
    Aux function used for debugging purpose only
*/
void TNodeCaller::EnterLocalGraph() {
    if (Scenario_.IsDebugLocalGraph()) {
        LOG_DEBUG(Logger_) << "Scenario '" << Scenario_.GetName() << "' activated from apphost node '/" << ApphostNodeName_ << '\'';
        if (LocalGraph_ == nullptr) {
            LOG_ERROR(Logger_) << "Apphost graph is not selected!" << Endl;
        } else {
            LOG_DEBUG(Logger_) << "Selected apphost graph: '" << LocalGraph_->NodeName << "', exp: " << LocalGraph_->ExpFlag;
        }
    }
    LocalGraphExecutionFlag_ = true;
}

/*
    Leave local graph loop
    Aux function used for debugging purpose only
*/
void TNodeCaller::LeaveLocalGraph() {
    if (Scenario_.IsDebugLocalGraph()) {
        LOG_DEBUG(Logger_) << "Exiting from local graph with result: " << (Error.Defined() ? "Failure" : "Success");
    }
    LocalGraphExecutionFlag_ = false;
}

/*
    Return true if the current scenario has this node and should be called
    This function is virtual and can be overrided for testing purpose (limited calls for local graph)
*/
bool TNodeCaller::HasThisStage() const {
    // Dump initial graph information if need
    if (LocalGraphExecutionFlag_ && Scenario_.IsDebugLocalGraph()) {
        const TSceneBase* scn = GetScenarioScene();
        LOG_DEBUG(Logger_) << "Preparing to call a node '" << GetCurrentStage() << "'; Scene: " <<
            (scn ? scn->GetSceneName() : "N/A");
    }
    bool hasThisStage = false;
    const EStageDeclaration stage = GetCurrentStage();

    switch (TScenarioGraphs::GetStageType(stage)) {
        case TScenarioGraphs::EStageType::ScenarioDispatch:
            // Always present
            hasThisStage = true;
            break;
        case TScenarioGraphs::EStageType::ScenarioSetup:
            // Scenario stage EStageDeclaration::DispatchSetup may absent
            hasThisStage = (Scenario_.FindScenarioNodeByStage(stage) != nullptr);
            break;
        case TScenarioGraphs::EStageType::SceneSetup:
            if (ScenarioScene_ == nullptr || !SceneArgs) {
                // Scene pointer must be set before this call
                hasThisStage = false;
            } else {
                hasThisStage = (ScenarioScene_->FindHandlerByStage(stage) != nullptr);
            }
            break;
        case TScenarioGraphs::EStageType::SceneMain:
            // Always present, but scene pointer must be set before this call
            hasThisStage = (ScenarioScene_ && SceneArgs) ? true : false;
            break;
        case TScenarioGraphs::EStageType::Renderer:
            hasThisStage = RenderArgs ? true : false;
            break;
        case TScenarioGraphs::EStageType::Internal:
            // Internal stages (i.e. Bypass, Finalize) always exist in the scenario
            hasThisStage = true;
            break;
        case TScenarioGraphs::EStageType::Unknown:
            break;
    }

    // Dump final debug graph information if scenario should not pass to this stage
    if (LocalGraphExecutionFlag_ && Scenario_.IsDebugLocalGraph()) {
        if (!hasThisStage) {
            LOG_DEBUG(Logger_) << "Node '" << stage << "' skipped because it's absent in local graph declaration";
        }
    }

    if (NeedSwitchToOldFlow_) {
        if (LocalGraphExecutionFlag_ && Scenario_.IsDebugLocalGraph()) {
            LOG_DEBUG(Logger_) << "Node '" << stage << "' skipped because old flow flag set";
        }
        return false;
    }
    if (Error.Defined()) {
        // User-defined nodes should not be called if error occured
        if (LocalGraphExecutionFlag_ && Scenario_.IsDebugLocalGraph()) {
            LOG_DEBUG(Logger_) << "Node '" << stage << "' skipped because an error occured in previous call";
        }
        if (TScenarioGraphs::GetStageType(stage) != TScenarioGraphs::EStageType::Internal) {
            return false;
        }
        // Bypass & Finalize nodes must be called anyway
    }
    return hasThisStage;
}

/*
    Check and advance CurrentStageIndex_ in the current scenario graph
*/
bool TNodeCaller::GoToNextStage() {
    Y_ENSURE(LocalGraph_, "NodeCaller did not initialized correctly");
    if (NeedSwitchToOldFlow_) {
        return false;
    }
    const EStageDeclaration stage = GetCurrentStage();
    switch (TScenarioGraphs::GetStageType(stage)) {
        case TScenarioGraphs::EStageType::Internal:
        case TScenarioGraphs::EStageType::Unknown:
            return false;

        case TScenarioGraphs::EStageType::ScenarioDispatch:
        case TScenarioGraphs::EStageType::ScenarioSetup:
        case TScenarioGraphs::EStageType::SceneSetup:
        case TScenarioGraphs::EStageType::SceneMain:
        case TScenarioGraphs::EStageType::Renderer:
            CurrentStageIndex_++;
            return true;
    }
}

/*
    Convert data to intermediate HW_SCENE_ITEM
*/
void TNodeCaller::Bypass() {
    TProtoHwScene sceneResults = ExportToProto(true, true);
    if (SetupRequests) {
        SetupRequests->MergeToCtx(Ctx_, sceneResults);
    }

    // Attach div render data if need
    for (const auto& it : DivRenderData) {
        AddProtobufItem(*it, NAlice::NHollywood::RENDER_DATA_ITEM);
    }

    // Attach setup item to protobuf
    if (ScenarioScene_ != nullptr)  {
        //
        // Add context flag
        // Context flag has format hwf#scenario_name#scene_name (always lowercase)
        // You may use this flag to setup apphost edges conditions
        //
        TString flagName = Join('#', "hwf", Scenario_.GetName(), ScenarioScene_->GetSceneName());
        flagName.to_lower();
        Ctx_.AddFlag(flagName);
    }
    AddProtobufItem(sceneResults, HW_SCENE_ITEM);
}

/*
    Prepare data for calling old scenario (TReturnValueDo())
    This function is virtual and can be overrided to save Run/Continue/... requests
*/
void TNodeCaller::PrepareOldScenario(NHollywood::TScenarioNewContext& oldFlowContext) const {
    oldFlowContext.SourceData = Source_ ? Source_.get() : nullptr;
    oldFlowContext.RenderData = Render ? Render.get() : nullptr;
    oldFlowContext.SceneArguments = SceneArgs ? &SceneArgs->GetArguments() : nullptr;
    oldFlowContext.RenderArguments = RenderArgs ? &RenderArgs->GetArguments() : nullptr;
    oldFlowContext.NodeCaller = this;
}

/*
    Internal function to convert data to apphost protobuf object
    @param needSaveRenderArgs -
    @param needSaveCommitContinueApplyArgs -
*/
TProtoHwScene TNodeCaller::ExportToProto(bool needSaveRenderArgs, bool needSaveCommitContinueApplyArgs) const {
    TProtoHwScene sceneResults;
    if (SceneArgs) {
        SceneArgs->ToProto(*sceneResults.MutableSceneArgs());
    }
    if (needSaveRenderArgs && RenderArgs) {
        RenderArgs->ToProto(*sceneResults.MutableRendererArgs());
    }
    if (Storage_) {
        Storage_->ToProto(sceneResults);
    }

    if (Error.Defined()) {
        Error.ExportToProto(sceneResults.MutableError());
    }
    sceneResults.SetOldFlowSelected(NeedSwitchToOldFlow_);
    GetBaseRequest().ExportToProto(sceneResults);
    if (RunFeatures) {
        RunFeatures->ExportToProto(sceneResults);
    }
    if (needSaveCommitContinueApplyArgs) {
        switch (CcaArguments.ArgType) {
            case ENodeType::Run:
                // Don't need to export Commit/Contiue/Apply arguments too
                break;
            case ENodeType::Commit:
                sceneResults.MutableCcaArguments()->SetArgumentType(TProtoHwScene_TCcaArguments_ECcaType_Commit);
                break;
            case ENodeType::Apply:
                sceneResults.MutableCcaArguments()->SetArgumentType(TProtoHwScene_TCcaArguments_ECcaType_Apply);
                break;
            case ENodeType::Continue:
                sceneResults.MutableCcaArguments()->SetArgumentType(TProtoHwScene_TCcaArguments_ECcaType_Continue);
                break;
        }
        if (CcaArguments.ArgType != ENodeType::Run) {
            sceneResults.MutableCcaArguments()->MutableScenarioArgs()->CopyFrom(CcaArguments.Args);
        }
    }
    return sceneResults;
}

/*
    Internal helper function to generate an error
*/
void TNodeCaller::SetError(const TString& msg) {
    Error = TError(TError::EErrorDefinition::ExternalError, "", 0);
    Error.Details() << msg << ". Scenario '" << Scenario_.GetName() << "'. Current stage: " << GetCurrentStage();
}

void TNodeCaller::SetErrorFromScenario(const TError& err) {
    Error = err;
    Error.SetNodeStageInfo(ApphostNodeName_, ToString(GetCurrentStage()));
}

/*
    Check: is need to dump detailed information
    Dump flags has format hwf_debug_dump_xxx=scenario_name[,scene_name1,scene_name2,...]
*/
bool TNodeCaller::IsNeedDump(const TMap<TString, bool>& map) const {
    if (!map.contains(Scenario_.GetName())) {
        // No such scenario in debug flags
        return false;
    }
    if (map.size() == 1) {
        // Only scenario name set (flag is active for all scenes)
        return true;
    }
    if (ScenarioScene_ == nullptr) {
        // No active scene, nothing to dump
        return false;
    }
    // Dump, if current scene exists in global map
    return map.contains(ScenarioScene_->GetSceneName());
}

/*
*/
void TNodeCaller::PrepareBaseResponseData() {
    NScenarios::TScenarioResponseBody* baseResponse = GetBaseResponseProto();
    if (Storage_) {
        Storage_->BuildAnswer(baseResponse, FrameworkState_);
    }
    if (Render) {
        Render->BuildAnswer(baseResponse, Ctx_, *this);
    }

    //
    // Add analytics info
    //
    if (baseResponse) {
        TAICustomData aiData;
        GetBaseRequest().AI().BuildAnswer(baseResponse, aiData);

        if (aiData.Psn.Empty() && ScenarioScene_ != nullptr) {
            // Use custom psn from scene
            aiData.Psn = ScenarioScene_->GetProductScenarioName();
        }
        if (aiData.Psn.Empty()) {
            // Use default psn from scenario
            aiData.Psn = Scenario_.GetProductScenarioName();
        }
        if (aiData.Psn.Empty()) {
            aiData.Psn = Scenario_.GetName();
        }
        baseResponse->MutableAnalyticsInfo()->SetProductScenarioName(aiData.Psn);

        if (aiData.Intent.Empty() && SceneArgs != nullptr) {
            aiData.Intent = SceneArgs->GetSelectedIntent();
        }
        baseResponse->MutableAnalyticsInfo()->SetIntent(aiData.Intent);

        if (aiData.OutputFrameName.Empty()) {
            aiData.OutputFrameName = aiData.Intent;
        }
        if (GetInputProto() != nullptr && !aiData.OutputFrameName.empty()) {
            // Copy Semantic frame back to response
            const auto& sf = GetInputProto()->GetSemanticFrames();
            for (const auto& frame : sf) {
                if (frame.GetName() == aiData.OutputFrameName) {
                    *(baseResponse->MutableSemanticFrame()) = frame;
                    break;
                }
            }
        }
    }

    // Attach div render data
    if (Scenario_.IsPrepareDivRenderOnFinalize()) {
        for (const auto& it : DivRenderData) {
            AddProtobufItem(*it, NAlice::NHollywood::RENDER_DATA_ITEM);
            if (HwfDebugDumpRenderData_) {
                DebugDump(*it, "RENDER DATA");
            }
        }
    }
}

/*
*/
TString TNodeCaller::GetLastError() {
    if (!Error.Defined()) {
        return TString{};
    }

    const TString result = Error.Print(Scenario_);
    if (ErrorReportingFlag_) {
        LOG_ERROR(Logger_) << result;
    }
    return result;
}

/*
    TNodeCaller extensions - for Run/Continue/Commit/Apply events
*/
TNodeCallerRun::TNodeCallerRun(const TScenario& sc,
                               const TString& nodeName,
                               NHollywood::IGlobalContext& globalContext,
                               NAppHost::IServiceContext& ctx,
                               const TProtoHwScene* protoHwScene,
                               const NScenarios::TRequestMeta& meta,
                               TRTLogger& logger)
    : TNodeCaller(sc, nodeName, ctx, protoHwScene, logger)
    , NodeInfo_({sc.GetName(), nodeName, NHollywood::GetAppHostParams(ctx), ENodeType::Run})
    , RunRequestProto_(NHollywood::GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx, NHollywood::REQUEST_ITEM))
    , RunRequest_(meta, RunRequestProto_, ctx, protoHwScene, globalContext, logger, NodeInfo_, sc.GetNlg())
    , Meta_(meta)
{
    NHollywood::TScenarioNewContext newContext;
    PrepareOldScenario(newContext);
    TScenarioBase::THookInputInfo info(ENodeType::Run, ctx, meta, protoHwScene, &newContext);
    sc.Hook(info, RunRequestProto_);

    // Finalize base class initialization
    SetInputParameters(protoHwScene);
    RunResponseProto_.SetVersion(VERSION_STRING);
    // Override local graph with possible exp
    LocalGraph_ = sc.GetScenarioGraphs()->FindLocalGraph(GetApphostNodeName(), &RunRequest_.Flags());

    // Dump sources if need
    if (HwfDebugDumpRunRequest_) {
        DebugDump(RunRequestProto_, "RUN REQUEST");
    }
    for (const auto& it : HwfDebugDumpDatasource_) {
        if (!it.second) {
            continue;
        }
        EDataSourceType type = NScenarios::GetDataSourceContextType(it.first);
        if (type == EDataSourceType::UNDEFINED_DATA_SOURCE) {
            LOG_WARNING(logger) << "Undefined tag: " << it.first;
        } else if (const auto* ds = RunRequest_.GetDataSource(type, false)) {
            DebugDump(*ds, TStringBuilder{} << "DATASOURCE (" << it.first << ")");
        } else {
            LOG_INFO(logger) << "DATASOURCE (" << it.first << "): null";
        }
    }
}

void TNodeCallerRun::PrepareOldScenario(NHollywood::TScenarioNewContext& oldFlowContext) const {
    TNodeCaller::PrepareOldScenario(oldFlowContext);
    oldFlowContext.RunRequest = &RunRequest_;
}

/*
    Finalize run request to scenario and prepare answer to MM
*/
void TNodeCallerRun::Finalize() {
    //
    // Check error messages
    //
    const TString err = GetLastError();
    if (err) {
        RunResponseProto_.MutableError()->SetMessage(err);
    } else {
        //
        // Fill TFeatures/TUserInfo
        //
        if (RunFeatures != nullptr) {
            RunFeatures->ExportToResponse(RunResponseProto_);
        }
        if (RenderArgs && RenderArgs->IsIrrelevant()) {
            RunResponseProto_.MutableFeatures()->SetIsIrrelevant(true);
        }

        // Check what result we have to return from this node
        switch (CcaArguments.ArgType) {
            case ENodeType::Run: {
                // Arguments not set or default run processing (with render)
                // Will return ResponseBody
                PrepareBaseResponseData();
                break;
            }
            case ENodeType::Continue: {
                // Will return ContinueArguments
                TProtoHwScene sceneResults = ExportToProto(false, false);

                TProtoHwSceneCCAArguments frameworkArguments;
                *frameworkArguments.MutableScenarioArgs() = CcaArguments.Args;
                *frameworkArguments.MutableProtoHwScene() = std::move(sceneResults);
                if (Storage_->HasChanges()) {
                    LOG_WARNING(Logger_) << "Your storage has changes but /continue hanlder was selected. Changes wil be lost";
                }
                RunResponseProto_.MutableContinueArguments()->PackFrom(frameworkArguments);
                if (IsNeedDump(HwfDebugDumpContinueArguments_)) {
                    DebugDump(CcaArguments.Args, "CONTINUE ARGS");
                }
                break;
            }
            case ENodeType::Apply: {
                // Will return ApplyArguments
                TProtoHwScene sceneResults = ExportToProto(false, false);

                TProtoHwSceneCCAArguments frameworkArguments;
                *frameworkArguments.MutableScenarioArgs() = CcaArguments.Args;
                *frameworkArguments.MutableProtoHwScene() = std::move(sceneResults);
                if (Storage_->HasChanges()) {
                    LOG_WARNING(Logger_) << "Your storage has changes but /apply hanlder was selected. Changes wil be lost";
                }
                RunResponseProto_.MutableApplyArguments()->PackFrom(frameworkArguments);
                if (IsNeedDump(HwfDebugDumpApplyArguments_)) {
                    DebugDump(CcaArguments.Args, "APPLY ARGS");
                }
                break;
            }
            case ENodeType::Commit: {
                // Will return CommitCandidate (i.e. TCommitCandidate::ResponseBody + TCommitCandidate::Arguments)
                TProtoHwScene sceneResults = ExportToProto(false, false);

                NScenarios::TScenarioRunResponse::TCommitCandidate commitCandidate;
                TProtoHwSceneCCAArguments frameworkArguments;
                *frameworkArguments.MutableScenarioArgs() = CcaArguments.Args;
                *frameworkArguments.MutableProtoHwScene() = std::move(sceneResults);
                PrepareBaseResponseData();

                // Move collected data from RunResponseProto because it will be replaced with CommitCandidate
                *commitCandidate.MutableResponseBody() = std::move(*RunResponseProto_.MutableResponseBody());
                commitCandidate.MutableArguments()->PackFrom(frameworkArguments);
                *RunResponseProto_.MutableCommitCandidate() = std::move(commitCandidate);

                if (IsNeedDump(HwfDebugDumpCommitArguments_)) {
                    DebugDump(CcaArguments.Args, "COMMIT ARGS");
                }
                break;
            }
        }
    }

    // Call hook function
    TProtoHwScene sceneResults = ExportToProto(true, true);
    NHollywood::TScenarioNewContext newContext;
    PrepareOldScenario(newContext);
    TScenarioBase::THookInputInfo info(ENodeType::Run, Ctx_, Meta_, &sceneResults, &newContext);
    GetScenario().Hook(info, RunResponseProto_);

    // Attach main body data collected from base node caller
    if (HwfDebugDumpRunResponse_) {
        DebugDump(RunResponseProto_, "RUN RESPONSE");
    }
    AddProtobufItem(RunResponseProto_, NHollywood::RESPONSE_ITEM);
}

TNodeCallerContinue::TNodeCallerContinue(const TScenario& sc,
                                         const TString& nodeName,
                                         NScenarios::TScenarioApplyRequest& continueRequest,
                                         NHollywood::IGlobalContext& globalContext,
                                         NAppHost::IServiceContext& ctx,
                                         const TProtoHwScene* protoHwScene,
                                         const NScenarios::TRequestMeta& meta,
                                         TRTLogger& logger)
    : TNodeCaller(sc, nodeName, ctx, protoHwScene, logger)
    , NodeInfo_({sc.GetName(), nodeName, NHollywood::GetAppHostParams(ctx), ENodeType::Continue})
    , ContinueRequestProto_(continueRequest)
    , ContinueRequest_(meta, continueRequest, ctx, protoHwScene, globalContext, logger, NodeInfo_, sc.GetNlg())
    , Meta_(meta)
{
    NHollywood::TScenarioNewContext newContext;
    PrepareOldScenario(newContext);
    TScenarioBase::THookInputInfo info(ENodeType::Continue, ctx, meta, protoHwScene, &newContext);
    sc.Hook(info, ContinueRequestProto_);

    // Finalize base class iniialization
    SetInputParameters(protoHwScene);
    ContinueResponseProto_.SetVersion(VERSION_STRING);
    // Override local graph with possible exp
    LocalGraph_ = sc.GetScenarioGraphs()->FindLocalGraph(GetApphostNodeName(), &ContinueRequest_.Flags());

    if (HwfDebugDumpContinueRequest_) {
        DebugDump(ContinueRequestProto_, "CONTINUE REQUEST");
    }
}

void TNodeCallerContinue::PrepareOldScenario(NHollywood::TScenarioNewContext& oldFlowContext) const {
    TNodeCaller::PrepareOldScenario(oldFlowContext);
    oldFlowContext.ContinueRequest = &ContinueRequest_;
}

void TNodeCallerContinue::Finalize() {
    // Attach main body data collected from base node caller
    PrepareBaseResponseData();

    const TString err = GetLastError();
    if (err) {
        ContinueResponseProto_.MutableError()->SetMessage(err);
    } else {
        if (RenderArgs && RenderArgs->IsIrrelevant()) {
            // Generally its impossible here
            LOG_WARNING(Logger_) << "Continue() handler set Irrelevant feature but this flag is not supported in continue stage";
        }

        // Call hook function
        TProtoHwScene sceneResults = ExportToProto(true, true);
        NHollywood::TScenarioNewContext newContext;
        PrepareOldScenario(newContext);
        TScenarioBase::THookInputInfo info(ENodeType::Continue, Ctx_, Meta_, &sceneResults, &newContext);
        GetScenario().Hook(info, ContinueResponseProto_);
    }

    if (HwfDebugDumpContinueResponse_) {
        DebugDump(ContinueResponseProto_, "CONTINUE RESPONSE");
    }
    AddProtobufItem(ContinueResponseProto_, NHollywood::RESPONSE_ITEM);
}

TNodeCallerApply::TNodeCallerApply(const TScenario& sc,
                                   const TString& nodeName,
                                   NScenarios::TScenarioApplyRequest& applyRequest,
                                   NHollywood::IGlobalContext& globalContext,
                                   NAppHost::IServiceContext& ctx,
                                   const TProtoHwScene* protoHwScene,
                                   const NScenarios::TRequestMeta& meta,
                                   TRTLogger& logger)
    : TNodeCaller(sc, nodeName, ctx, protoHwScene, logger)
    , NodeInfo_({sc.GetName(), nodeName, NHollywood::GetAppHostParams(ctx), ENodeType::Apply})
    , ApplyRequestProto_(applyRequest)
    , ApplyRequest_(meta, applyRequest, ctx, protoHwScene, globalContext, logger, NodeInfo_, sc.GetNlg())
    , Meta_(meta)
{
    NHollywood::TScenarioNewContext newContext;
    PrepareOldScenario(newContext);
    TScenarioBase::THookInputInfo info(ENodeType::Apply, ctx, meta, protoHwScene, &newContext);
    sc.Hook(info, ApplyRequestProto_);

    // Finalize base class iniialization
    SetInputParameters(protoHwScene);
    ApplyResponseProto_.SetVersion(VERSION_STRING);
    // Override local graph with possible exp
    LocalGraph_ = sc.GetScenarioGraphs()->FindLocalGraph(GetApphostNodeName(), &ApplyRequest_.Flags());

    if (HwfDebugDumpApplyRequest_) {
        DebugDump(ApplyRequestProto_, "APPLY REQUEST");
    }
}

void TNodeCallerApply::PrepareOldScenario(NHollywood::TScenarioNewContext& oldFlowContext) const {
    TNodeCaller::PrepareOldScenario(oldFlowContext);
    oldFlowContext.ApplyRequest = &ApplyRequest_;
}

void TNodeCallerApply::Finalize() {
    // Attach main body data collected from base node caller
    PrepareBaseResponseData();

    const TString err = GetLastError();
    if (err) {
        ApplyResponseProto_.MutableError()->SetMessage(err);
    } else {
        if (RenderArgs && RenderArgs->IsIrrelevant()) {
            // Generally its impossible here
            LOG_WARNING(Logger_) << "Apply() handler set Irrelevant feature but this flag is not supported in apply stage";
        }

        // Call hook function
        TProtoHwScene sceneResults = ExportToProto(true, true);
        NHollywood::TScenarioNewContext newContext;
        PrepareOldScenario(newContext);
        TScenarioBase::THookInputInfo info(ENodeType::Apply, Ctx_, Meta_, &sceneResults, &newContext);
        GetScenario().Hook(info, ApplyResponseProto_);
    }

    if (HwfDebugDumpApplyResponse_) {
        DebugDump(ApplyResponseProto_, "APPLY RESPONSE");
    }
    AddProtobufItem(ApplyResponseProto_, NHollywood::RESPONSE_ITEM);
}

TNodeCallerCommit::TNodeCallerCommit(const TScenario& sc,
                                     const TString& nodeName,
                                     NScenarios::TScenarioApplyRequest& commitRequest,
                                     NHollywood::IGlobalContext& globalContext,
                                     NAppHost::IServiceContext& ctx,
                                     const TProtoHwScene* protoHwScene,
                                     const NScenarios::TRequestMeta& meta,
                                     TRTLogger& logger)
    : TNodeCaller(sc, nodeName, ctx, protoHwScene, logger)
    , NodeInfo_({sc.GetName(), nodeName, NHollywood::GetAppHostParams(ctx), ENodeType::Commit})
    , CommitRequestProto_(commitRequest)
    , CommitRequest_(meta, commitRequest, ctx, protoHwScene, globalContext, logger, NodeInfo_, sc.GetNlg())
    , Meta_(meta)
{
    NHollywood::TScenarioNewContext newContext;
    PrepareOldScenario(newContext);
    TScenarioBase::THookInputInfo info(ENodeType::Commit, ctx, meta, protoHwScene, &newContext);
    sc.Hook(info, CommitRequestProto_);

    // Finalize base class iniialization
    SetInputParameters(protoHwScene);
    CommitResponseProto_.SetVersion(VERSION_STRING);
    // Override local graph with possible exp
    LocalGraph_ = sc.GetScenarioGraphs()->FindLocalGraph(GetApphostNodeName(), &CommitRequest_.Flags());

    if (HwfDebugDumpCommitRequest_) {
        DebugDump(CommitRequestProto_, "COMMIT REQUEST");
    }
}

void TNodeCallerCommit::PrepareOldScenario(NHollywood::TScenarioNewContext& oldFlowContext) const {
    TNodeCaller::PrepareOldScenario(oldFlowContext);
    oldFlowContext.CommitRequest = &CommitRequest_;
}

void TNodeCallerCommit::Finalize() {
    // Attach main body data collected from base node caller
    PrepareBaseResponseData();

    const TString err = GetLastError();
    if (err) {
        CommitResponseProto_.MutableError()->SetMessage(err);
    } else {
        if (RenderArgs && RenderArgs->IsIrrelevant()) {
            // Generally its impossible here
            LOG_WARNING(Logger_) << "Commit() handler set Irrelevant feature but this flag is not supported in commit stage";
        }
        *CommitResponseProto_.MutableSuccess() = NScenarios::TScenarioCommitResponse::TSuccess{};

        if (Scenario_.IsPrepareDivRenderOnFinalize()) {
            if (!DivRenderData.empty()) {
                LOG_WARN(Logger_) << "Can't handle DIV render data on commit stage";
            }
        }

        // Call hook function
        TProtoHwScene sceneResults = ExportToProto(true, true);
        NHollywood::TScenarioNewContext newContext;
        PrepareOldScenario(newContext);
        TScenarioBase::THookInputInfo info(ENodeType::Commit, Ctx_, Meta_, &sceneResults, &newContext);
        GetScenario().Hook(info, CommitResponseProto_);
    }

    if (HwfDebugDumpCommitResponse_) {
        DebugDump(CommitResponseProto_, "COMMIT RESPONSE");
    }

    AddProtobufItem(CommitResponseProto_, NHollywood::RESPONSE_ITEM);
}

} // namespace NAlice::NHollywoodFw::NPrivate


namespace NAlice::NHollywood {

/*
    Prepare return arguments for Continue, Apply and Commit arguments
*/
google::protobuf::Any PrepareArguments(const google::protobuf::Any& scenarioArgs, const TScenarioNewContext* newContext) {
    if (newContext == nullptr) {
        // This is an old scenario without forwarding from HWF
        // Does nothing, just return scenario arguments
        return scenarioArgs;
    }

    // Wrap scenario arguments with protobuf
    Y_ENSURE(newContext->NodeCaller != nullptr, "Internal error: no Node Caller");
    NHollywoodFw::TProtoHwScene sceneResults = newContext->NodeCaller->ExportToProto(false, false);

    NHollywoodFw::TProtoHwSceneCCAArguments frameworkArguments;
    *frameworkArguments.MutableScenarioArgs() = scenarioArgs;
    *frameworkArguments.MutableProtoHwScene() = std::move(sceneResults);

    google::protobuf::Any resultArguments;
    resultArguments.PackFrom(frameworkArguments);
    return std::move(resultArguments);
}

google::protobuf::Any PrepareOldFlowArguments(const google::protobuf::Any& scenarioArgs) {
    NHollywoodFw::TProtoHwSceneCCAArguments frameworkArguments;
    *frameworkArguments.MutableScenarioArgs() = scenarioArgs;
    frameworkArguments.MutableProtoHwScene()->SetOldFlowSelected(true);

    google::protobuf::Any resultArguments;
    resultArguments.PackFrom(frameworkArguments);
    return std::move(resultArguments);
}

} // namespace NAlice::NHollywood
