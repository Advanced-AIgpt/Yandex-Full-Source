#pragma once

//
// HOLLYWOOD FRAMEWORK
// Internal classes: TNodeCaller
//

#include "error.h"
#include "request.h"
#include "scene.h"
#include "scene_graph.h"
#include "setup.h"
#include "source.h"
#include "storage.h"

#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/metrics/metrics.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/metrics/util.h>
#include <alice/library/json/json.h>
#include <alice/library/proto/proto.h>

#include <util/generic/fwd.h>
#include <util/generic/map.h>
#include <util/generic/string.h>

namespace NAlice::NHollywoodFw::NPrivate {

//
// Arguments received from Main() function in scene
//
struct TCCAArguments {
public:
    ENodeType ArgType = ENodeType::Run;
    google::protobuf::Any Args;
};

//
// Node Caller (TNodeCaller)
// Base class to handle local scenario graph
// Contains all information about objects thansferred between scenario functions
// Can save results to protobufs (hw_scene_item, mm_scenario_response, etc)
// Can restore results on next apphost node
//
// Node Caller also has an extension (TNodeCallerTesting) which uses in HWF unit tests
// TNodeCallerTesting allows to integrate source and results with TTestEnvironment instead of AppHost context
//
class TNodeCaller: public NNonCopyable::TNonCopyable {
public:
    //
    // Ctor and initial setup functions
    //
    TNodeCaller(const TScenario& sc,
                const TString& nodeName,
                NAppHost::IServiceContext& ctx,
                const TProtoHwScene* protoHwScene,
                TRTLogger& logger);
    virtual ~TNodeCaller() = default;
    virtual ENodeType GetNodeType() const = 0;

    void SetInputParameters(const TProtoHwScene* protoScene);
    void SelectScene(const TSceneBase* scene) {
        ScenarioScene_ = scene;
    }
    void DisableErrorReporting() {
        ErrorReportingFlag_ = false;
    }
    void AdjustRandomSeed(const TMap<EStageDeclaration, TString>& customNodeNames, ui64 seed);

    //
    // Working with stages
    //
    EStageDeclaration GetCurrentStage() const {
        if (LocalGraph_ == nullptr) {
            return EStageDeclaration::Undefined;
        }
        return LocalGraph_->At(CurrentStageIndex_);
    }
    NPrivate::EHwProtoSource GetHwItemSource() {
        if (LocalGraph_ == nullptr) {
            return NPrivate::EHwProtoSource::None;
        }
        return LocalGraph_->ProtoSource;
    }
    virtual bool HasThisStage() const;
    bool GoToNextStage();
    void EnterLocalGraph();
    void LeaveLocalGraph();
    void SetSwitchToOldFlow();
    bool IsSwitchToOldFlow() const {
        return NeedSwitchToOldFlow_;
    }
    const TString& GetApphostNodeName() const {
        return ApphostNodeName_;
    }
    NAppHost::IServiceContext& GetApphostCtx() {
        return Ctx_;
    }

    //
    // Bypass and Finalize nodes
    //
    virtual void Bypass();
    virtual void Finalize() = 0;
    virtual void PrepareOldScenario(NHollywood::TScenarioNewContext& oldFlowContext) const;

