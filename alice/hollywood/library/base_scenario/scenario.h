#pragma once

#include "fwd.h"

#include <alice/hollywood/library/context/context.h>
#include <alice/hollywood/library/framework/fwd.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/resources/resources.h>
#include <alice/hollywood/library/util/service_context.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <alice/library/util/rng.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>

#include <apphost/api/service/cpp/service.h>

#include <memory>

namespace NAlice::NHollywood {

//
// New framework context
// This structure can be used to migrate old scenarios to new flow
// The pointer in TScenarioHandleContext::NewContext will be not-null if you old scenario
//     will receive control via `NHollywoodFw::TReturnValueDo()`
//
struct TScenarioNewContext {
    // Pointer to run request. Non null for all nodes in /run mode
    const NHollywoodFw::TRunRequest* RunRequest = nullptr;
    // Pointer to continue/commit/apply requests. Non null in selected modes
    const NHollywoodFw::TContinueRequest* ContinueRequest = nullptr;
    const NHollywoodFw::TCommitRequest* CommitRequest = nullptr;
    const NHollywoodFw::TApplyRequest* ApplyRequest = nullptr;
    // Pointer to scene arguments. May be NULL if we are not in scene phase (i.e. Dispatch or Render)
    const google::protobuf::Message* SceneArguments = nullptr;
    // Pointer to network response. Non-null for Dispatch() and SceneMain() nodes
    const NHollywoodFw::TSource* SourceData = nullptr;
    // Pointer to render data. Non-null for Render nodes.
    NHollywoodFw::TRender* RenderData = nullptr;
    // Pointer to render arguments. May be NULL if we are not in render phase
    const google::protobuf::Message* RenderArguments = nullptr;
    // Internal data
    const NHollywoodFw::NPrivate::TNodeCaller* NodeCaller = nullptr;
};

struct TScenarioHandleContext {
    NAppHost::IServiceContext& ServiceCtx;
    const NScenarios::TRequestMeta& RequestMeta;
    TContext& Ctx;
    IRng& Rng;
    ELanguage Lang;
    ELanguage UserLang;
    const NJson::TJsonValue& AppHostParams;
    // Pointer to new framework context (see above)
    const TScenarioNewContext* NewContext;
};

class TScenario {
public:
    class THandleBase {
    public:
        virtual ~THandleBase() = default;

        const TString& ScenarioName() const {
            return ScenarioName_;
        }

        void SetScenarioName(const TString& scenarioName) {
            ScenarioName_ = scenarioName;
        }

        virtual TString Name() const = 0;
        virtual void Do(TScenarioHandleContext&) const = 0;
        virtual void Shutdown() const {
        }

    private:
        TString ScenarioName_;
    };

public:
    explicit TScenario(const TString& name)
        : Name_(name)
    {}

    const TString& Name() const {
        return Name_;
    }

    const TVector<std::unique_ptr<THandleBase>>& Handles() const {
        return Handles_;
    }

    const TVector<TFastDataInfo>& FastDataInfo() const {
        return FastDataInfo_;
    }

    template <typename THandle, typename ...TArgs>
    TScenario&& AddHandle(TArgs... args) && {
        auto handle = std::make_unique<THandle>(std::forward<TArgs>(args)...);
        handle->SetScenarioName(Name());
        Handles_.push_back(std::move(handle));
        return std::move(*this);
    }

    template <typename TProto, typename TParsedFastData>
    TScenario&& AddFastData(const TString& fileName) && {
        std::pair<TFastDataProtoProducer, TFastDataProducer> producers{
            []() { return std::make_shared<TProto>(); },
            [fileName](const TScenarioFastDataProtoPtr proto) {
                Y_ENSURE(proto != nullptr,
                         TStringBuilder() << "Null pointer is passed as FastData constructor argument, proto file: "
                                          << fileName);
                return std::make_shared<TParsedFastData>(dynamic_cast<const TProto&>(*proto));
            }
        };
        FastDataInfo_.emplace_back(fileName, producers);
        return std::move(*this);
    }

    template <typename TScenarioResources, typename... TArgs>
    TScenario&& SetResources(TArgs&& ...args) && {
        Resources_ = std::make_unique<TScenarioResources>(std::forward<TArgs>(args)...);
        return std::move(*this);
    }

    TScenario&& SetNlgRegistration(TCompiledNlgComponent::TRegisterFunction registerFunction) && {
        NlgRegisterFunction_ = std::move(registerFunction);
        return std::move(*this);
    }

    TCompiledNlgComponent::TRegisterFunction GetNlgRegistration() const {
        return NlgRegisterFunction_;
    }

    const IResourceContainer* Resources() const {
        return Resources_.get();
    }

    IResourceContainer* Resources() {
        return Resources_.get();
    }

    void SetNlg(TCompiledNlgComponent nlg) {
        Nlg_ = std::move(nlg);
    }

    const TCompiledNlgComponent* Nlg() const {
        return Nlg_.Get();
    }

private:
    TString Name_;
    TVector<std::unique_ptr<THandleBase>> Handles_;
    TVector<TFastDataInfo> FastDataInfo_;
    std::unique_ptr<IResourceContainer> Resources_;
    TCompiledNlgComponent::TRegisterFunction NlgRegisterFunction_;
    TMaybe<TCompiledNlgComponent> Nlg_;
};

} // namespace NAlice::NHollywood
