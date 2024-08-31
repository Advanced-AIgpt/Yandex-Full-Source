#include <alice/bass/forms/computer_vision/translate_handler.h>
#include <alice/library/url_builder/url_builder.h>

#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/forms/directives.h>

using namespace NBASS;

bool TCVAnswerTranslate::IsSuitable(const TComputerVisionContext& ctx, bool force) const {
    Y_UNUSED(ctx);
    return force;
}

void TCVAnswerTranslate::Compose(TComputerVisionContext& ctx) const {
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_is_translate");
    ctx.AttachTextCard();
    ctx.Output("is_translate").SetBool(true);

    // TODO: Should we generate url in `urls_builder.cpp`?
    NSc::TValue link;
    TStringBuilder url;
    url << TStringBuf("https://translate.yandex.ru/");
    TCgiParameters cgi;
    TStringBuilder imageUrl;
    imageUrl << "https://avatars.mds.yandex.net/get-images-cbir/" << ctx.GetCbirId() << "/ocr";
    cgi.InsertEscaped(TStringBuf("ocr_image_url"), imageUrl);
    url << "?" << NAlice::PrintCgi(cgi);
    link["uri"].SetString(url);
    ctx.HandlerContext().AddCommand<TCVRedirectDirective>(TStringBuf("open_uri"), std::move(link));
    ctx.AttachCustomUriSuggest(TStringBuf("image_what_is_this__translate"), url);
}

void TCVAnswerTranslate::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest(
            {NImages::NCbir::ECbirIntents::CI_CLOTHES, NImages::NCbir::ECbirIntents::CI_MARKET,
             NImages::NCbir::ECbirIntents::CI_SIMILAR, NImages::NCbir::ECbirIntents::CI_OCR_VOICE},
            {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE, NImages::NCbir::ECbirIntents::CI_INFO});
}

TComputerVisionTranslateHandler::TComputerVisionTranslateHandler()
        : TComputerVisionMainHandler(ECaptureMode::TRANSLATE, false)
{
}

void TComputerVisionTranslateHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionTranslateHandler>();
    };
    handlers->emplace(TComputerVisionTranslateHandler::FormName(), handler);
}
