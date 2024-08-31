#include <alice/hollywood/library/scenarios/image_what_is_this/answers/info.h>

using namespace NAlice::NHollywood::NImage;
using namespace NAlice::NHollywood::NImage::NAnswers;

namespace {

constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_info";
constexpr TStringBuf SHORT_ANSWER_NAME = "info";
constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_info";

}

TInfo::TInfo()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
    Intent = NImages::NCbir::ECbirIntents::CI_INFO;
    IntentButtonIcon = "https://avatars.mds.yandex.net/get-images-similar-mturk/15681/icon-info/orig";
}

TInfo* TInfo::GetPtr() {
    static TInfo* answer = new TInfo;
    return answer;
}

bool TInfo::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const {
    return force && ctx.GetCbirId().Defined();
}

bool TInfo::IsSuggestibleAnswer(TImageWhatIsThisApplyContext& ctx) const {
    return ctx.GetCbirId().Defined();
}

void TInfo::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    ctx.RedirectTo("", "imageview", /* disable ptr */ true);
    ctx.AddTextCard("render_info_answer", {});

    //ctx.HandlerContext().AddOnboardingSuggest();
}
