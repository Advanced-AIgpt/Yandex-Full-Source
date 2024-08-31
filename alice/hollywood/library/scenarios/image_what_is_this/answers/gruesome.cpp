#include "gruesome.h"
#include "similarlike.h"

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_gruesome";
    constexpr TStringBuf SHORT_ANSWER_NAME = "gruesome";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_gruesome";
}

namespace NAlice::NHollywood::NImage::NAnswers {

    TGruesome::TGruesome(): IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
    {
    }

    bool TGruesome::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const {
        const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
        if (!imageAliceResponseMaybe.Defined()) {
            return false;
        }

        if (force) {
            return true;
        }

        const NSc::TValue& classes = imageAliceResponseMaybe.GetRef()["Classes"];
        constexpr double thresholdGruesome = 0.85;
        return (classes["gruesome"].GetNumber() > thresholdGruesome);
    }

    void TGruesome::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
        if (ctx.GetFiltrationMode() != NScenarios::TUserPreferences_EFiltrationMode_FamilySearch) {
            ctx.AddOpenUriButton("open_similarlike", ctx.GenerateImagesSearchUrl("similar", TStringBuf("imageview"), 
                                                                                 /* disable ptr */ false, "similar"));
            const TString id(TSimilarLike::GetPtr()->GetAnswerName());
            const TString actionId = ctx.AddSwitchIntentAction(id);
            ctx.AddActionSuggest(id, actionId);
        }
        ctx.AddTextCard("render_gruesome", {});
    }

    TGruesome* TGruesome::GetPtr() {
        static TGruesome* answer = new TGruesome;
        return answer;
    }

}
