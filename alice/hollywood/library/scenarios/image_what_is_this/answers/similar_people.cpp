#include "similar_people.h"
#include "similars.h"

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_similar_people";
    constexpr TStringBuf SHORT_ANSWER_NAME = "similar_people";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_similar_people";

    constexpr TStringBuf ANSWER_NAME_FRONTAL = "alice.image_what_is_this_frontal_similar_people";
    constexpr TStringBuf SHORT_ANSWER_NAME_FRONTAL = "frontal_similar_people";
    constexpr TStringBuf DISABLE_FLAG_FRONTAL = "alice.disable_image_what_is_this_frontal_similar_people";
}

namespace NAlice::NHollywood::NImage::NAnswers {

    TSimilarPeopleCommon::TSimilarPeopleCommon(const TStringBuf answerName, const TStringBuf shortAnswerName,
                                               const TStringBuf disableFlag)
        : IAnswer(answerName, shortAnswerName, disableFlag)
    {
        AllowedIntents = {
            ::NImages::NCbir::ECbirIntents::CI_SIMILAR,
            ::NImages::NCbir::ECbirIntents::CI_CLOTHES,
            ::NImages::NCbir::ECbirIntents::CI_MARKET,
            ::NImages::NCbir::ECbirIntents::CI_OCR,
            ::NImages::NCbir::ECbirIntents::CI_OCR_VOICE,
        };
        FirstForceAlternativeSuggest = {
            ::NImages::NCbir::ECbirIntents::CI_SIMILAR_ARTWORK,
        };
        LastForceAlternativeSuggest = {
            ::NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
            ::NImages::NCbir::ECbirIntents::CI_INFO,
        };

        IsSwitchableTo = false;
    }

    void TSimilarPeopleCommon::MakeRequests(TImageWhatIsThisApplyContext& ctx) const {
        if (!ctx.GetImageAliceResponse().Defined()) {
            const TVector<std::pair<TStringBuf, TStringBuf>> params = {{"flag", "selebrity_filter"}};
            ctx.AddImageAliceRequest(params);
        }
    }

    bool TSimilarPeopleCommon::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool) const {
        const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
        if (!imageAliceResponseMaybe.Defined()) {
            return false;
        }

        const NSc::TValue& imageAliceResponse = imageAliceResponseMaybe.GetRef();
        return imageAliceResponse["SimilarPeople"].ArraySize() > 0;
    }

    void TSimilarPeopleCommon::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
        const auto& similarPeople = ctx.GetImageAliceResponse().GetRef()["SimilarPeople"][0];

        NSc::TValue textCard;
        textCard["name"] = similarPeople["name"];
        ctx.AddTextCard("render_similar_people_result", textCard);

        NSc::TValue card;
        card["source_url"] = similarPeople["url"];
        card["title"] = similarPeople["title"];
        card["text"] = similarPeople["text"];
        card["image"] = similarPeople["image"];
        card["image_url"] = similarPeople["image_url"].GetString();
        card["similarity"] = std::lround(similarPeople["similarity"].GetNumber() * 100);
        card["search_url"] = ctx.GenerateSearchUri(similarPeople["name"]);
        card["source_image"] = ctx.GetImageUrl();
        ctx.AddDivCardBlock("similar_people", std::move(card));
        TSimilars* similarAnswer = TSimilars::GetPtr();
        similarAnswer->AddSimilarsGallery(ctx);

        const TString actionId = ctx.AddAction(TString(GetAnswerName()));
        ctx.AddActionSuggest(TString(GetAnswerName()) + "_again", actionId);

        ctx.GetAnalyticsInfoBuilder().AddObject("similar_people", "similar_people", TString(similarPeople["title"].GetString()));
    }

    bool TSimilarPeopleCommon::RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const {
        if (ctx.GetImageAliceResponse().Defined()) {
            ctx.AddTextCard("render_cannot_apply_similar_people_error", {});
            return true;
        }
        return false;
    }

    TSimilarPeople::TSimilarPeople() : TSimilarPeopleCommon(ANSWER_NAME, SHORT_ANSWER_NAME,
                                                            DISABLE_FLAG)
    {
        Intent = ::NImages::NCbir::ECbirIntents::CI_SIMILAR_PEOPLE;
        IntentButtonIcon = "https://avatars.mds.yandex.net/get-images-similar-mturk/13615/icon-similar_people/orig";
    }

    TSimilarPeople* TSimilarPeople::GetPtr() {
        static TSimilarPeople* answer = new TSimilarPeople;
        return answer;
    }

    TSimilarPeopleFrontal::TSimilarPeopleFrontal() : TSimilarPeopleCommon(ANSWER_NAME_FRONTAL, SHORT_ANSWER_NAME_FRONTAL, DISABLE_FLAG_FRONTAL)
    {
        IsFrontalCaptureMode = true;
    }

    TSimilarPeopleFrontal* TSimilarPeopleFrontal::GetPtr() {
        static TSimilarPeopleFrontal* answer = new TSimilarPeopleFrontal;
        return answer;
    }

}
