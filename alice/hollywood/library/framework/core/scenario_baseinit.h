#pragma once

//
// HOLLYWOOD FRAMEWORK
// Base scenario interface
// Base initialization functions used for both scenarios and scenes
//
#include "return_types.h"

#include <alice/hollywood/library/fast_data/fast_data.h>
#include <alice/hollywood/library/nlg/nlg.h>

#include <google/protobuf/any.pb.h>

#include <util/generic/fwd.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/string/join.h>

namespace NAlice::NScenarios {

    class TScenarioRunRequest;
    class TScenarioApplyRequest;
    class TScenarioCommitRequest;
    class TScenarioRunResponse;
    class TScenarioContinueResponse;
    class TScenarioApplyResponse;
    class TScenarioCommitResponse;

} // namespace NAlice::NScenarios

namespace NAlice::NHollywoodFw {

//
// Forward declarations
//
class TRender;
using IFastData = NHollywood::IFastData;
using TFastDataInfo = NHollywood::TFastDataInfo;

//
// Closed factory implementation (not for public usage)
//
namespace NPrivate {
    class TScenarioGraphs;
    class TScenarioFactory;
}

enum class EStageDeclaration {
    Undefined,
    DispatchSetup,
    Dispatch,
    SceneSetupRun,
    SceneMainRun,
    SceneSetupContinue,
    SceneMainContinue,
    SceneSetupApply,
    SceneMainApply,
    SceneSetupCommit,
    SceneMainCommit,
    Render,
    Bypass,     // Last node to prepare ctx data (bypass to next apphost graph)
    Finalize,   // Last node to prepare mm_response
};


//
// Scenario graphs declarations
//
class TScenarioGraphFlow {
public:
    struct TLocalGraphNode {
        // Node name incoming from apphost
        TString NodeName;
        // Optional experiment flag to select this graph
        TString ExpFlag;
        // Initial stage for this local graph
        EStageDeclaration InitialStage;
    };

    TScenarioGraphFlow operator >> (TScenarioGraphFlow rhs) const {
        for (const auto& it : GraphEntries_) {
            rhs.GraphEntries_.push_back(it);
        }
        return rhs;
    }
    TScenarioGraphFlow(TStringBuf apphostNodeName, TStringBuf expFlag, EStageDeclaration initialStage) {
        GraphEntries_.push_back(TLocalGraphNode{TString(apphostNodeName), TString(expFlag), initialStage});
    }
    // Access to local objects
    size_t size() const {
        return GraphEntries_.size();
    }
    const TLocalGraphNode& At(size_t pos) const {
        return GraphEntries_[pos];
    }
    // Get an item from array with special behavior - in reverse order, skip 1st and last object
    const TLocalGraphNode& AtReverse(size_t pos) const {
        return GraphEntries_[GraphEntries_.size() - (2 + pos)];
    }
    const TLocalGraphNode& First() const {
        return GraphEntries_[GraphEntries_.size() - 1];
    }
    const TLocalGraphNode& Last() const {
        return GraphEntries_[0];
    }

private:
    TVector <TLocalGraphNode> GraphEntries_;
};

class TScenarioGraphFlowContinue : public TScenarioGraphFlow {
public:
    TScenarioGraphFlowContinue(TStringBuf apphostNodeName, TStringBuf expFlag, EStageDeclaration initialStage)
        : TScenarioGraphFlow(apphostNodeName, expFlag, initialStage)
    {
    }
};

class TScenarioGraphFlowCommit : public TScenarioGraphFlow {
public:
    TScenarioGraphFlowCommit(TStringBuf apphostNodeName, TStringBuf expFlag, EStageDeclaration initialStage)
        : TScenarioGraphFlow(apphostNodeName, expFlag, initialStage)
    {
    }
};

class TScenarioGraphFlowApply : public TScenarioGraphFlow {
public:
    TScenarioGraphFlowApply(TStringBuf apphostNodeName, TStringBuf expFlag, EStageDeclaration initialStage)
        : TScenarioGraphFlow(apphostNodeName, expFlag, initialStage)
    {
    }
};

namespace NPrivate {

//
// Function selector for renderers
//
class TScenarioRenderBaseCaller {
public:
    TScenarioRenderBaseCaller(const TString& scenePath, const TString& msgName)
        : ScenePath_(scenePath)
        , MsgName_(msgName)
    {
    }
    virtual ~TScenarioRenderBaseCaller() = default;
    virtual TRetResponse Call(const google::protobuf::Any& msg, TRender& render) const = 0;