    //
    // Acsessors
    //
    const TScenario& GetScenario() const {
        return Scenario_;
    };
    const TSceneBase* GetScenarioScene() const {
        return ScenarioScene_;
    }
    const TApphostNode* GetApphostNode() const {
        return LocalGraph_;
    }
    virtual const TRunRequest& GetRunRequest() const {
        Y_ENSURE(false, "This type of node caller can't provide TRunRequest");
    }
    virtual const TContinueRequest& GetContinueRequest() const {
        Y_ENSURE(false, "This type of node caller can't provide TContinueRequest");
    }
    virtual const TApplyRequest& GetApplyRequest() const {
        Y_ENSURE(false, "This type of node caller can't provide TApplyRequest");
    }
    virtual const TCommitRequest& GetCommitRequest() const {
        Y_ENSURE(false, "This type of node caller can't provide TCommitRequest");
    }
    TStorage& GetStorage() {
        Y_ENSURE(Storage_ != nullptr, "Illegal call: TStorage is nullptr");
        return *Storage_;
    }
    const TSource& GetSource() const {
        Y_ENSURE(Source_ != nullptr, "Illegal call: TSource is nullptr");
        return *Source_;
    }
    // These functions can be provided by inherited classes only!
    virtual const TRequest& GetBaseRequest() const  = 0;
    virtual const NScenarios::TScenarioBaseRequest& GetBaseRequestProto() const = 0;
    virtual const NScenarios::TInput* GetInputProto() const = 0;
    virtual NScenarios::TScenarioResponseBody* GetBaseResponseProto() = 0;

    const NPrivate::TRetRenderSelectorBase& GetRenderArgs() const {
        Y_ENSURE(RenderArgs != nullptr, "Illegal call: RenderArgs is nullptr");
        return *RenderArgs;
    }
    const NPrivate::TRetSceneSelector& GetSceneArgs() const {
        Y_ENSURE(SceneArgs != nullptr, "Illegal call: SceneArgs is nullptr");
        return *SceneArgs;
    }

    //
    // Errors messages
    //
    void SetError(const TString& msg);
    void SetErrorFromScenario(const TError& err);
    TProtoHwScene ExportToProto(bool needSaveRenderArgs, bool needSaveCommitContinueApplyArgs) const;
    TRTLogger& Logger() {
        return Logger_;
    }

    template <typename TProto>
    void AddProtobufItem(const TProto& proto, TStringBuf name) {
        Ctx_.AddProtobufItem(proto, name);
    }

    //
    // Debug dumping
    //
    bool IsNeedDumpSceneArgs() const {
        return IsNeedDump(HwfDebugDumpSceneArguments_);
    }
    bool IsNeedDumpRenderArgs() const {
        return IsNeedDump(HwfDebugDumpRenderArguments_);
    }
    bool IsNeedDumpRenderData() const {
        return IsNeedDump(HwfDebugDumpRenderData_);
    }
    template <class TProto> void DebugDump(const TProto& proto, TStringBuf prefix) const {
        if (HwfDebugDumpAsJson_) {
            LOG_INFO(Logger_) << prefix << ": " << JsonStringFromProto(proto);
        } else {
            LOG_INFO(Logger_) << prefix << ": " << SerializeProtoText(proto);
        }
    }

public:
    //
    // TODO: These variables stored in public section because initially this class was a 'struct'.
    // Will be refactored soon
    //
    // Output results
    TError Error = TError(TError::EErrorDefinition::Success);
    std::unique_ptr<TSetup> SetupRequests;
    std::unique_ptr<TRender> Render;
    std::unique_ptr<NPrivate::TRetSceneSelector> SceneArgs;
    std::unique_ptr<TRunFeatures> RunFeatures;
    std::unique_ptr<NPrivate::TRetRenderSelectorBase> RenderArgs;
    // Continue/Commit/Apply arguments
    TCCAArguments CcaArguments;
    // DivRender
    TVector<std::shared_ptr<NRenderer::TDivRenderData>> DivRenderData;

protected:
    void PrepareBaseResponseData();
    TString GetLastError();
    bool IsNeedDump(const TMap<TString, bool>& map) const;

protected:
    const TScenario& Scenario_;
    TRTLogger& Logger_;
    NAppHost::IServiceContext& Ctx_;
    const TSceneBase* ScenarioScene_;
    const TApphostNode* LocalGraph_;
    TString ApphostNodeName_;
    // Initial node selector, received from apphost dispatcher
    size_t CurrentStageIndex_;
    // Flag to go to old scenario flow
    bool NeedSwitchToOldFlow_ = false;

    TProtoHwFramework FrameworkState_;
    std::unique_ptr<TStorage> Storage_;
    std::unique_ptr<TSource> Source_;

