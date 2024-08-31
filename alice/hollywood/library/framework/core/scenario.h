#pragma once

//
// HOLLYWOOD FRAMEWORK
// Base scenario interface
//
#include "error.h"
#include "render.h"
#include "request.h"
#include "return_types.h"
#include "run_features.h"
#include "scenario_baseinit.h"
#include "scene.h"
#include "setup.h"
#include "source.h"
#include "storage.h"

#include <alice/hollywood/library/fast_data/fast_data.h>
#include <alice/hollywood/library/nlg/nlg.h>
#include <alice/hollywood/library/resources/resources.h>

#include <alice/library/metrics/sensors.h>

#include <library/cpp/json/json_value.h>

#include <google/protobuf/any.pb.h>

#include <util/generic/fwd.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/string.h>

#include <exception>

namespace NAlice::NHollywood {
    class TScenario;
}

namespace NAlice::NHollywoodFw {

//
// Forward declarations
//
class TProtoHwFramework;

namespace {

template <class Ret, typename... Args>
constexpr EStageDeclaration GetTScenarioStage();

template <>
constexpr EStageDeclaration GetTScenarioStage<TRetSetup, const TRunRequest&, const TStorage&>() {
    return EStageDeclaration::DispatchSetup;
}
template <>
constexpr EStageDeclaration GetTScenarioStage<TRetScene, const TRunRequest&, const TStorage&, const TSource&>() {
    return EStageDeclaration::Dispatch;
}

} // anonimous namespace

namespace NPrivate {

class TScenarioFactory;
class TNodeCaller;

//
// Function selector for main scenario handler
//
class TScenarioBaseCaller {
public:
    virtual ~TScenarioBaseCaller() = default;

    virtual TRetSetup Call(const TRunRequest&, const TStorage&) const {
        return ErrorHandlerNotFound("DispatchSetup()");
    }
    virtual TRetScene Call(const TRunRequest&, const TStorage&, const TSource&) const {
        return ErrorHandlerNotFound("Dispatch()");
    }
    EStageDeclaration GetStage() const {
        return Selector_;
    }
protected:
    explicit TScenarioBaseCaller(EStageDeclaration selector)
        : Selector_(selector)
    {
    }
private:
    TError ErrorHandlerNotFound(TStringBuf call) const {
        TError err(TError::EErrorDefinition::HandlerNotFound);
        err.Details() << "Handler not found. Type: "<< call
                      << "Stage: " << Selector_;
        return err;
    }
private:
    EStageDeclaration Selector_;
};

template <typename Object, class Ret, typename... Args>
class TScenarioCaller: public TScenarioBaseCaller {
public:
    explicit TScenarioCaller(Object* this_, Ret(Object::*fn)(Args...) const)
        : TScenarioBaseCaller(GetTScenarioStage<Ret, Args...>())
        , This_(this_)
        , Fn_(fn)
    {
    }
    virtual Ret Call(Args... args) const override {
        return (This_->*Fn_)(args...);
    }

    EStageDeclaration GetSelector();
private:
    Object* This_;
    Ret(Object::*Fn_)(Args...) const;
};

} // namespace NPrivate

//
// Base Scenario handler interface
//
class TScenario: public TScenarioBase {
friend class NPrivate::TScenarioFactory;
public:
    // Set DIV Render mode
    enum class EDivRenderMode {
        PrepareForRender,         // DivRender will be prepared in 'Main' stage and should be ready before 'Render' stage
        PrepareForOutsideMerge    // DivRender will be prepared in 'Render' stage (old style processing)
    };

    // dtor
    virtual ~TScenario();

    // Register scene function
    template <class Object, class Proto, typename Ret, typename ... Args>
    void RegisterSceneFn(Ret(Object::*fn)(const Proto&, Args...) const)
    {
        Y_ENSURE(CurrentSceneRegister_ != nullptr, "This call allowed inside RegisterScene() call only!");
        CurrentSceneRegister_->Register(fn);
    }

    //
    // Internal functions
    //
    const NPrivate::TScenarioBaseCaller* FindScenarioNodeByStage(EStageDeclaration stage) const;
    const TSceneBase* FindScene(const TString& sceneName) const;

    const NHollywood::IResourceContainer* GetResources() const {
        return Resources_.get();
    }

    // Set resources for scenario
    template <typename TScenarioResources, typename... TArgs>
    TScenario& SetResources(TArgs&& ...args) {
        Resources_ = std::make_unique<TScenarioResources>(std::forward<TArgs>(args)...);
        return *this;
    }
    bool IsPrepareDivRenderOnFinalize() const {
        return DivRenderMode_ == EDivRenderMode::PrepareForOutsideMerge;
    }

protected:
    // ctor
    // Scnario name must be set in lower_case format!
    TScenario(TStringBuf str);

    // Set DIV Render mode
    void SetDivRenderMode(EDivRenderMode mode) {
        DivRenderMode_ = mode;
    }

    // Register scene functions
    template <class Object, typename Ret, typename ... Args>
    void Register(Ret(Object::*fn)(Args...) const)
    {
        Object* this_ = static_cast<Object*>(this);
        std::unique_ptr<NPrivate::TScenarioBaseCaller> holder(new NPrivate::TScenarioCaller<Object, Ret, Args...>(this_, fn));
        HandlersMap_.push_back(std::move(holder));
    }

