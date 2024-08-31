#include "museum.h"

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_museum";
    constexpr TStringBuf SHORT_ANSWER_NAME = "museum";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_museum";
}

namespace NAlice::NHollywood::NImage::NAnswers {

    TMuseum::TMuseum(): IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
    {
        AllowedIntents = {
            NImages::NCbir::ECbirIntents::CI_SIMILAR,
            NImages::NCbir::ECbirIntents::CI_CLOTHES,
            NImages::NCbir::ECbirIntents::CI_MARKET,
            NImages::NCbir::ECbirIntents::CI_OCR,
            NImages::NCbir::ECbirIntents::CI_OCR_VOICE
        };
        LastForceAlternativeSuggest = {
            NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
            NImages::NCbir::ECbirIntents::CI_INFO
        };
    }

    bool TMuseum::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool) const {
        const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
        if (!imageAliceResponseMaybe.Defined()) {
            return false;
        }

        const NSc::TValue& imageAliceResponse = imageAliceResponseMaybe.GetRef();
        const NSc::TValue& museumPromo = imageAliceResponse.TrySelect(PROTMO_PATH);
        return !museumPromo.IsNull() && !museumPromo.DictEmpty();
    }

    void TMuseum::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
        static const TMap<TString, TString> fieldsMap = {{"AliceTitle", "card_title"},
                                                        {"AliceText",  "card_text"},
                                                        {"SourceDescription",  "card_footer"},
                                                        {"HasAudio", "has_audio"}};
        const NSc::TValue& museumPromo = ctx.GetImageAliceResponse().GetRef().TrySelect(PROTMO_PATH);
        NSc::TValue museumCard;
        for (const auto& fieldMap : fieldsMap) {
            museumCard[fieldMap.second] = museumPromo[fieldMap.first];
        }
        museumCard["img"]["src"] = museumPromo["ImagePreview"];
        museumCard["img"]["w"] = IMAGE_SIZE.first;
        museumCard["img"]["h"] = IMAGE_SIZE.second;
        museumCard["card_url"].SetString(museumPromo["AliceLink"]);
        ctx.AddDivCardBlock(TStringBuf("museum_object"), std::move(museumCard));
        NSc::TValue textCardData;
        textCardData["tts"] = museumPromo["AliceVoice"];
        ctx.AddTextCard("render_museum", textCardData);
        ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::TAG_WRONG);

        ctx.GetAnalyticsInfoBuilder().AddObject("museum", "museum", TString(museumPromo["AliceTitle"].GetString()));
    }

    TMuseum* TMuseum::GetPtr() {
        static TMuseum* answer = new TMuseum;
        return answer;
    }

}