    bool IsSame(const TString& scenePath, const TString& msgName) const {
        return ScenePath_ == scenePath && MsgName_ == msgName;
    }
    bool operator == (const TScenarioRenderBaseCaller& rhs) const {
        return ScenePath_ == rhs.ScenePath_ && MsgName_ == rhs.MsgName_;
    }
    // For debugging purposes only
    const TString& GetScenePath() const {
        return ScenePath_;
    }
    const TString& GetMsgName() const {
        return MsgName_;
    }
private:
    TString ScenePath_;
    TString MsgName_;
};

//
// Final render caller for memrbes of scenario of scene functions
//
template <class Obj, class Proto>
class TScenarioRenderCaller final: public TScenarioRenderBaseCaller {
public:
    TScenarioRenderCaller(const Obj* obj, TRetResponse(Obj::*fn)(const Proto&, TRender&) const, const TString& scenePath, const TString& msgName)
        : TScenarioRenderBaseCaller(scenePath, msgName)
        , Obj_(obj)
        , Fn_(fn)
    {
    }
    TRetResponse Call(const google::protobuf::Any& msg, TRender& render) const override final {
        Proto renderArgs;
        if (!msg.Is<Proto>() || !msg.UnpackTo(&renderArgs)) {
            TError err(TError::EErrorDefinition::ProtobufCast);
            err.Details() << "Can not upcast protobuf to user-defined object. " <<
                             "Received object: '" << msg.type_url() << '\'' <<
                             "Expected object: '" << renderArgs.GetTypeName() << '\'' <<
                             "Path: '" << GetScenePath() << '\'';
            return err;
        }
        return (Obj_->*Fn_)(renderArgs, render);
    }
private:
    const Obj* Obj_;
    TRetResponse (Obj::*Fn_)(const Proto&, TRender&) const;
};

//
// Final render caller for free functions / static functions
// Note scene path is always "" for these functions
//
template <class Proto>
class TScenarioRenderCallerFf final: public TScenarioRenderBaseCaller {
public:
    TScenarioRenderCallerFf(TRetResponse(*fn)(const Proto&, TRender&), const TString& msgName)
        : TScenarioRenderBaseCaller("", msgName)
        , Fn_(fn)
    {
    }
    TRetResponse Call(const google::protobuf::Any& msg, TRender& render) const override final {
        Proto renderArgs;
        if (!msg.Is<Proto>() || !msg.UnpackTo(&renderArgs)) {
            TError err(TError::EErrorDefinition::ProtobufCast);
            err.Details() << "Can not upcast protobuf to user-defined object. " <<
                             "Received object: '" << msg.type_url() << '\'' <<
                             "Expected object: '" << renderArgs.GetTypeName() << '\'';
                             //TODO
                             //"Scenario: '" << GetScenario()->GetName() << '\''
            return err;
        }
        return (*Fn_)(renderArgs, render);
    }
private:
    TRetResponse(*Fn_)(const Proto&, TRender&);
};

} // namespace NPrivate

//
// Base Scenario handler interface
//
class TScenarioBase {
friend class NPrivate::TScenarioFactory;
public:

    // dtor
    virtual ~TScenarioBase() = default;

    // Return scenario name (set in ctor)
    // Scnario name must be set in lower_case format!
    const TString& GetName() const {
        return Name_;
    }
    const TString& GetProductScenarioName() const {
        return ProductScenarioName_;
    }

    // Register renderers as a free function / class static function
    template <class Proto>
    void RegisterRenderer(TRetResponse (*fn)(const Proto&, TRender&)) {
        Proto temp;
        RendererList_.emplace_back(new NPrivate::TScenarioRenderCallerFf<Proto>(fn, temp.GetTypeName()));
    }

    // Register renderers as a class member function
    template <class Object, class Proto>
    void RegisterRenderer(TRetResponse (Object::*fn)(const Proto&, TRender&) const) {
        Proto temp;
        RendererList_.emplace_back(new NPrivate::TScenarioRenderCaller<Object, Proto>(static_cast<Object*>(this),
            fn, GetName(), temp.GetTypeName()));
    }

