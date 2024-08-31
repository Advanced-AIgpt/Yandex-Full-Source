#include "porn.h"
#include "entity.h"
#include "similarlike.h"

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_porn";
    constexpr TStringBuf SHORT_ANSWER_NAME = "porn";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_porn";
}

namespace NAlice::NHollywood::NImage::NAnswers {

    TPorn::TPorn(): IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
    {
    }

    bool TPorn::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const {
        const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
        if (!imageAliceResponseMaybe.Defined()) {
            return false;
        }

        if (force) {
            return true;
        }

        const auto intents = ctx.GetCbirIntents();
        const bool isEntityAnswerSuitable = !intents.empty() && intents.front() == NImages::NCbir::ECbirIntents::CI_ENTITY && TEntity::GetPtr()->IsSuitable(ctx, false);
        if (isEntityAnswerSuitable) {
            return false;
        }

        const NSc::TValue& classes = imageAliceResponseMaybe.GetRef()["Classes"];
        constexpr double thresholdBinaryPorn = 0.86;
        constexpr double thresholdMobilePorn = 0.37;
        return (classes["binary_porn"].GetNumber() > thresholdBinaryPorn) || (classes["mobile_porn"].GetNumber() > thresholdMobilePorn);
    }

    void TPorn::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
        if (ctx.GetFiltrationMode() != NScenarios::TUserPreferences_EFiltrationMode_FamilySearch) {
            ctx.AddOpenUriButton("open_similarlike", ctx.GenerateImagesSearchUrl("similar", TStringBuf("imageview"),
                                                                                 /* disable ptr */ false, "similar"));
            const TString id(TSimilarLike::GetPtr()->GetAnswerName());
            const TString actionId = ctx.AddSwitchIntentAction(id);
            ctx.AddActionSuggest(id, actionId);
        }
        ctx.AddTextCard("render_porn", {});
    }

    TPorn* TPorn::GetPtr() {
        static TPorn* answer = new TPorn;
        return answer;
    }

}
