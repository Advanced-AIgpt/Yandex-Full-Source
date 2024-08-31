#include "on_utterance.h"

#include "begemot.h"
#include "components.h"
#include "misspell.h"
#include "polyglot.h"
#include "query_tokens_stats.h"
#include "responses.h"
#include "saas.h"
#include "speechkit_session.h"

#include <alice/megamind/library/request_composite/composite.h>
#include <alice/megamind/library/vins/wizard.h>

#include <alice/library/experiments/experiments.h>

namespace NAlice::NMegamind {

namespace {

TStatus RunAppHostPolyglotBegemotSetup(IAppHostCtx& ahCtx, const IContext& ctx) {
    if (ctx.HasExpFlag(EXP_DISABLE_POLYGLOT_BEGEMOT)) {
        return Success();
    }

    TString originalUtterance;
    auto result = GetUtteranceFromEvent(ctx.SpeechKitRequest(), originalUtterance);
    if (!result.IsSuccess()) {
        return result.Status();
    }
    if (result.Value() == ESourcePrepareType::Succeeded) {
        if (auto err = AppHostPolyglotBegemotSetup(ahCtx, originalUtterance, ctx)) {
            return std::move(*err);
        }
    }
    return Success();
}

class TSourceResponses final : public TAppHostSourceResponses {
public:
    TSourceResponses(IAppHostCtx& ahCtx)
        : AhCtx_{ahCtx}
    {
    }

private:
    void InitSpeechKitSessionResponse() const override {
        SpeechKitSessionResponse_->Status = AppHostSpeechKitSessionPostSetup(AhCtx_, SpeechKitSessionResponse_->Object);
    }

private:
    IAppHostCtx& AhCtx_;
};

} // namespace

TStatus TAppHostUtterancePostSetupNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    TAppHostRequestCtx ahRequestCtx(ahCtx);
    TFromAppHostSpeechKitRequest::TPtr skr;
    if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skr)) {
        return std::move(*err);
    }

    const TContext ctx{*skr, MakeHolder<TSourceResponses>(ahCtx), ahRequestCtx};

    TMaybe<TString> utterance;
    TPolyglotTranslateUtteranceResponse polyglotTranslateUtteranceResponse;
    if (auto err = AppHostPolyglotPostSetup(ahCtx, polyglotTranslateUtteranceResponse)) {
        LOG_WARN(ahCtx.Log()) << "Polyglot translation of utterance not found: " << err;
    } else {
        utterance = polyglotTranslateUtteranceResponse.GetTranslatedUtterance();
        if (const auto err = RunAppHostPolyglotBegemotSetup(ahCtx, ctx)) {
            LOG_WARN(ahCtx.Log()) << "POLYGLOT_BEGEMOT setup failed: " << err;
        }
    }

    if (!utterance.Defined()) {
        if (auto err = AppHostMisspellPostSetup(ahCtx, utterance)) {
            LOG_WARN(ahCtx.Log()) << "Fetch of misspell response failed: " << err;
        }
    }

    if (!utterance.Defined() && !ctx.PolyglotUtterance().Empty()) {
        utterance = ctx.PolyglotUtterance();
    }

    if (!utterance.Defined()) {
        return TError{TError::EType::DataError}
               << "Parsing of misspell response failed. Look at logs to get more information";
    }

    return AppHostOnUtteranceReadySetup(ahCtx, *utterance, ctx);
}

void RegisterAppHostUtteranceHandlers(IGlobalCtx& globalCtx, TRegistry& registry) {
    static const TAppHostUtterancePostSetupNodeHandler utterancePostSetupHandler{globalCtx, /* useAppHostStreaming= */ false};
    registry.Add("/mm_utterance_post_setup",
                 [](NAppHost::IServiceContext& ctx) { utterancePostSetupHandler.RunSync(ctx); });
}

TStatus AppHostOnUtteranceReadySetup(IAppHostCtx& ahCtx, TString utterance, const IContext& ctx) {
    NVins::NWizard::DoClipNormalization(utterance);
    if (utterance.Empty()) {
        return Success();
    }

    if (auto err = AppHostQueryTokensStatsSetup(ahCtx, utterance, ctx.SpeechKitRequest())) {
        return std::move(*err);
    }
    if (auto err = AppHostSaasSkillDiscoverySetup(ahCtx, utterance)) {
        return std::move(*err);
    }

    return AppHostBegemotSetup(ahCtx, utterance, ctx);
}

} // namespace NAlice::NMegamind