    // Register renderers as a scene member function
    template <class Object, class Proto, class Scene>
    void RegisterRenderer(TRetResponse (Object::*fn)(const Proto&, TRender&) const, Scene* scene) {
        Proto temp;
        RendererList_.emplace_back(new NPrivate::TScenarioRenderCaller<Object, Proto>(static_cast<Object*>(scene), fn,
            Join("/", GetName(), scene->GetSceneName()), temp.GetTypeName()));
    }

    //
    // Internal functions
    //
    inline const NPrivate::TScenarioGraphs* GetScenarioGraphs() const {
        Y_ENSURE(ScenarioGraphs_.get(), "Trying to access to scene graphs without FinishInitialization()");
        return ScenarioGraphs_.get();
    }

    //
    // Startup initialization functions
    // Note these functions can be called in scenario/scene ctors only!
    //

    // Set semantic frame names allowed for this scenario (empty = all frames are allowed)
    // If semantic frame is not listed EErrorDefinition::NotAllowed error will be returned w/o scenario call
    void AddSemanticFrame(TStringBuf frameName) {
        Y_ENSURE(InitializationStageFlag_, "This call is allowed in ctor only!");
        AllowedSemanticFrames_.insert(frameName.Data());
    }

    // Set allowed protobuf types for scenario state
    template <class TScenarioStateProto>
    void AddScenarioState() {
        TScenarioStateProto obj;
        AllowedProtobufs_.insert(obj.GetTypeName());
    }

    // Attach NLG templates to scenario
    void SetNlgRegistration(NHollywood::TCompiledNlgComponent::TRegisterFunction registerFunction);

    // Register apphost graph for this scenario
    void SetApphostGraph(const TScenarioGraphFlow& flow);

    // TODO: SetLogLevel
    // Set FastData
    template <typename TProto, typename TParsedFastData>
    void AddFastData(const TString& fileName) {
        std::pair<NHollywood::TFastDataProtoProducer, NHollywood::TFastDataProducer> producers{
            []() { return std::make_shared<TProto>(); },
            [fileName](const NHollywood::TScenarioFastDataProtoPtr proto) {
                Y_ENSURE(proto != nullptr,
                    TStringBuilder() << "Null pointer is passed as FastData constructor argument, proto file: "
                                    << fileName);
                return std::make_shared<TParsedFastData>(dynamic_cast<const TProto&>(*proto));
            }
        };
        FastDataInfo_.emplace_back(fileName, producers);
    }

    // Setup custom hash for random seed initilization
    void SetCustomRandomHash(EStageDeclaration stage, TStringBuf hashNodeName) {
        CustomNodeNames_[stage] = TString{hashNodeName};
    }
    void DisableCustomRandomHash(EStageDeclaration stage) {
        CustomNodeNames_[stage] = "";
    }

    NPrivate::TScenarioRenderBaseCaller* FindRender(const TString& protoName, const TString& scenePath) const;

    bool IsDebugLocalGraph() const {
        return DebugGraphFlag_;
    }

    const TMaybe<NHollywood::TCompiledNlgComponent>& GetNlg() const {
        return Nlg_;
    }

    //
    // HOOKS
    //
    struct THookInputInfo {
        ENodeType NodeType;
        NAppHost::IServiceContext& Ctx;
        const NScenarios::TRequestMeta& Meta;
        const NHollywood::TScenarioNewContext* NewContext;
        TString SceneName; // SceneName can be "" in DispatchSetup/Dispatch hook
        TMaybe<google::protobuf::Any> SceneAruments;   // Can be Nothing in DispatchSetup/Dispatch hook
        TMaybe<google::protobuf::Any> RenderArguments; // Can be Nothing before MainRun() phase

        THookInputInfo(ENodeType nodeType,
                       NAppHost::IServiceContext& ctx,
                       const NScenarios::TRequestMeta& meta,
                       const TProtoHwScene* protoHwScene,
                       const NHollywood::TScenarioNewContext* NewContext);
    };

