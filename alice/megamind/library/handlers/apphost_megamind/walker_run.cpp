#include "walker_run.h"

#include "begemot.h"
#include "blackbox.h"
#include "combinators.h"
#include "components.h"
#include "misspell.h"
#include "node.h"
#include "personal_intents.h"
#include "polyglot.h"
#include "proactivity.h"
#include "query_tokens_stats.h"
#include "responses.h"
#include "saas.h"
#include "speechkit_session.h"
#include "walker_util.h"
#include "websearch.h"
#include "websearch_query.h"

#include <alice/megamind/library/handlers/speechkit/error_interceptor.h>
#include <alice/megamind/library/handlers/speechkit/speechkit.h>
#include <alice/megamind/library/handlers/utils/analytics_logs_context_builder.h>
#include <alice/megamind/library/handlers/utils/sensors.h>

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/response.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/apphost_request/protos/combinators.pb.h>
#include <alice/megamind/library/apphost_request/protos/scenario_errors.pb.h>
#include <alice/megamind/library/factor_storage/factor_storage.h>
#include <alice/megamind/library/new_modifiers/modifier_request_factory.h>
#include <alice/megamind/library/new_modifiers/utils.h>
#include <alice/megamind/library/proactivity/common/common.h>
#include <alice/megamind/library/requestctx/common.h>
#include <alice/megamind/library/search/request.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/walker/requestctx.h>
#include <alice/megamind/library/walker/walker.h>
#include <alice/megamind/library/worldwide/language/is_alice_worldwide_language.h>

#include <alice/megamind/protos/scenarios/combinator_request.pb.h>

namespace NAlice::NMegamind {
namespace {

void PutScenarioErrorsToApphostContext(NMegamind::TItemProxyAdapter& itemAdapter,
                                       const TWalkerResponse& walkerResponse) {
    if (walkerResponse.GetScenarioErrors().Empty()) {
        return;
    }
    NMegamindAppHost::TScenarioErrorsProto scenarioErrors;
    walkerResponse.GetScenarioErrors().ForEachError(
        [&scenarioErrors](const TString& scenarioName, const TString& stage, const TError& error) {
            NMegamindAppHost::TScenarioErrorsProto_TScenarioError proto;
            proto.SetScenario(scenarioName);
            proto.SetStage(stage);
            *proto.MutableError() = ErrorToProto(error);
            *scenarioErrors.AddScenarioErrors() = std::move(proto);
        });
    itemAdapter.PutIntoContext(std::move(scenarioErrors), NMegamind::AH_ITEM_SCENARIO_ERRORS);
}

class TRunSourceResponses final : public TAppHostSourceResponses {
public:
    TRunSourceResponses(IAppHostCtx& ahCtx, const IContext& ctx)
        : AhCtx_{ahCtx}
        , Ctx_{ctx}
    {
    }

private:
    void InitWizardResponse() const override {
        WizardResponse_->Status = AppHostBegemotPostSetup(AhCtx_, Ctx_, WizardResponse_->Object);
    }

    void InitMisspellResponse() const override {
        MisspellResponse_->Status = AppHostMisspellFromContext(AhCtx_, MisspellResponse_->Object);
    }

    void InitBlackBoxResponse() const override {
        const auto err = AppHostBlackBoxPostSetup(AhCtx_, BlackBoxResponse_->Object);
        BlackBoxResponse_->Status = err;
        if (err.Defined()) {
            if (err->Type != TError::EType::Logic) {
                LOG_ERR(AhCtx_.Log()) << "BlackBox response error: " << *err;
            }
        }
    }

    void InitPersonalIntentsResponse() const override {
        PersonalIntentsResponse_->Status = AppHostPersonalIntentsPostSetup(AhCtx_, PersonalIntentsResponse_->Object);
    }

    void InitQueryTokensStatsResponse() const override {
        QueryTokensStatsResponse_->Status = AppHostQueryTokensStatsPostSetup(AhCtx_, QueryTokensStatsResponse_->Object);
    }

