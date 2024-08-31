#include "walker_apply.h"

#include "begemot.h"
#include "blackbox.h"
#include "components.h"
#include "misspell.h"
#include "node.h"
#include "personal_intents.h"
#include "polyglot.h"
#include "speechkit_session.h"
#include "query_tokens_stats.h"
#include "responses.h"
#include "walker_util.h"

#include <alice/megamind/library/apphost_request/response.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/handlers/utils/sensors.h>
#include <alice/megamind/library/handlers/speechkit/error_interceptor.h>
#include <alice/megamind/library/handlers/speechkit/speechkit.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/walker/walker.h>
#include <alice/megamind/library/worldwide/language/is_alice_worldwide_language.h>

namespace NAlice::NMegamind {
namespace {

class TApplySourceResponses final : public TAppHostSourceResponses {
public:
    explicit TApplySourceResponses(IAppHostCtx& ahCtx, const IContext& ctx)
        : AhCtx_{ahCtx}
        , Ctx_{ctx}
    {
    }

private:
    void InitBlackBoxResponse() const override {
        BlackBoxResponse_->Status = AppHostBlackBoxPostSetup(AhCtx_, BlackBoxResponse_->Object);
    }

    void InitPolyglotTranslateUtteranceResponse() const override {
        if (IsAliceWorldWideLanguage(Ctx_.Language())) {
            PolyglotTranslateUtteranceResponse_->Status = AppHostPolyglotPostSetup(AhCtx_, PolyglotTranslateUtteranceResponse_->Object);
        }
    }

    void InitSpeechKitSessionResponse() const override {
        SpeechKitSessionResponse_->Status = AppHostSpeechKitSessionPostSetup(AhCtx_, SpeechKitSessionResponse_->Object);
    }

private:
    IAppHostCtx& AhCtx_;
    const IContext& Ctx_;
};

class TWalkerApplyContext : public ILightWalkerRequestCtx {
public:
    TWalkerApplyContext(IAppHostCtx& ahCtx, TSpeechKitRequest skr, ERunStage runStage)
        : AhCtx_{ahCtx}
        , RequestCtx_{ahCtx}
        , Context_{skr, {}, RequestCtx_}
        , Rng_{skr.GetSeed()}
        , RunStage_{runStage}
        , ItemProxyAdapter_{ahCtx.ItemProxyAdapter()}
        , AppHostModifierRequestFactory_{
                ItemProxyAdapter_,
                Ctx(),
                ahCtx.GlobalCtx().ScenarioConfigRegistry()
            }
    {
        Context_.SetResponses(MakeHolder<TApplySourceResponses>(AhCtx_, Context_));
    }

    NModifiers::IModifierRequestFactory& ModifierRequestFactory() override {
        return AppHostModifierRequestFactory_;
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

    ERunStage RunStage() const override {
        return RunStage_;
    }

    NMegamind::TItemProxyAdapter& ItemProxyAdapter() override {
        return ItemProxyAdapter_;
    }

protected:
    IAppHostCtx& AhCtx_;

private:
    TAppHostRequestCtx RequestCtx_;
    TContext Context_;
    TRng Rng_;
    const ERunStage RunStage_;
    NMegamind::TItemProxyAdapter& ItemProxyAdapter_;
    NModifiers::TAppHostModifierRequestFactory AppHostModifierRequestFactory_;
};

} // namespace

// TAppHostWalkerApplyNodeHandler ----------------------------------------------
TAppHostWalkerApplyNodeHandler::TAppHostWalkerApplyNodeHandler(
    IGlobalCtx& globalCtx,
    TWalkerPtr walker,
    ILightWalkerRequestCtx::ERunStage runStage,
    bool useAppHostStreaming)
    : TAppHostNodeHandler{globalCtx, useAppHostStreaming}
    , Walker_{walker}
    , RunStage_{runStage}
{
}

TStatus TAppHostWalkerApplyNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    TAppHostHttpResponse httpResponse{ahCtx.ItemProxyAdapter()};
    auto onExecute = [&httpResponse, &ahCtx, this]() mutable -> TStatus {
        TRequestTimeMetrics requestTimeMetrics{ahCtx.GlobalCtx().ServiceSensors(), ahCtx.Log(),
                                               ahCtx.ItemProxyAdapter().NodeLocation().Name,
                                               EUniproxyStage::Apply, ERequestResult::Fail};

        TFromAppHostSpeechKitRequest::TPtr skr;
        if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skr)) {
            return std::move(*err);
        }
        Y_ASSERT(skr);

        TWalkerApplyContext wCtx{ahCtx, *skr, RunStage_};
        UpdateMetricsData(requestTimeMetrics, wCtx);
        const auto walkerResponse = Walker_->ApplySideEffects(wCtx);
        if (RunStage_ == ILightWalkerRequestCtx::ERunStage::ApplyRenderScenario) {
            TSpeechKitHandler handler{Walker_};
            TAnalyticsLogContextBuilder logContextBuilder;
            handler.HandleHttpRequest(wCtx.RequestCtx(), wCtx.Ctx(), httpResponse, walkerResponse, logContextBuilder);
            wCtx.ItemProxyAdapter().PutIntoContext(logContextBuilder.BuildProto(), AH_ITEM_ANALYTICS_LOG_CONTEXT);
        }

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

// TAppHostWalkerApplyModifiersNodeHandler ----------------------------------------------
NAlice::NMegamind::TAppHostWalkerApplyModifiersNodeHandler::TAppHostWalkerApplyModifiersNodeHandler(
    IGlobalCtx& globalCtx, TWalkerPtr walker)
    : TAppHostNodeHandler{globalCtx, /* useAppHostStreaming= */ false}
    , Walker_{walker}
{
}

TStatus TAppHostWalkerApplyModifiersNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    TAppHostHttpResponse httpResponse{ahCtx.ItemProxyAdapter()};
    auto onExecute = [&ahCtx, this]() mutable -> TStatus {
        TRequestTimeMetrics requestTimeMetrics{ahCtx.GlobalCtx().ServiceSensors(), ahCtx.Log(),
                                               ahCtx.ItemProxyAdapter().NodeLocation().Name, EUniproxyStage::Apply,
                                               ERequestResult::Fail};

        TFromAppHostSpeechKitRequest::TPtr skr;
        if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skr)) {
            return std::move(*err);
        }
        Y_ASSERT(skr);

        TWalkerApplyContext wCtx{ahCtx, *skr, ILightWalkerRequestCtx::ERunStage::ModifyApplyScenario};
        UpdateMetricsData(requestTimeMetrics, wCtx);

        if (const auto status = Walker_->ModifyApplyScenarioResponse(wCtx, Walker_->RestoreApplyState(wCtx))) {
            LOG_ERROR(wCtx.Ctx().Logger()) << "Modify apply scenario response error: " << *status;
        }
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
