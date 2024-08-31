#include "info_cv_handler.h"

#include <alice/bass/libs/metrics/metrics.h>

using namespace NBASS;

TCVAnswerInfo::TCVAnswerInfo()
        : IComputerVisionAnswer(TComputerVisionEllipsisInfoHandler::FormShortName(),
                                TStringBuf("https://avatars.mds.yandex.net/get-images-similar-mturk/15681/icon-info/orig"))
{
}

bool TCVAnswerInfo::TryApplyTo(TComputerVisionContext& ctx, bool force, bool /*shouldAttachAlternativeIntents*/) const {
    if (!force || !ctx.HasCbirId()) {
        return false;
    }
    Compose(ctx);
    return true;
}

bool TCVAnswerInfo::IsSuitable(const TComputerVisionContext& ctx, bool force) const {
    return force && ctx.HasCbirId();
}

void TCVAnswerInfo::Compose(TComputerVisionContext& ctx) const {
    ctx.RedirectTo("", "imageview", /* disable ptr */ true);
    Y_STATS_INC_COUNTER("bass_computer_vision_result_info_success");
    ctx.HandlerContext().AddOnboardingSuggest();
}

TComputerVisionEllipsisInfoHandler::TComputerVisionEllipsisInfoHandler()
        : TComputerVisionMainHandler(ECaptureMode::PHOTO, false)
{
}

void TComputerVisionEllipsisInfoHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionEllipsisInfoHandler>();
    };
    handlers->emplace(TComputerVisionEllipsisInfoHandler::FormName(), handler);
}

TResultValue TComputerVisionEllipsisInfoHandler::WrappedDo(TComputerVisionContext& cvContext) const {
    if (!AnswerInfo.TryApplyTo(cvContext, true)) {
        cvContext.Output("code").SetString("info_form_error");
        Y_STATS_INC_COUNTER("bass_computer_vision_result_info_error_form");
    }
    return TResultValue();
}