    virtual void Hook(THookInputInfo& info, NScenarios::TScenarioRunRequest& runRequest) const {
        Y_UNUSED(info);
        Y_UNUSED(runRequest);
    }
    virtual void Hook(THookInputInfo& info, NScenarios::TScenarioApplyRequest& applyRequest) const {
        // Note apply and continue requests share the same protobuf. For more details use THookInputInfo::NodeType
        Y_UNUSED(info);
        Y_UNUSED(applyRequest);
    }
    virtual void Hook(THookInputInfo& info, NScenarios::TScenarioCommitRequest& commitRequest) const {
        Y_UNUSED(info);
        Y_UNUSED(commitRequest);
    }
    virtual void Hook(THookInputInfo& info, NScenarios::TScenarioRunResponse& runResponse) const {
        Y_UNUSED(info);
        Y_UNUSED(runResponse);
    }
    virtual void Hook(THookInputInfo& info, NScenarios::TScenarioContinueResponse& continueResponse) const {
        Y_UNUSED(info);
        Y_UNUSED(continueResponse);
    }
    virtual void Hook(THookInputInfo& info, NScenarios::TScenarioApplyResponse& applyResponse) const {
        Y_UNUSED(info);
        Y_UNUSED(applyResponse);
    }
    virtual void Hook(THookInputInfo& info, NScenarios::TScenarioCommitResponse& commitResponse) const {
        Y_UNUSED(info);
        Y_UNUSED(commitResponse);
    }

protected:
    // ctor
    // Scnario name must be set in lower_case format!
    TScenarioBase(TStringBuf scenarioName);

    // Set default PSN for all scenario (by default == scenarioName)
    void SetProductScenarioName(TStringBuf productScenarioName);
    // Enable debug log with detailed local scenario graph state/info
    void EnableDebugGraph() {
        DebugGraphFlag_ = true;
    }

    // Functions to prepare SetApphostGraph()
    TScenarioGraphFlow ScenarioRequest();
    TScenarioGraphFlow ScenarioResponse();
    TScenarioGraphFlowContinue ScenarioContinue();
    TScenarioGraphFlowApply ScenarioApply();
    TScenarioGraphFlowCommit ScenarioCommit();

    TScenarioGraphFlow NodeRun(TStringBuf apphostNodeName = "run", TStringBuf expFlag = "");
    TScenarioGraphFlow NodeMain(TStringBuf apphostNodeName = "main", TStringBuf expFlag = "");
    TScenarioGraphFlow NodeRender(TStringBuf apphostNodeName = "render", TStringBuf expFlag = "");
    TScenarioGraphFlow NodePreselect(TStringBuf apphostNodeName = "preselect", TStringBuf expFlag = "");
    TScenarioGraphFlowContinue NodeContinueSetup(TStringBuf apphostNodeName = "continue_setup", TStringBuf expFlag = "");
    TScenarioGraphFlowContinue NodeContinue(TStringBuf apphostNodeName = "continue", TStringBuf expFlag = "");
    TScenarioGraphFlowApply NodeApplySetup(TStringBuf apphostNodeName = "apply_setup", TStringBuf expFlag = "");
    TScenarioGraphFlowApply NodeApply(TStringBuf apphostNodeName = "apply", TStringBuf expFlag = "");
    TScenarioGraphFlowCommit NodeCommitSetup(TStringBuf apphostNodeName = "commit_setup", TStringBuf expFlag = "");
    TScenarioGraphFlowCommit NodeCommit(TStringBuf apphostNodeName = "commit", TStringBuf expFlag = "");

protected:
    // Scenario name in lower_case format
    TString Name_;
    // Pointer to scenario graph (built in FinishInitialization())
    std::unique_ptr<NPrivate::TScenarioGraphs> ScenarioGraphs_;
    // List of all main registered handlers
    TVector<std::unique_ptr<NPrivate::TScenarioRenderBaseCaller>> RendererList_;
    // 'Scenario inside ctor' flag
    bool InitializationStageFlag_;
    // NLG information
    NHollywood::TCompiledNlgComponent::TRegisterFunction NlgRegisterFunction_;
    TMaybe<NHollywood::TCompiledNlgComponent> Nlg_;
    // Custom stage names for random sequence order
    TMap<EStageDeclaration, TString> CustomNodeNames_;

private:
    TVector<TScenarioGraphFlow> NodeCache_;

    // List of all allowed semantic frames
    TSet<TString> AllowedSemanticFrames_;
    // Product Scenario Name
    TString ProductScenarioName_;
    // List of all allowed protobufs
    TSet<TString> AllowedProtobufs_;

    // FastData
    TVector<TFastDataInfo> FastDataInfo_;
    // true to dump debug graphs
    bool DebugGraphFlag_;
};

} // namespace NAlice::NHollywoodFw
