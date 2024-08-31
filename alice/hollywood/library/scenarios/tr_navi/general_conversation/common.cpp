#include "common.h"

namespace NAlice::NHollywood::NTrNavi {

void PrepareResponseWithPhrase(TResponseBodyBuilder& responseBodyBuilder, TRTLogger& logger,
                               const TScenarioRunRequestWrapper& requestWrapper, TStringBuf phraseName) {
    TNlgData nlgData{logger, requestWrapper};
    responseBodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_GENERAL_CONVERSATION, phraseName, /* buttons = */ {}, nlgData);
    const auto intentName = TStringBuilder{} << GENERAL_CONVERSATION_FRAME << "." << phraseName;
    auto& analyticsInfoBuilder = responseBodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetIntentName(intentName);
}

} // namespace NAlice::NHollywood::NTrNavi