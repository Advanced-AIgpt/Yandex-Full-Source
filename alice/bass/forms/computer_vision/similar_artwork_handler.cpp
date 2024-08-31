#include <alice/bass/forms/computer_vision/similar_artwork_handler.h>

#include <alice/bass/libs/metrics/metrics.h>

#include <dict/dictutil/scripts.h>
#include <util/charset/wide.h>

using namespace NBASS;
using namespace NImages::NCbir;

namespace {

bool IsCyrillic(const TStringBuf text) {
    const TUtf16String textWide = UTF8ToWide(text);
    const TScriptMask scriptMask = ClassifyScript(textWide.data(), textWide.size());
    return scriptMask == TScriptMask::CYRILLIC;
};

}


TCVAnswerSimilarArtwork::TCVAnswerSimilarArtwork()
    : IComputerVisionAnswer(
            TStringBuf("image_what_is_this__similar_artwork"),
            TStringBuf("https://avatars.mds.yandex.net/get-images-similar-mturk/15681/icon-similar_artwork/orig"))
{
}

bool TCVAnswerSimilarArtwork::IsSuitable(const TComputerVisionContext& ctx, bool /* force */) const {
    return ctx.GetData()["SimilarArtwork"].ArraySize() > 0;
}

bool TCVAnswerSimilarArtwork::TryApplyTo(TComputerVisionContext& ctx, bool force, bool shouldAttachAlternativeIntents) const {
    if (IsSuitable(ctx, force) && !DisabledByFlag(ctx)) {
        Compose(ctx);
        AttachAlternativeIntentsSuggest(ctx);
        ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::USELESS);
        ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OTHER);
        return true;
    } else if (force && shouldAttachAlternativeIntents) {
        AttachCannotApplyMessage(ctx);
    }

    return false;
}


void TCVAnswerSimilarArtwork::Compose(TComputerVisionContext& ctx) const {
    const auto& similarArtwork = ctx.GetData()["SimilarArtwork"][0];
    ctx.Output("is_similar_artwork") = true;
    ctx.Output("name") = similarArtwork["title"];
    ctx.Output("is_cyrillic").SetBool(IsCyrillic(similarArtwork["title"].GetString()));

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
    if (ctx.HandlerContext().MetaClientInfo().IsIOS()) {
        card["share_icon"] = "http://avatars.mds.yandex.net/get-images-similar-mturk/16267/ios_share/orig";
    } else {
        card["share_icon"] = "http://avatars.mds.yandex.net/get-images-similar-mturk/41142/android_share/orig";
    }

    ctx.HandlerContext().AddDivCardBlock(TStringBuf("similar_artwork"), std::move(card));
    ctx.AttachTextCard();
    ctx.HandlerContext().AddSuggest("image_what_is_this__similar_artwork");
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_similar_artwork");
}

TStringBuf TCVAnswerSimilarArtwork::GetCannotApplyMessage() const {
    return TStringBuf("cannot_apply_similar_artwork");
}


void TCVAnswerSimilarArtwork::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest(
            {ECbirIntents::CI_SIMILAR, ECbirIntents::CI_CLOTHES, ECbirIntents::CI_MARKET, ECbirIntents::CI_OCR, ECbirIntents::CI_OCR_VOICE},
            {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE, NImages::NCbir::ECbirIntents::CI_INFO},
            {ECbirIntents::CI_SIMILAR_PEOPLE});
}

TComputerVisionSimilarArtworkHandler::TComputerVisionSimilarArtworkHandler()
        : TComputerVisionMainHandler(ECaptureMode::SIMILAR_ARTWORK, false)
{
}

void TComputerVisionSimilarArtworkHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionSimilarArtworkHandler>();
    };
    handlers->emplace(TComputerVisionSimilarArtworkHandler::FormName(), handler);
}


void TComputerVisionEllipsisSimilarArtworkHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TComputerVisionEllipsisSimilarArtworkHandler>();
    };
    handlers->emplace("personal_assistant.scenarios.image_what_is_this__similar_artwork", handler);
}

bool TComputerVisionEllipsisSimilarArtworkHandler::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    cvContext.Switch(NComputerVisionForms::IMAGE_WHAT_IS_THIS);
    if (!AnswerSimilarArtwork.TryApplyTo(cvContext, /* force */ true)) {
        cvContext.Output("code").SetString("similar_artwork_empty");
        Y_STATS_INC_COUNTER("bass_computer_vision_result_ellipsis_similar_artwork_empty");
    }
    return true;
}