    void InitSaasSkillDiscoveryResponse() const override {
        SaasSkillDiscoveryResponse_->Status = AppHostSaasSkillDiscoveryPostSetup(AhCtx_, SaasSkillDiscoveryResponse_->Object);
    }

    void InitPolyglotTranslateUtteranceResponse() const override {
        if (IsAliceWorldWideLanguage(Ctx_.Language())) {
            PolyglotTranslateUtteranceResponse_->Status = AppHostPolyglotPostSetup(AhCtx_, PolyglotTranslateUtteranceResponse_->Object);
        }
    }

    void InitProactivityResponse() const override {
        ProactivityResponse_->Status = AppHostProactivityPostSetup(AhCtx_, ProactivityResponse_->Object);
    }

    void InitWebSearchResponse() const override {
        WebSearchResponse_.ConstructInPlace(std::move(AppHostWebSearchPostSetup(AhCtx_)));
    }

    void InitWebSearchQueryResponse() const override {
        WebSearchQueryResponse_->Status = AppHostWebSearchQueryPostSetup(AhCtx_, WebSearchQueryResponse_->Object);
    }

    void InitSpeechKitSessionResponse() const override {
        SpeechKitSessionResponse_->Status = AppHostSpeechKitSessionPostSetup(AhCtx_, SpeechKitSessionResponse_->Object);
    }

private:
    IAppHostCtx& AhCtx_;
    const IContext& Ctx_;
};

class TBaseWalkerContext : public IRunWalkerRequestCtx {
public:
    TBaseWalkerContext(IAppHostCtx& ahCtx, TSpeechKitRequest skr)
        : AhCtx_{ahCtx}
        , ItemProxyAdapter_{ahCtx.ItemProxyAdapter()}
        , RequestCtx_{ahCtx}
        , Context_{skr, {}, RequestCtx_}
        , Rng_{skr.GetSeed()}
        , FactorStorage_{NMegamind::CreateFactorStorage(NMegamind::CreateFactorDomain())}
    {
        Context_.SetResponses(MakeHolder<TRunSourceResponses>(AhCtx_, Context_));
    }

    TRequestCtx& RequestCtx() override {
        return RequestCtx_;
    }
    const TRequestCtx& RequestCtx() const override {
        return RequestCtx_;
    }

    IContext& Ctx() override {
        return Context_;
    }
    const IContext& Ctx() const override {
        return Context_;
    }

    IRng& Rng() override {
        return Rng_;
    }

    TFactorStorage& FactorStorage() override {
        return FactorStorage_;
    }

    NMegamind::TItemProxyAdapter& ItemProxyAdapter() override {
        return ItemProxyAdapter_;
    };

protected:
    IAppHostCtx& AhCtx_;
    NMegamind::TItemProxyAdapter& ItemProxyAdapter_;

private:
    TAppHostRequestCtx RequestCtx_;
    TContext Context_;
    TRng Rng_;
    TFactorStorage FactorStorage_;
};

class TPreClassifyWalkerContext : public TBaseWalkerContext {
public:
    using TBaseWalkerContext::TBaseWalkerContext;

    ERunStage RunStage() const override {
        return ERunStage::PreClassification;
    }

    void MakeSearchRequest(TWebSearchRequestBuilder& builder, const IEvent& event) override {
        if (auto err = AppHostWebSearchSetup(AhCtx_, Ctx().SpeechKitRequest(), event, builder)) {
            LOG_ERR(AhCtx_.Log()) << "AppHost WebSearch prepare request failed: " << *err;
        }
    }

    void MakeProactivityRequest(const TRequest& requestModel,
                                const TScenarioToRequestFrames& scenarioToFrames,
                                const TProactivityStorage& storage) override
    {
        if (const auto err = AppHostProactivitySetup(AhCtx_, Ctx(), requestModel, storage, scenarioToFrames)) {
            LOG_ERR(AhCtx_.Log()) << "Error during preparation proactivity request: " << err;
        }
    }



