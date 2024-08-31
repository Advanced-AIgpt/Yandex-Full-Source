#include "similarlike_cv_handler.h"

#include <alice/bass/libs/metrics/metrics.h>

using namespace NBASS;

TCVAnswerSimilarLike::TCVAnswerSimilarLike()
        : IComputerVisionAnswer(TComputerVisionEllipsisSimilarLikeHandler::FormShortName(),
                                TStringBuf("https://avatars.mds.yandex.net/get-images-similar-mturk/40186/icon-similar/orig"))
{
}

bool TCVAnswerSimilarLike::TryApplyTo(TComputerVisionContext& ctx, bool force, bool /*shouldAttachAlternativeIntents*/) const {
    ctx.TryFillLastForceAnswer(AnswerType());
    if (!force || !ctx.HasCbirId()) {
        return false;
    }
    Compose(ctx);
    return true;
}

bool TCVAnswerSimilarLike::IsSuitable(const TComputerVisionContext& ctx, bool force) const {
    return force && ctx.HasCbirId();
}

void TCVAnswerSimilarLike::Compose(TComputerVisionContext& ctx) const {
    ctx.RedirectTo("", "imagelike");
    Y_STATS_INC_COUNTER("bass_computer_vision_result_ellipsis_similar_success");
    ctx.AttachTextCard();
    ctx.Output("similar_ellipsis").SetBool(true);
    ctx.HandlerContext().AddOnboardingSuggest();
}

TComputerVisionEllipsisSimilarLikeHandler::TComputerVisionEllipsisSimilarLikeHandler()
    : TComputerVisionMainHandler(ECaptureMode::SIMILAR_LIKE)
{
}

void TComputerVisionEllipsisSimilarLikeHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionEllipsisSimilarLikeHandler>();
    };
    handlers->emplace(TComputerVisionEllipsisSimilarLikeHandler::FormName(), handler);
}

bool TComputerVisionEllipsisSimilarLikeHandler::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    cvContext.Switch(NComputerVisionForms::IMAGE_WHAT_IS_THIS);
    if (!AnswerLike.TryApplyTo(cvContext, true)) {
        cvContext.Output("code").SetString("similar_form_error");
        Y_STATS_INC_COUNTER("bass_computer_vision_result_ellipsis_similar_error_form");
        return false;
    }
    return true;
}
