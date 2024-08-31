#include <alice/hollywood/library/scenarios/image_what_is_this/answers/similarlike.h>

using namespace NAlice::NHollywood::NImage;
using namespace NAlice::NHollywood::NImage::NAnswers;

namespace {

constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_similarlike";
constexpr TStringBuf SHORT_ANSWER_NAME = "similarlike";
constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_similarlike";

}

TSimilarLike::TSimilarLike()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
    Intent = NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE;
    IntentButtonIcon = "https://avatars.mds.yandex.net/get-images-similar-mturk/40186/icon-similar/orig";
}

TSimilarLike* TSimilarLike::GetPtr() {
    static TSimilarLike* answer = new TSimilarLike;
    return answer;
}

bool TSimilarLike::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const {
    return force && ctx.GetCbirId().Defined();
}

bool TSimilarLike::IsSuggestibleAnswer(TImageWhatIsThisApplyContext& ctx) const {
    return ctx.GetCbirId().Defined();
}

void TSimilarLike::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    ctx.RedirectToCustomUri(ctx.GenerateImagesSearchUrl("", TStringBuf("imageview"),
                                                        /* disable ptr */ false, "similar"));
    ctx.AddTextCard("render_similarlike_answer", {});
}