    NModifiers::IModifierRequestFactory& ModifierRequestFactory() override {
        throw yexception() << "Modifiers shouldn't be called from PreClassify stage";
    }

};

class TWalkerContext : public TBaseWalkerContext {
public:
    TWalkerContext(IAppHostCtx& ahCtx, TSpeechKitRequest skr, ILightWalkerRequestCtx::ERunStage runStage)
        : TBaseWalkerContext{ahCtx, skr}
        , RunStage_{runStage}
        , PostClassifyState_{ItemProxyAdapter()}
        , AppHostModifierRequestFactory_{ItemProxyAdapter(), Ctx(), ahCtx.GlobalCtx().ScenarioConfigRegistry()} {
    }

    ERunStage RunStage() const override {
        return RunStage_;
    }

    void MakeSearchRequest(TWebSearchRequestBuilder& /* builder */, const IEvent& /* event */) override {
        // No need to make websearch request in post classify node
        return;
    }

    void MakeProactivityRequest(const TRequest& /* requestModel */,
                                const TScenarioToRequestFrames& /* frames */,
                                const NMegamind::TProactivityStorage& /* storage */) override
    {
        // No need to make proactivity request in post classify node
        return;
    }

    NModifiers::IModifierRequestFactory& ModifierRequestFactory() override {
        return AppHostModifierRequestFactory_;
    }

    void SavePostClassifyState(const TWalkerResponse& walkerResponse,
                               const NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                               TStatus postClassifyError, const TScenarioWrapperPtr winnerScenario,
                               const TRequest& request) override {
        auto& itemAdapter = ItemProxyAdapter();

        if (!walkerResponse.Scenarios.empty()) {
            const auto& topResponse = walkerResponse.Scenarios.front();
            if (const auto* responseBody = topResponse.ResponseBodyIfExists()) {
                ModifierRequestFactory().SetupModifierRequest(request, *responseBody, topResponse.GetScenarioName());
            } else {
                LOG_INFO(Ctx().Logger()) << "Can't setup modifier request because no responseBody in topResponse";
            }
            if (const auto* continueResponse = topResponse.ContinueResponseIfExists()) {
                itemAdapter.PutIntoContext(*continueResponse, NMegamind::AH_ITEM_CONTINUE_RESPONSE_POSTCLASSIFY);
            }
        }

        itemAdapter.PutIntoContext(walkerResponse.QualityStorage, NMegamind::AH_ITEM_QUALITYSTORAGE_POSTCLASSIFY);
        itemAdapter.PutIntoContext(analyticsInfoBuilder.BuildProto(), NMegamind::AH_ITEM_ANALYTICS_POSTCLASSIFY);

        if (postClassifyError.Defined()) {
            itemAdapter.PutIntoContext(NMegamind::ErrorToProto(*postClassifyError),
                                        NMegamind::AH_ITEM_ERROR_POSTCLASSIFY);
        }

        PutScenarioErrorsToApphostContext(itemAdapter, walkerResponse);

        if (winnerScenario) {
            itemAdapter.PutIntoContext(winnerScenario->GetScenarioProto(), NMegamind::AH_ITEM_WINNER_SCENARIO);
        }
    }

