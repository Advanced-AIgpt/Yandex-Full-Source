#include "error_strategy.h"

#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>

#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NGeneralConversation {

TErrorRenderStrategy::TErrorRenderStrategy(TGeneralConversationRunContextWrapper& contextWrapper, TStringBuf errorType)
    : ContextWrapper_(contextWrapper)
    , ErrorType_(errorType)
{
    LOG_INFO(ContextWrapper_.Logger()) << "ReplySourceRenderStrategy: ErrorRenderStrategy, errorType: " << errorType;
}

void TErrorRenderStrategy::AddResponse(TGeneralConversationResponseWrapper* responseWrapper)    {
    Y_ENSURE(!ContextWrapper_.RequestWrapper().HasExpFlag(EXP_HW_GC_DEBUG_RENDER_TIMEOUT_EXCEPTION));

    TNlgData nlgData{ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper()};
    nlgData.Context["type"] = ErrorType_;
    responseWrapper->Builder.GetResponseBodyBuilder()->AddRenderedTextWithButtonsAndVoice(GENERAL_CONVERSATION_SCENARIO_NAME, NLG_RENDER_ERROR, /* buttons = */ {}, nlgData);
    responseWrapper->GcResponseInfo.SetSource(ErrorType_);
}

} // namespace NAlice::NHollywood::NGeneralConversation
