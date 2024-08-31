#include "aggregated_strategy.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/search_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/render_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/aggregated_reply_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>

#include <alice/hollywood/library/gif_card/gif_card.h>

#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

void UpdateFactsCrosspromoState(const TClassificationResult& classificationResult, const TReplyInfo& replyInfo, TSessionState* sessionState) {
    if (!replyInfo.HasFactsCrossPromoInfo()) {
        return;
    }
    sessionState->SetLastFactsCrosspromoRequestServerTimeMs(classificationResult.GetCurrentRequestServerTimeMs());
    sessionState->AddFactsCrosspromoHashHistory(THash<TString>{}(replyInfo.GetFactsCrossPromoInfo().GetFactsCrossPromoText()));

    const size_t factsHistorySize = sessionState->GetFactsCrosspromoHashHistory().size();
    if (factsHistorySize > MAX_FACTS_HISTORY_SIZE) {
        const size_t oldFactsIndex = factsHistorySize - MAX_FACTS_HISTORY_SIZE;
        auto& history = *sessionState->MutableFactsCrosspromoHashHistory();
        history.erase(history.begin(), history.begin() + oldFactsIndex);
    }
}

} // namespace

TAggregatedRenderStrategy::TAggregatedRenderStrategy(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TReplyInfo& replyInfo)
    : ContextWrapper_(contextWrapper)
    , ClassificationResult_(classificationResult)
    , ReplyInfo_(replyInfo)
{
    Y_ENSURE(replyInfo.GetReplySourceCase() == TReplyInfo::ReplySourceCase::kAggregatedReply);
    LOG_INFO(ContextWrapper_.Logger()) << "ReplySourceRenderStrategy: TAggregatedRenderStrategy";
}

void TAggregatedRenderStrategy::AddResponse(TGeneralConversationResponseWrapper* responseWrapper) {
    auto* responseBodyBuilder = responseWrapper->Builder.GetResponseBodyBuilder();

    TNlgData nlgData{ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper()};
    nlgData.Context["text"] = ReplyInfo_.GetRenderedText();
    const auto& clientInfo = ContextWrapper_.RequestWrapper().ClientInfo();
    bool isClientSupportEmoji = clientInfo.IsSearchApp() || clientInfo.IsYaBrowserMobile() || clientInfo.IsYaLauncher();
    const auto& emoji = ReplyInfo_.GetGifsAndEmojiInfo().GetEmoji();
    if (isClientSupportEmoji && emoji) {
        nlgData.Context["emoji"] = emoji;
    }
    if (emoji && !ContextWrapper_.RequestWrapper().HasExpFlag(EXP_HW_DISABLE_EMOTIONAL_TTS_CLASSIFIER)) {
        nlgData.Context["emotions_enabled"] = true;
        nlgData.Context["tts_emoji"] = emoji;
    }
    if (ReplyInfo_.HasFactsCrossPromoInfo()) {
        nlgData.Context["facts_crosspromo_text"] = ReplyInfo_.GetFactsCrossPromoInfo().GetFactsCrossPromoText();
        nlgData.Context["facts_crosspromo_period"] = ReplyInfo_.GetFactsCrossPromoInfo().GetFactsCrossPromoPeriod();
    }
    if (ReplyInfo_.GetTtsSpeed() != 1.0) {
        nlgData.Context["tts_speed"] = ReplyInfo_.GetTtsSpeed();
    }

    responseBodyBuilder->AddRenderedTextWithButtonsAndVoice(GENERAL_CONVERSATION_SCENARIO_NAME, NLG_RENDER_RESULT, /* buttons = */ {}, nlgData);
    if (ReplyInfo_.GetGifsAndEmojiInfo().HasGif()) {
        RenderGifCard(ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper(), ReplyInfo_.GetGifsAndEmojiInfo().GetGif(), *responseBodyBuilder, GENERAL_CONVERSATION_SCENARIO_NAME);
    }
    const auto& baseRequest = ContextWrapper_.RequestWrapper().Proto().GetBaseRequest();
    if (baseRequest.GetInterfaces().GetHasLedDisplay()) {
        if (const auto* emotionalImages = ContextWrapper_.Resources().GetEmotionalLedImages().FindPtr(ReplyInfo_.GetGifsAndEmojiInfo().GetEmoji())) {
            auto& rng = ContextWrapper_.Rng();
            AddFrontalLedImage({(*emotionalImages)[rng.RandomInteger() % emotionalImages->size()]}, responseBodyBuilder);
        }
    }
    if (baseRequest.GetInterfaces().GetHasScledDisplay()) {
        AddScledEyes(responseBodyBuilder);
    }


    UpdateUsedRepliesState(ContextWrapper_.RequestWrapper(), GetAggregatedReplyText(ReplyInfo_.GetAggregatedReply()), &responseWrapper->SessionState);
    UpdateFactsCrosspromoState(ClassificationResult_, ReplyInfo_, &responseWrapper->SessionState);

    responseWrapper->GcResponseInfo.SetSource(GetAggregatedReplySource(ReplyInfo_.GetAggregatedReply()));

    if (FindPtr(SOURCES_ENTITY, GetAggregatedReplySource(ReplyInfo_.GetAggregatedReply()))) {
        UpdateLastDiscussion(ClassificationResult_, &responseWrapper->SessionState);
    }

    responseWrapper->GcResponseInfo.SetHasGifCard(ReplyInfo_.GetGifsAndEmojiInfo().HasGif());
    if (ReplyInfo_.HasFactsCrossPromoInfo()) {
        auto& crosspromoInfo = *responseWrapper->GcResponseInfo.MutableFactsCrossPromoInfo();
        crosspromoInfo.SetEntityKey(ReplyInfo_.GetFactsCrossPromoInfo().GetFactsCrossPromoEntityKey());
    }
}

} // namespace NAlice::NHollywood::NGeneralConversation
