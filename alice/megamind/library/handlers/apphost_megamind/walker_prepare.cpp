#include "walker_prepare.h"

#include "begemot.h"
#include "blackbox.h"
#include "components.h"
#include "misspell.h"
#include "node.h"
#include "personal_intents.h"
#include "query_tokens_stats.h"
#include "responses.h"
#include "saas.h"
#include "speechkit_session.h"
#include "websearch_query.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/response.h>
#include <alice/megamind/library/apphost_request/protos/preclassify.pb.h>
#include <alice/megamind/library/apphost_request/protos/scenario.pb.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/factor_storage/factor_storage.h>
#include <alice/megamind/library/handlers/speechkit/error_interceptor.h>
#include <alice/megamind/library/handlers/speechkit/speechkit.h>
#include <alice/megamind/library/handlers/utils/sensors.h>
#include <alice/megamind/library/requestctx/common.h>
#include <alice/megamind/library/search/request.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/walker/requestctx.h>
#include <alice/megamind/library/walker/walker.h>
#include <alice/megamind/library/worldwide/language/is_alice_worldwide_language.h>

#include <kernel/factor_storage/factor_storage.h>

#include <util/stream/str.h>

namespace NAlice::NMegamind {
namespace {

class TSourceResponses final : public TAppHostSourceResponses {
public:
    TSourceResponses(IAppHostCtx& ahCtx, const IContext& ctx)
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
        SaasSkillDiscoveryResponse_->Status =
            AppHostSaasSkillDiscoveryPostSetup(AhCtx_, SaasSkillDiscoveryResponse_->Object);
    }

    void InitBegemotResponseRewrittenRequestResponse() const override {
        BegemotResponseRewrittenRequestResponse_->Status =
            AppHostBegemotResponseRewrittenRequestPostSetup(AhCtx_, BegemotResponseRewrittenRequestResponse_->Object);
    }

    void InitSpeechKitSessionResponse() const override {
        SpeechKitSessionResponse_->Status = AppHostSpeechKitSessionPostSetup(AhCtx_, SpeechKitSessionResponse_->Object);
    }

private:
    IAppHostCtx& AhCtx_;
    const IContext& Ctx_;
};

class TWalkerContext : public IRunWalkerRequestCtx {
public:
    TWalkerContext(IAppHostCtx& ahCtx, TSpeechKitRequest skr)
        : AhCtx_{ahCtx}
        , ItemProxyAdapter_{ahCtx.ItemProxyAdapter()}
        , RequestCtx_{ahCtx}
        , Context_{skr, {}, RequestCtx_}
        , Rng_{skr.GetSeed()}
        , FactorStorage_{NMegamind::CreateFactorStorage(NMegamind::CreateFactorDomain())}
    {
        Context_.SetResponses(MakeHolder<TSourceResponses>(AhCtx_, Context_));
    }

    NModifiers::IModifierRequestFactory& ModifierRequestFactory() override {
        throw yexception() << "Modifiers should not be called from prepare node";
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
    }

    ERunStage RunStage() const override {
        return ERunStage::Prepare;
    }

    void MakeSearchRequest(TWebSearchRequestBuilder& /* builder */, const IEvent& /* event */) override
    {
        // Do nothing in prepare preclassify.
        return;
    }

    void MakeProactivityRequest(const TRequest& /* requestModel */,
                                const TScenarioToRequestFrames& /* scenarioToFrames */,
                                const TProactivityStorage& /* storage */) override
    {
        // Do nothing in prepare preclassify.
        return;
    }

private:
    IAppHostCtx& AhCtx_;
    NMegamind::TItemProxyAdapter& ItemProxyAdapter_;
    TAppHostRequestCtx RequestCtx_;
    TContext Context_;
    TRng Rng_;
    TFactorStorage FactorStorage_;
};

} // namespace


TAppHostWalkerPrepareNodeHandler::TAppHostWalkerPrepareNodeHandler(IGlobalCtx& globalCtx, TWalkerPtr walker)
    : TAppHostNodeHandler{globalCtx, /* useAppHostStreaming= */ false}
    , Walker_{walker}
{
}

TStatus TAppHostWalkerPrepareNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    ahCtx.Log().SetLogMessagesTypes(ELogMessageType::MegamindPreClasification);
    TAppHostHttpResponse httpResponse{ahCtx.ItemProxyAdapter()};

    auto onExecute = [&ahCtx, this]() mutable {
        TFromAppHostSpeechKitRequest::TPtr skr;
        if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skr)) {
            return err;
        }
        Y_ASSERT(skr);

        TRequestTimeMetrics requestTimeMetrics{ahCtx.GlobalCtx().ServiceSensors(), ahCtx.Log(),
                                               ahCtx.ItemProxyAdapter().NodeLocation().Name,
                                               EUniproxyStage::Run, ERequestResult::Fail};

        TWalkerContext wCtx{ahCtx, *skr};

        IScenarioWalker::TRunState runState;
        TMaybe<IScenarioWalker::TRequestState> requestState;
        TMaybe<IScenarioWalker::TPreClassifyState> preClassifyState;
        if (const auto e = Walker_->RunPrepare(wCtx, runState, requestState, preClassifyState)) {
            return e;
        }

        // Emplace scenarios into context.
        auto& itemAdapter = wCtx.ItemProxyAdapter();
        for (const auto& scenario : preClassifyState->ScenarioWrappers) {
            auto proto = scenario->GetScenarioProto();
            itemAdapter.PutIntoContext(proto, AH_ITEM_SCENARIO);
        }

        // Emplace QualityStorage into context.
        itemAdapter.PutIntoContext(preClassifyState->QualityStorage, AH_ITEM_QUALITYSTORAGE);

        // Emplace FactorStorage into context.
        NMegamindAppHost::TFactorStorageBinaryProto factorStorage;
        TStringOutput strStream{*factorStorage.MutableFactorStorage()};
        NFSSaveLoad::Serialize(wCtx.FactorStorage(), &strStream);
        itemAdapter.PutIntoContext(factorStorage, AH_ITEM_FACTORSTORAGE_BINARY);

        // Fill and emplace PreClassify proto into context.
        NMegamindAppHost::TPreClassifyProto preClassifyProto;
        preClassifyProto.SetIsTrashPartial(preClassifyState->IsTrashPartial);
        itemAdapter.PutIntoContext(preClassifyProto, AH_ITEM_PRECLASSIFY);

        if (requestState.Defined()) {
            itemAdapter.PutIntoContext(requestState->Request.ToProto(), NMegamind::AH_ITEM_MM_REQUEST_DATA);
            itemAdapter.PutIntoContext(runState.AnalyticsInfoBuilder.BuildProto(),
                                        NMegamind::AH_ITEM_MM_RUN_STATE_ANALYTICS_WALKER_PREPARE);
        }

        requestTimeMetrics.UpdateResult(ERequestResult::Success);

        AppHostWebSearchQuerySetup(ahCtx, wCtx.Ctx(), requestState->Request.GetEvent());
        AppHostBegemotResponseRewrittenRequestSetup(ahCtx, wCtx.Ctx().Responses().WizardResponse().GetRewrittenRequest());
        return Success();
    };
    TResponseErrorInterceptor{httpResponse}
        .EnableSensors(ahCtx.GlobalCtx().ServiceSensors())
        .EnableLogging(ahCtx.Log())
        .SetNetLocation(ToString(ahCtx.ItemProxyAdapter().NodeLocation().Path))
        .ProcessExecutor(onExecute);

    return Success();
}

} // namespace NAlice::NMegamind
