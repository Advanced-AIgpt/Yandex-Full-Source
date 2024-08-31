#include "dark.h"
#include "similarlike.h"

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_dark";
    constexpr TStringBuf SHORT_ANSWER_NAME = "dark";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_dark";
}

namespace NAlice::NHollywood::NImage::NAnswers {

    TDark::TDark(): IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
    {
    }

    bool TDark::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const {
        const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
        if (!imageAliceResponseMaybe.Defined()) {
            return false;
        }

        const NSc::TValue& imageAliceResponse = imageAliceResponseMaybe.GetRef();

        constexpr double thresholdDark = 0.85;
        return (force || imageAliceResponse["Classes"]["dark"].GetNumber() > thresholdDark);
    }

    void TDark::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
        ctx.AddOpenUriButton("open_similarlike", ctx.GenerateImagesSearchUrl("similar", TStringBuf("imageview"),
                                                                             /* disable ptr */ false, "similar"));
        ctx.AddTextCard("render_dark", {});

        const TString id(TSimilarLike::GetPtr()->GetAnswerName());
        const TString actionId = ctx.AddSwitchIntentAction(id);
        ctx.AddActionSuggest(id, actionId);
    }

    TDark* TDark::GetPtr() {
        static TDark* answer = new TDark;
        return answer;
    }

}
