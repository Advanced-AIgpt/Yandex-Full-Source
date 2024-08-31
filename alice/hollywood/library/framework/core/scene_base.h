//
// HOLLYWOOD FRAMEWORK
// ScenarioEntiry declarations
//

#pragma once

#include "error.h"
#include "render.h"
#include "return_types.h"
#include "scenario_baseinit.h"
#include "setup.h"
#include "source.h"

#include <google/protobuf/any.pb.h>

#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

namespace NAlice::NHollywoodFw {

//
// Forward declarations
//
class TScenario;
class TRequest;
class TRunRequest;
class TContinueRequest;
class TApplyRequest;
class TCommitRequest;
class TStorage;

namespace {

template<class Ret, typename... Args> constexpr EStageDeclaration GetScenarioStage();

template<>
constexpr EStageDeclaration GetScenarioStage<TRetSetup, const TRunRequest&, const TStorage&>() {
    return EStageDeclaration::SceneSetupRun;
}
template<>
constexpr EStageDeclaration GetScenarioStage<TRetMain, const TRunRequest&, TStorage&, const TSource&>() {
    return EStageDeclaration::SceneMainRun;
}
template<>
constexpr EStageDeclaration GetScenarioStage<TRetSetup, const TContinueRequest&, const TStorage&>() {
    return EStageDeclaration::SceneSetupContinue;
}
template<>
constexpr EStageDeclaration GetScenarioStage<TRetContinue, const TContinueRequest&, TStorage&, const TSource&>() {
    return EStageDeclaration::SceneMainContinue;
}
template<>
constexpr EStageDeclaration GetScenarioStage<TRetSetup, const TApplyRequest&, const TStorage&>() {
    return EStageDeclaration::SceneSetupApply;
}
template<>
constexpr EStageDeclaration GetScenarioStage<TRetContinue, const TApplyRequest&, TStorage&, const TSource&>() {
    return EStageDeclaration::SceneMainApply;
}
template<>
constexpr EStageDeclaration GetScenarioStage<TRetSetup, const TCommitRequest&, const TStorage&>() {
    return EStageDeclaration::SceneSetupCommit;
}
template<>
constexpr EStageDeclaration GetScenarioStage<TRetCommit, const TCommitRequest&, TStorage&, const TSource&>() {
    return EStageDeclaration::SceneMainCommit;
}

} // anonimous namespace


namespace NPrivate {

class TScenarioFactory;
class TNodeCaller;

//
// Base caller class for TScene objects
// Internal declaration, can't be used outside framework
//
class TSceneBaseCaller {
public:
    TSceneBaseCaller(const TScenario* owner, EStageDeclaration stage);
    virtual ~TSceneBaseCaller() = default;

    const TScenario* GetScenario() const {
        return Owner_;
    }
    EStageDeclaration GetStage() const {
        return Stage_;
    }
    const TString& GetProtoClassName() const {
        return ProtoClassName_;
    }

    virtual TRetSetup Call(const google::protobuf::Any&, const TRunRequest&, const TStorage&) const {
        return ErrorHandlerNotFound("RunSetup()");
    }
    virtual TRetMain Call(const google::protobuf::Any&, const TRunRequest&, TStorage&, const TSource&) const {
        return ErrorHandlerNotFound("Run()");
    }
    virtual TRetSetup Call(const google::protobuf::Any&, const TContinueRequest&, const TStorage&) const {
        return ErrorHandlerNotFound("ContinueSetup()");
    }
    virtual TRetContinue Call(const google::protobuf::Any&, const TContinueRequest&, TStorage&, const TSource&) const {
        return ErrorHandlerNotFound("Continue()");
    }
    virtual TRetSetup Call(const google::protobuf::Any&, const TCommitRequest&, const TStorage&) const {
        return ErrorHandlerNotFound("CommitSetup()");
    }
    virtual TRetCommit Call(const google::protobuf::Any&, const TCommitRequest&, TStorage&, const TSource&) const {
        return ErrorHandlerNotFound("Commit()");
    }
    virtual TRetSetup Call(const google::protobuf::Any&, const TApplyRequest&, const TStorage&) const {
        return ErrorHandlerNotFound("ApplySetup()");
    }
    virtual TRetContinue Call(const google::protobuf::Any&, const TApplyRequest&, TStorage&, const TSource&) const {
        return ErrorHandlerNotFound("Apply()");
    }
protected:
    // Protobuf typename (for dynamic casts)
    TString ProtoClassName_;
private:
    TError ErrorHandlerNotFound(TStringBuf node) const {
        TError err(TError::EErrorDefinition::HandlerNotFound);
        err.Details() << "Handler for node '" << node << "' not found.";
        return err;
    }
    // Owner of this scene
    const TScenario* Owner_;
    // Stage of function
    EStageDeclaration Stage_;
};

//
// Endpoint caller class for TScene objects
// Internal declaration, can't be used outside framework
//
template<class Object, class Proto, class Ret, typename... Args>
class TSceneCaller final: public TSceneBaseCaller {
public:
    explicit TSceneCaller(const TScenario* owner,
                                     Object* this_, Ret(Object::*fn)(const Proto& msg, Args...) const)
        : TSceneBaseCaller(owner, GetScenarioStage<Ret, Args...>())
        , This_(this_)
        , Fn_(fn)
    {
        Proto p;
        ProtoClassName_ = p.GetTypeName();
    }
    virtual ~TSceneCaller() {
    }