    //
    // Additional options received from EXP flags
    //
    bool HwfDebugDumpRunRequest_ = false;
    bool HwfDebugDumpRunResponse_ = false;
    bool HwfDebugDumpApplyRequest_ = false;
    bool HwfDebugDumpContinueRequest_ = false;
    bool HwfDebugDumpContinueResponse_ = false;
    bool HwfDebugDumpApplyResponse_ = false;
    bool HwfDebugDumpCommitRequest_ = false;
    bool HwfDebugDumpCommitResponse_ = false;
    TMap<TString, bool> HwfDebugDumpRenderData_;
    TMap<TString, bool> HwfDebugDumpSceneArguments_;
    TMap<TString, bool> HwfDebugDumpRenderArguments_;
    TMap<TString, bool> HwfDebugDumpContinueArguments_;
    TMap<TString, bool> HwfDebugDumpCommitArguments_;
    TMap<TString, bool> HwfDebugDumpApplyArguments_;
    TMap<TString, bool> HwfDebugDumpDatasource_;
    bool HwfDebugDumpLocalGraph_ = false;
    bool HwfDebugDumpAsJson_ = false;

private:
    // set to false to disable error reporting (useful for UT)
    bool ErrorReportingFlag_ = true;
    // set while we are inside local graph
    bool LocalGraphExecutionFlag_ = false;
};

/*
    TNodeCaller extensions - for Run/Continue/Commit/Apply events
*/
class TNodeCallerRun: public TNodeCaller {
public:
    TNodeCallerRun(const TScenario& sc,
                   const TString& nodeName,
                   NHollywood::IGlobalContext& globalContext,
                   NAppHost::IServiceContext& ctx,
                   const TProtoHwScene* protoHwScene,
                   const NScenarios::TRequestMeta& meta,
                   TRTLogger& logger);

    ENodeType GetNodeType() const override {
        return ENodeType::Run;
    }
    const TRunRequest& GetRunRequest() const override {
        return RunRequest_;
    }
    void PrepareOldScenario(NHollywood::TScenarioNewContext& oldFlowContext) const override;
    const TRequest& GetBaseRequest() const override {
        return RunRequest_;
    }
    const NScenarios::TScenarioBaseRequest& GetBaseRequestProto() const override {
        return RunRequestProto_.GetBaseRequest();
    }
    const NScenarios::TInput* GetInputProto() const override {
        return RunRequestProto_.HasInput() ? &RunRequestProto_.GetInput() : nullptr;
    }
    NScenarios::TScenarioResponseBody* GetBaseResponseProto() override {
        return RunResponseProto_.MutableResponseBody();
    }
    void Finalize() override;

private:
    TRequest::TApphostNodeInfo NodeInfo_;
    NScenarios::TScenarioRunRequest RunRequestProto_;
    TRunRequest RunRequest_;
    NScenarios::TScenarioRunResponse RunResponseProto_;
    const NScenarios::TRequestMeta& Meta_;
};

class TNodeCallerContinue: public TNodeCaller {
public:
    TNodeCallerContinue(const TScenario& sc,
                        const TString& nodeName,
                        NScenarios::TScenarioApplyRequest& continueRequest,
                        NHollywood::IGlobalContext& globalContext,
                        NAppHost::IServiceContext& ctx,
                        const TProtoHwScene* protoHwScene,
                        const NScenarios::TRequestMeta& meta,
                        TRTLogger& logger);

