#include "similar_artwork.h"

#include <alice/hollywood/library/scenarios/image_what_is_this/utils.h>

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_similar_artwork";
    constexpr TStringBuf SHORT_ANSWER_NAME = "similar_artwork";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_similar_artwork";
}

namespace NAlice::NHollywood::NImage::NAnswers {

    TSimilarArtwork::TSimilarArtwork(): IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
    {
        Intent = ::NImages::NCbir::ECbirIntents::CI_SIMILAR_ARTWORK;
        IntentButtonIcon = "https://avatars.mds.yandex.net/get-images-similar-mturk/15681/icon-similar_artwork/orig";
        AllowedIntents = {
            ::NImages::NCbir::ECbirIntents::CI_SIMILAR,
            ::NImages::NCbir::ECbirIntents::CI_CLOTHES,
            ::NImages::NCbir::ECbirIntents::CI_MARKET,
            ::NImages::NCbir::ECbirIntents::CI_OCR,
            ::NImages::NCbir::ECbirIntents::CI_OCR_VOICE,
        };
        FirstForceAlternativeSuggest = {
            ::NImages::NCbir::ECbirIntents::CI_SIMILAR_PEOPLE,
        };
        LastForceAlternativeSuggest = {
            ::NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
            ::NImages::NCbir::ECbirIntents::CI_INFO,
        };

        IsSwitchableTo = false;
    }

    void TSimilarArtwork::MakeRequests(TImageWhatIsThisApplyContext& ctx) const {
        if (!ctx.GetImageAliceResponse().Defined()) {
            const TVector<std::pair<TStringBuf, TStringBuf>> params = {{"flag", "artwork_filter"}};
            ctx.AddImageAliceRequest(params);
        }
    }

    bool TSimilarArtwork::IsSuggestibleAnswer(TImageWhatIsThisApplyContext&) const {
        return true;
    }

    bool TSimilarArtwork::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool) const {
        const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
        if (!imageAliceResponseMaybe.Defined()) {
            return false;
        }

        const NSc::TValue& imageAliceResponse = imageAliceResponseMaybe.GetRef();
        return imageAliceResponse["SimilarArtwork"].ArraySize() > 0;
    }

    void TSimilarArtwork::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
        const auto& similarArtwork = ctx.GetImageAliceResponse().GetRef()["SimilarArtwork"][0];

        NSc::TValue textCard;
        textCard["name"] = similarArtwork["title"];
        textCard["is_cyrillic"].SetBool(IsCyrillic(similarArtwork["title"].GetString()));
        ctx.AddTextCard("render_similar_artwork_result", textCard);

        NSc::TValue card;
        card["preview_image"] = similarArtwork["image"];
        card["url_image"] = similarArtwork["image_url"];
        card["url_source"] = similarArtwork["url"];
        card["title"] = similarArtwork["title"];
        card["author"] = similarArtwork["author"];
        card["source"] = similarArtwork["source"];
        card["source_image"] = ctx.GetImageUrl();
        int similarity = std::lround(similarArtwork["similarity"].GetNumber() * 100);
        similarity = std::min(3 * similarity, 90);
        card["similarity"] = similarity;
        const int height = 160;
        card["height"] = height;
        int imgHeight = similarArtwork["original_size"]["height"].GetIntNumber();
        if (imgHeight == 0) {
            imgHeight = height;
        }
        const int width = 160;
        int imgWidth = similarArtwork["original_size"]["width"].GetIntNumber();
        if (imgWidth == 0) {
            imgWidth = width;
        }
        imgHeight = Max(160, Min(240, static_cast<int>(1.0 * width * imgHeight / imgWidth)));
        card["height"] = imgHeight;
        card["ratio"] = static_cast<float>(1.0 * width / imgHeight);
        if (ctx.GetRequest().ClientInfo().IsIOS()) {
            card["share_icon"] = "http://avatars.mds.yandex.net/get-images-similar-mturk/16267/ios_share/orig";
        } else {
            card["share_icon"] = "http://avatars.mds.yandex.net/get-images-similar-mturk/41142/android_share/orig";
        }
        ctx.AddDivCardBlock(TStringBuf("similar_artwork"), std::move(card));

        const TString actionId = ctx.AddAction(TString(GetAnswerName()));
        ctx.AddActionSuggest(TString(GetAnswerName()) + "_again", actionId);

        ctx.GetAnalyticsInfoBuilder().AddObject("similar_artwork", "similar_artwork", TString(similarArtwork["title"].GetString()));
    }

    TSimilarArtwork* TSimilarArtwork::GetPtr() {
        static TSimilarArtwork* answer = new TSimilarArtwork;
        return answer;
    }

    bool TSimilarArtwork::RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const {
        if (ctx.GetImageAliceResponse().Defined()) {
            ctx.AddTextCard("render_cannot_apply_similar_artwork_error", {});
            return true;
        }
        return false;
    }

}
