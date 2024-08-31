#include "face.h"
#include "ocr.h"
#include "similars.h"

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_face";
    constexpr TStringBuf SHORT_ANSWER_NAME = "face";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_face";
}

namespace NAlice::NHollywood::NImage::NAnswers {

    TFace::TFace(): IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
    {
        Intent = NImages::NCbir::ECbirIntents::CI_FACES;
        AllowedIntents = {
            NImages::NCbir::ECbirIntents::CI_CLOTHES,
            NImages::NCbir::ECbirIntents::CI_MARKET,
            NImages::NCbir::ECbirIntents::CI_OCR,
            NImages::NCbir::ECbirIntents::CI_SIMILAR,
            NImages::NCbir::ECbirIntents::CI_OCR_VOICE
        };
        LastForceAlternativeSuggest = {
            NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
            NImages::NCbir::ECbirIntents::CI_INFO
        };
    }

    bool TFace::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool) const {
        const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
        if (!imageAliceResponseMaybe.Defined()) {
            return false;
        }

        const NSc::TValue& imageAliceResponse = imageAliceResponseMaybe.GetRef();
        LOG_INFO(ctx.Logger()) << "Face count " << imageAliceResponse["Faces"].ArraySize();
        return imageAliceResponse["Faces"].ArraySize() > 0;
    }

    void TFace::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
        ctx.AddTextCard("render_face", {});

        TSimilars* similarAnswer = TSimilars::GetPtr();
        similarAnswer->AddSimilarsGallery(ctx);

        TOcr* ocrAnswer = TOcr::GetPtr();
        ocrAnswer->AddOcrAnswer(ctx, EOcrResultCategory::ORC_VAGUE);
    }

    TFace* TFace::GetPtr() {
        static TFace* answer = new TFace;
        return answer;
    }

}