    ENodeType GetNodeType() const override {
        return ENodeType::Continue;
    }
    const TContinueRequest& GetContinueRequest() const override {
        return ContinueRequest_;
    }
    void PrepareOldScenario(NHollywood::TScenarioNewContext& oldFlowContext) const override;
    const TRequest& GetBaseRequest() const override {
        return ContinueRequest_;
    }
    const NScenarios::TScenarioBaseRequest& GetBaseRequestProto() const override {
        return ContinueRequestProto_.GetBaseRequest();
    }
    const NScenarios::TInput* GetInputProto() const override {
        return ContinueRequestProto_.HasInput() ? &ContinueRequestProto_.GetInput() : nullptr;
    }
    NScenarios::TScenarioResponseBody* GetBaseResponseProto() override {
        return ContinueResponseProto_.MutableResponseBody();
    }
    void Finalize() override;

private:
    TRequest::TApphostNodeInfo NodeInfo_;
    NScenarios::TScenarioApplyRequest& ContinueRequestProto_;
    TContinueRequest ContinueRequest_;
    NScenarios::TScenarioContinueResponse ContinueResponseProto_;
    const NScenarios::TRequestMeta& Meta_;
};

class TNodeCallerApply: public TNodeCaller {
public:
    TNodeCallerApply(const TScenario& sc,
                     const TString& nodeName,
                     NScenarios::TScenarioApplyRequest& applyRequest,
                     NHollywood::IGlobalContext& globalContext,
                     NAppHost::IServiceContext& ctx,
                     const TProtoHwScene* protoHwScene,
                     const NScenarios::TRequestMeta& meta,
                     TRTLogger& logger);

    ENodeType GetNodeType() const override {
        return ENodeType::Apply;
    }
    const TApplyRequest& GetApplyRequest() const override {
        return ApplyRequest_;
    }
    void PrepareOldScenario(NHollywood::TScenarioNewContext& oldFlowContext) const override;
    const TRequest& GetBaseRequest() const override {
        return ApplyRequest_;
    }
    const NScenarios::TScenarioBaseRequest& GetBaseRequestProto() const override {
        return ApplyRequestProto_.GetBaseRequest();
    }
    const NScenarios::TInput* GetInputProto() const override {
        return ApplyRequestProto_.HasInput() ? &ApplyRequestProto_.GetInput() : nullptr;
    }
    NScenarios::TScenarioResponseBody* GetBaseResponseProto() override {
        return ApplyResponseProto_.MutableResponseBody();
    }
    void Finalize() override;

private:
    TRequest::TApphostNodeInfo NodeInfo_;
    NScenarios::TScenarioApplyRequest& ApplyRequestProto_;
    TApplyRequest ApplyRequest_;
    NScenarios::TScenarioApplyResponse ApplyResponseProto_;
    const NScenarios::TRequestMeta& Meta_;
};

class TNodeCallerCommit: public TNodeCaller {
public:
    TNodeCallerCommit(const TScenario& sc,
                      const TString& nodeName,
                      NScenarios::TScenarioApplyRequest& commitRequest,
                      NHollywood::IGlobalContext& globalContext,
                      NAppHost::IServiceContext& ctx,
                      const TProtoHwScene* protoHwScene,
                      const NScenarios::TRequestMeta& meta,
                      TRTLogger& logger);

    ENodeType GetNodeType() const override {
        return ENodeType::Commit;
    }
    const TCommitRequest& GetCommitRequest() const override {
        return CommitRequest_;
    }
    void PrepareOldScenario(NHollywood::TScenarioNewContext& oldFlowContext) const override;
    const TRequest& GetBaseRequest() const override {
        return CommitRequest_;
    }
    const NScenarios::TScenarioBaseRequest& GetBaseRequestProto() const override {
        return CommitRequestProto_.GetBaseRequest();
    }
    const NScenarios::TInput* GetInputProto() const override {
        return CommitRequestProto_.HasInput() ? &CommitRequestProto_.GetInput() : nullptr;
    }
    NScenarios::TScenarioResponseBody* GetBaseResponseProto() override {
        return nullptr;
    }
    void Finalize() override;

private:
    TRequest::TApphostNodeInfo NodeInfo_;
    NScenarios::TScenarioApplyRequest& CommitRequestProto_;
    TCommitRequest CommitRequest_;
    NScenarios::TScenarioCommitResponse CommitResponseProto_;
    const NScenarios::TRequestMeta& Meta_;
};

} // namespace NAlice::NHollywoodFw::NPrivate