    void SaveCombinatorState(const TCombinatorResponse& combinatorResponse, const TRequest& request) override {
        auto& itemAdapter = ItemProxyAdapter();

        NMegamindAppHost::TCombinatorProto combinatorProto;
        combinatorProto.SetName(combinatorResponse.GetConfig().GetName());

        const auto* responseProto = combinatorResponse.ResponseProtoIfExists();
        if (!responseProto) {
            LOG_ERR(Ctx().Logger()) << "No responseProto in combinatorResponse";
        }
        if (responseProto && responseProto->HasResponse()) {
            if (responseProto->GetResponse().HasResponseBody()) {
                // Request to modifier only from ProcessCombinatorContinue walker stage
                ModifierRequestFactory().SetupModifierRequest(request, responseProto->GetResponse().GetResponseBody(),
                                                              combinatorResponse.GetConfig().GetName());
                combinatorProto.SetStage(NMegamindAppHost::TCombinatorProto::Run);
            } else if (responseProto->GetResponse().HasContinueArguments()) {
                NScenarios::TCombinatorContinueRequest combinatorContinueRequest;
                *combinatorContinueRequest.MutableContinueArguments() = responseProto->GetResponse().GetContinueArguments();
                auto continueItemName =
                    NMegamind::AH_ITEM_COMBINATOR_CONTINUE_REQUEST_PREFIX + combinatorResponse.GetConfig().GetName();
                itemAdapter.PutIntoContext(combinatorContinueRequest, continueItemName);
                combinatorProto.SetStage(NMegamindAppHost::TCombinatorProto::Continue);
            }
        }

        itemAdapter.PutIntoContext(combinatorProto, NMegamind::AH_ITEM_WINNER_COMBINATOR);
    }

    NMegamind::IPostClassifyState& PostClassifyState() override {
        return PostClassifyState_;
    }

private:
    ILightWalkerRequestCtx::ERunStage RunStage_;
    TPostClassifyState PostClassifyState_;
    NModifiers::TAppHostModifierRequestFactory AppHostModifierRequestFactory_;
};

class TWalkerFinalizeContext : public TWalkerContext {
public:
    TWalkerFinalizeContext(IAppHostCtx& ahCtx, TSpeechKitRequest skr)
        : TWalkerContext{ahCtx, skr, ILightWalkerRequestCtx::ERunStage::RunFinalize}
        , ModifierRequestFactory_{ItemProxyAdapter(), Ctx(), ahCtx.GlobalCtx().ScenarioConfigRegistry()}
    {
    }

    NModifiers::IModifierRequestFactory& ModifierRequestFactory() override {
        return ModifierRequestFactory_;
    }

private:
    NModifiers::TAppHostModifierRequestFactory ModifierRequestFactory_;
};

TStatus RestoreRequestModel(TWalkerContext& wCtx, TMaybe<TRequest>& request) {
    const auto errorOrRequest =
        wCtx.ItemProxyAdapter().GetFromContext<NMegamind::TRequestData>(NMegamind::AH_ITEM_MM_REQUEST_DATA);
    if (errorOrRequest.Error()) {
        return TError{TError::EType::Critical} << "unable to restore request";
    }
    request = CreateRequest(errorOrRequest.Value());
    return Success();
}

} // namespace

// TAppHostWalkerRunNodeHandler ---------------------------------------------------
TAppHostWalkerRunNodeHandler::TAppHostWalkerRunNodeHandler(IGlobalCtx& globalCtx, TWalkerPtr walker,
                                                           ILightWalkerRequestCtx::ERunStage runStage,
                                                           bool useAppHostStreaming)
    : TAppHostNodeHandler{globalCtx, useAppHostStreaming}
    , Walker_{walker}
    , RunStage_{runStage}
{
}

// TAppHostPostClassifyNodeHandler ----------------------------------------------
TStatus TAppHostPostClassifyNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    TAppHostHttpResponse httpResponse{ahCtx.ItemProxyAdapter()};

    auto onExecute = [&ahCtx, this]() mutable {
        TRequestTimeMetrics requestTimeMetrics{ahCtx.GlobalCtx().ServiceSensors(), ahCtx.Log(),
                                               ahCtx.ItemProxyAdapter().NodeLocation().Name,
                                               EUniproxyStage::Run, ERequestResult::Fail};

        TFromAppHostSpeechKitRequest::TPtr skrPtr;
        if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skrPtr)) {
            return err;
        }
        Y_ASSERT(skrPtr);

        TWalkerContext wCtx{ahCtx, *skrPtr, RunStage_};
        UpdateMetricsData(requestTimeMetrics, wCtx);

        ahCtx.Log().SetLogMessagesTypes(ELogMessageType::MegamindRetrieveScenarioRunResponses);
        Walker_->RunPostClassifyStage(wCtx);

        requestTimeMetrics.UpdateResult(ERequestResult::Success);

        return Success();
    };
    TResponseErrorInterceptor{httpResponse}
        .EnableSensors(ahCtx.GlobalCtx().ServiceSensors())
        .EnableLogging(ahCtx.Log())
        .SetNetLocation(ahCtx.ItemProxyAdapter().NodeLocation().Path)
        .ProcessExecutor(onExecute);

    return Success();
}