    virtual Ret Call(const google::protobuf::Any& msg, const Args... args) const override final {
        Proto sceneArgs;
        if (!msg.Is<Proto>() || !msg.UnpackTo(&sceneArgs)) {
            TError err(TError::EErrorDefinition::ProtobufCast);
            err.Details() << "Can not upcast protobuf to user-defined object. " <<
                             "Received object: '" << msg.type_url() << '\'' <<
                             "Expected object: '" << sceneArgs.GetTypeName() << '\'' <<
                             "Scenario: '" << GetScenario()->GetName() << '\'';
            return err;
        }
        return (This_->*Fn_)(sceneArgs, args...);
    }

private:
    Object* This_;
    Ret(Object::*Fn_)(const Proto& msg, Args...) const;
};

} // namespace NPrivate

//
// Main Scenario scene base class
//
class TSceneBase {
friend class NPrivate::TScenarioFactory;
friend class TScenario;
public:
    explicit TSceneBase(const TScenario* owner, TStringBuf sceneName);
    virtual ~TSceneBase() {}

    const TScenario* GetOwner() const {
        return Owner_;
    }
    const TString& GetSceneName() const {
        return SceneName_;
    }
    const TString& GetProtoClassName() const {
        return ProtoClassName_;
    }
    const TString& GetProductScenarioName() const {
        return ProductScenarioName_;
    }

    //
    // Additional helpers
    //
    const NPrivate::TSceneBaseCaller* FindHandlerByStage(EStageDeclaration stage) const;

protected:
    //
    // Initialization functions allowed to be call from scene handler
    // These functions forward call to scenario owner
    //
    //
    // Startup initialization functions
    // Note these functions can be called in scenario/scene ctors only!
    //

    // Set custom product scenario name
    // Note this PSN is applicable for this scene only!
    void SetProductScenarioName(const TStringBuf& psn) {
        ProductScenarioName_ = TString{psn};
    }


    // Register renderers as a free function / class static function
    template <class Proto>
    void RegisterRenderer(TRetResponse (*fn)(const Proto&, TRender&)) {
        return BaseOwner_->RegisterRenderer(fn);
    }

    // Register renderers as a class member function
    template <class Object, class Proto>
    void RegisterRenderer(TRetResponse (Object::*fn)(const Proto&, TRender&) const) {
        return BaseOwner_->RegisterRenderer(fn, this);
    }

    // Set allowed protobuf types for scenario state
    template <class TScenarioStateProto>
    void AddScenarioState() {
        BaseOwner_->AddScenarioState<TScenarioStateProto>();
    }

    // Set FastData
    template <typename TProto, typename TParsedFastData>
    void AddFastData(const TString& fileName) const {
        BaseOwner_->AddFastData<TProto, TParsedFastData>(fileName);
    }

protected:
    // Scenario-owner of this scene
    const TScenario* Owner_;
    TScenarioBase* BaseOwner_;

private:
    // Entiity name
    TString SceneName_;
    // Default protobuf typename
    TString ProtoClassName_;
    // Product Scenario Name (for this scene only)
    TString ProductScenarioName_;
    // List of all scene handlers
    TVector<std::unique_ptr<NPrivate::TSceneBaseCaller>> HandlersMap_;

private:
    void FinishInitialization();

    // Register scene handler function
    // Internal function, called from TScenario::RegisterSceneFn
    template <class Object, class Proto, typename Ret, typename ... Args>
    void Register(Ret(Object::*fn)(const Proto&, Args...) const)
    {
        Object* this_ = static_cast<Object*>(this);
        // Create new caller object and store it info the map
        std::unique_ptr<NPrivate::TSceneBaseCaller> caller(new NPrivate::TSceneCaller<Object, Proto, Ret, Args...>(Owner_, this_, fn));
        StoreTypename(caller->GetProtoClassName());
        HandlersMap_.push_back(std::move(caller));
    }
    void StoreTypename(const TString& t);

    // Call node handles
    void CallSceneNode(NPrivate::TNodeCaller& callerData) const;
    void CallSceneRunSetup(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const;
    void CallSceneRun(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const;
    void CallSceneContinueSetup(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const;
    void CallSceneContinue(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const;
    void CallSceneApplySetup(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const;
    void CallSceneApply(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const;
    void CallSceneCommitSetup(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const;
    void CallSceneCommit(NPrivate::TNodeCaller& callerData, const NPrivate::TSceneBaseCaller* node) const;
};

} // namespace NAlice::NHollywoodFw