    // Register scene and scene functions
    template <class Scene>
    void RegisterScene(std::function<void()> fn) {
        TSceneBase* scene = new Scene(this);
        // Attach this scene to generic list and build
        AllScenes_.emplace_back(scene);
        // Register all handlers inside fn
        CurrentSceneRegister_ = scene;
        fn();
        CurrentSceneRegister_ = nullptr;
    }

    // Default ctor to scene selector
    // New version, see https://wiki.yandex-team.ru/alice/hollywood/hwfhotchanges/#23marta
    template <class Object, class TSceneArgsProto>
    NPrivate::TRetSceneSelector TReturnValueScene(const TSceneArgsProto& proto, const TString& selectedIntent = "") const {
        // Ensure that this scene can handle specified protobuf TSceneArgsProto
        static_assert(std::is_base_of<TScene<TSceneArgsProto>, Object>::value, "Scene class and scene args are inconsistent");
        TString sceneName = "";
        for (auto& it : AllScenes_) {
            if (dynamic_cast<Object*>(it.get())) {
                sceneName = it->GetSceneName();
                break;
            }
        }
        google::protobuf::Any protoScene;
        protoScene.PackFrom(proto);
        return NPrivate::TRetSceneSelector(std::move(protoScene), sceneName, selectedIntent);
    }

    // Irrelevant renderer (the render is a member of scenario)
    template <class Object, class Proto>
    NPrivate::TRetRenderSelectorIrrelevant TReturnValueRenderIrrelevant(
        TRetResponse (Object::*)(const Proto&, TRender& render) const,
        const Proto& renderArgs,
        TRunFeatures&& features = TRunFeatures{}) const {
        static_assert(std::is_base_of<TScenario, Object>::value, "This render object should be a member of TScenario");
        google::protobuf::Any renderArgsPacked;
        renderArgsPacked.PackFrom(renderArgs);
        return NPrivate::TRetRenderSelectorIrrelevant(std::move(renderArgsPacked), GetName(), std::move(features));
    }
    // Irrelevant renderer (the render is a free function or static member of scenario)
    template <class Proto>
    NPrivate::TRetRenderSelectorIrrelevant TReturnValueRenderIrrelevant(
        TRetResponse (*)(const Proto&, TRender& render),
        const Proto& renderArgs,
        TRunFeatures&& features = TRunFeatures{}) const {
        google::protobuf::Any renderArgsPacked;
        renderArgsPacked.PackFrom(renderArgs);
        return NPrivate::TRetRenderSelectorIrrelevant(std::move(renderArgsPacked), "", std::move(features));
    }
    // Irrelevant renderer (with predefined name and phrase)
    NPrivate::TRetRenderSelectorIrrelevant TReturnValueRenderIrrelevant(const TString& nlgName,
                                                                        const TString& phrase,
                                                                        TRunFeatures&& features = TRunFeatures{}) const;
    NPrivate::TRetRenderSelectorIrrelevant TReturnValueRenderIrrelevant(const TString& nlgName,
                                                                        const TString& phrase,
                                                                        const google::protobuf::Message& context,
                                                                        TRunFeatures&& features = TRunFeatures{}) const;


private:
    // Reset stage initialization flag to protect scenario object from prohibited functions.
    void FinishInitialization(const NNlg::ITranslationsContainerPtr& nlgTranslations, const TFsPath& resourcesBasePath);
    bool HasNode(EStageDeclaration stage) const;
    NJson::TJsonValue DumpScenarioInfo() const;

    // Call scenario hanldes
    void CallMainNode(NPrivate::TNodeCaller& callerData, NMetrics::ISensors& s) const;
    void CallDispatchSetup(NPrivate::TNodeCaller& callerData, const NPrivate::TScenarioBaseCaller* node) const;
    void CallDispatch(NPrivate::TNodeCaller& callerData, const NPrivate::TScenarioBaseCaller* node, NMetrics::ISensors& s) const;
    void CallRenderNode(NPrivate::TNodeCaller& callerData) const;
private:
    // Old linked scenario
    const NAlice::NHollywood::TScenario* OldScenarioFlow_ = nullptr;
    // Current pointer for RegisterScene()
    TSceneBase* CurrentSceneRegister_ = nullptr;
    // List of all main registered handlers
    TVector<std::unique_ptr<NPrivate::TScenarioBaseCaller>> HandlersMap_;
    // All attached scenes
    TVector<std::unique_ptr<TSceneBase>> AllScenes_;
    // Scenario resources
    std::unique_ptr<NHollywood::IResourceContainer> Resources_;
    // Set DIV Render mode
    EDivRenderMode DivRenderMode_ = EDivRenderMode::PrepareForRender;
};

class IScenarioRegistratorBase {
public:
    IScenarioRegistratorBase();
    virtual void CreateScenario() = 0;
    virtual void DestroyScenario() = 0;
    virtual ~IScenarioRegistratorBase() = default;
};

template <class TScenarioClass>
class TScenarioRegistrator : public IScenarioRegistratorBase {
public:
    static TScenarioRegistrator& Instance() {
        static TScenarioRegistrator instance;
        return instance;
    }
private:
    void CreateScenario() override {
        ScenarioObject_ = std::make_unique<TScenarioClass>();
    }
    void DestroyScenario() override {
        ScenarioObject_.reset();
    }

private:
    std::unique_ptr<TScenarioClass> ScenarioObject_;
};

#define HW_REGISTER(Class)      static TScenarioRegistrator<Class> Global##Class

} // namespace NAlice::NHollywoodFw
