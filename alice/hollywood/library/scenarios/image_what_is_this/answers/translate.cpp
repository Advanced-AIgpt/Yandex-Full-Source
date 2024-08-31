#include "translate.h"

#include <alice/library/url_builder/url_builder.h>

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_translate";
    constexpr TStringBuf SHORT_ANSWER_NAME = "translate";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_translate";

    constexpr TStringBuf ALICE_MODE = "text";
    constexpr TStringBuf ALICE_SMART_MODE = "translate";
}

namespace NAlice::NHollywood::NImage::NAnswers {

    TTranslate::TTranslate(): IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
    {
        CaptureMode = ECaptureMode::TInput_TImage_ECaptureMode_Translate;
        IsSupportSmartCamera = true;

        AliceMode = ALICE_MODE;
        AliceSmartMode = ALICE_SMART_MODE;
    }

    bool TTranslate::IsSuitableAnswer(TImageWhatIsThisApplyContext&, bool force) const {
        return force;
    }

    void TTranslate::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
        ctx.AddTextCard("render_translate", {});
        const TString url = GenerateTranslateUrl(ctx);
        ctx.RedirectToCustomUri(url);
        // TODO: Add suggest
    }

    TString TTranslate::GenerateTranslateUrl(TImageWhatIsThisApplyContext& ctx) const {
        // TODO: Should we generate url in `urls_builder.cpp`?
        TStringBuilder url;
        url << TStringBuf("https://translate.yandex.ru/");
        TCgiParameters cgi;
        TStringBuilder imageUrl;
        imageUrl << "https://avatars.mds.yandex.net/get-images-cbir/" << ctx.GetCbirId() << "/ocr";
        cgi.InsertEscaped(TStringBuf("ocr_image_url"), imageUrl);
        url << "?" << NAlice::PrintCgi(cgi);
        return url;
    }

    TTranslate* TTranslate::GetPtr() {
        static TTranslate* answer = new TTranslate;
        return answer;
    }

}