// TAppHostPreClassifyNodeHandler ----------------------------------------------
TStatus TAppHostPreClassifyNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    ahCtx.Log().SetLogMessagesTypes(ELogMessageType::MegamindPrepareScenarioRunRequests);

    TFromAppHostSpeechKitRequest::TPtr skr;
    if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skr)) {
        return std::move(*err);
    }

    Y_ASSERT(skr);

    TPreClassifyWalkerContext wCtx{ahCtx, *skr};

    Walker_->RunPreClassifyStage(wCtx);

    return Success();
}

// TAppHostWalkerRunProcessContinueHandler ----------------------------------------
TStatus TAppHostWalkerRunProcessContinueHandler::Execute(IAppHostCtx& ahCtx) const {
    TAppHostHttpResponse httpResponse{ahCtx.ItemProxyAdapter()};

    auto onExecute = [&ahCtx, this]() mutable {
        TRequestTimeMetrics requestTimeMetrics{ahCtx.GlobalCtx().ServiceSensors(), ahCtx.Log(),
                                               ahCtx.ItemProxyAdapter().NodeLocation().Name,
                                               EUniproxyStage::Run, ERequestResult::Fail};

        TFromAppHostSpeechKitRequest::TPtr skrPtr;
        if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skrPtr)) {
            return err;
        }
        Y_ASSERT(skrPtr);
        TWalkerContext wCtx{ahCtx, *skrPtr, RunStage_};
        UpdateMetricsData(requestTimeMetrics, wCtx);

        Walker_->RunProcessContinueStage(wCtx);

        requestTimeMetrics.UpdateResult(ERequestResult::Success);

        return Success();
    };
    TResponseErrorInterceptor{httpResponse}
        .EnableSensors(ahCtx.GlobalCtx().ServiceSensors())
        .EnableLogging(ahCtx.Log())
        .SetNetLocation(ahCtx.ItemProxyAdapter().NodeLocation().Path)
        .ProcessExecutor(onExecute);

    return Success();
}

// TAppHostWalkerRunFinalizeNodeHandler ----------------------------------------
TStatus TAppHostWalkerRunFinalizeNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    ahCtx.Log().SetLogMessagesTypes(ELogMessageType::MegamindRetrieveScenarioRunResponses);

    TAppHostHttpResponse httpResponse{ahCtx.ItemProxyAdapter()};

    auto onExecute = [&httpResponse, &ahCtx, this]() mutable {
        TRequestTimeMetrics requestTimeMetrics{ahCtx.GlobalCtx().ServiceSensors(), ahCtx.Log(),
                                               ahCtx.ItemProxyAdapter().NodeLocation().Name, EUniproxyStage::Run,
                                               ERequestResult::Fail};

        TFromAppHostSpeechKitRequest::TPtr skrPtr;
        if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skrPtr)) {
            return err;
        }
        Y_ASSERT(skrPtr);

        TWalkerFinalizeContext wCtx{ahCtx, *skrPtr};
        UpdateMetricsData(requestTimeMetrics, wCtx);

        TSpeechKitHandler handler{Walker_};
        const auto walkerResponse = Walker_->RunFinalizeStage(wCtx);
        TAnalyticsLogContextBuilder logContextBuilder;
        handler.HandleHttpRequest(wCtx.RequestCtx(), wCtx.Ctx(), httpResponse, walkerResponse, logContextBuilder);
        wCtx.ItemProxyAdapter().PutIntoContext(logContextBuilder.BuildProto(), AH_ITEM_ANALYTICS_LOG_CONTEXT);

        requestTimeMetrics.UpdateResult(ERequestResult::Success);

        return Success();
    };
    TResponseErrorInterceptor{httpResponse}
        .EnableSensors(ahCtx.GlobalCtx().ServiceSensors())
        .EnableLogging(ahCtx.Log())
        .SetNetLocation(ahCtx.ItemProxyAdapter().NodeLocation().Path)
        .ProcessExecutor(onExecute);

    return Success();
}

// TAppHostWalkerRunClassifyWinnerNodeHandle ----------------------------------------
TStatus TAppHostWalkerRunClassifyWinnerNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    TAppHostHttpResponse httpResponse{ahCtx.ItemProxyAdapter()};

    auto onExecute = [&ahCtx, this]() mutable {
        TRequestTimeMetrics requestTimeMetrics{ahCtx.GlobalCtx().ServiceSensors(), ahCtx.Log(),
                                               ahCtx.ItemProxyAdapter().NodeLocation().Name,
                                               EUniproxyStage::Run, ERequestResult::Fail};

        TFromAppHostSpeechKitRequest::TPtr skrPtr;
        if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skrPtr)) {
            return err;
        }
        Y_ASSERT(skrPtr);

        TWalkerContext wCtx{ahCtx, *skrPtr, RunStage_};
        UpdateMetricsData(requestTimeMetrics, wCtx);

        TMaybe<TRequest> requestModel;
        if (const auto err = RestoreRequestModel(wCtx, requestModel)) {
            return err;
        }
        Y_ASSERT(requestModel.Defined());

        Walker_->RunClassifyWinner(wCtx, requestModel.GetRef());

        requestTimeMetrics.UpdateResult(ERequestResult::Success);

        return Success();
    };
    TResponseErrorInterceptor{httpResponse}
        .EnableSensors(ahCtx.GlobalCtx().ServiceSensors())
        .EnableLogging(ahCtx.Log())
        .SetNetLocation(ahCtx.ItemProxyAdapter().NodeLocation().Path)
        .ProcessExecutor(onExecute);

    return Success();
}

// TAppHostWalkerRunProcessCombinatorContinueNodeHandler ----------------------------------------
TStatus TAppHostWalkerRunProcessCombinatorContinueNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    TAppHostHttpResponse httpResponse{ahCtx.ItemProxyAdapter()};

    auto onExecute = [&ahCtx, this]() mutable {
        TRequestTimeMetrics requestTimeMetrics{ahCtx.GlobalCtx().ServiceSensors(), ahCtx.Log(),
                                               ahCtx.ItemProxyAdapter().NodeLocation().Name,
                                               EUniproxyStage::Run, ERequestResult::Fail};

        TFromAppHostSpeechKitRequest::TPtr skrPtr;
        if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skrPtr)) {
            return err;
        }
        Y_ASSERT(skrPtr);

        TWalkerContext wCtx{ahCtx, *skrPtr, RunStage_};
        UpdateMetricsData(requestTimeMetrics, wCtx);

        TMaybe<TRequest> requestModel;
        if (const auto err = RestoreRequestModel(wCtx, requestModel)) {
            return err;
        }
        Y_ASSERT(requestModel.Defined());

        Walker_->RunProcessCombinatorContinue(wCtx, requestModel.GetRef());

        requestTimeMetrics.UpdateResult(ERequestResult::Success);

        return Success();
    };
    TResponseErrorInterceptor{httpResponse}
        .EnableSensors(ahCtx.GlobalCtx().ServiceSensors())
        .EnableLogging(ahCtx.Log())
        .SetNetLocation(ahCtx.ItemProxyAdapter().NodeLocation().Path)
        .ProcessExecutor(onExecute);

    return Success();
}

} // namespace NAlice::NMegamind
