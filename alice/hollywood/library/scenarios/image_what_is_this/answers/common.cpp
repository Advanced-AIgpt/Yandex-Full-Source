#include <alice/hollywood/library/scenarios/image_what_is_this/answers/common.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer_library.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>

using namespace NAlice::NHollywood::NImage;
using namespace NAlice::NHollywood::NImage::NAnswers;

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_common";
    constexpr TStringBuf SHORT_ANSWER_NAME = "common";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_common";

    constexpr TStringBuf ALICE_SMART_MODE = "smartcamera";
}

TCommon::TCommon()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
    IsSwitchableTo = false;
    IsSupportSmartCamera = true;

    AliceSmartMode = ALICE_SMART_MODE;
}

void TCommon::MakeRequests(TImageWhatIsThisApplyContext& ctx) const {
    if (!ctx.GetImageAliceResponse().Defined()) {
        ctx.AddImageAliceRequest();
    } else {
        IAnswer* bestAnswer = FindBestIntent(ctx);
        if (!bestAnswer) {
            return;
        }
        bestAnswer->MakeRequests(ctx);
    }
}

void TCommon::Compose(TImageWhatIsThisApplyContext& ctx) const {
    ComposeAnswer(ctx);
}

TCommon* TCommon::GetPtr() {
    static TCommon* answer = new TCommon;
    return answer;
}

TStringBuf TCommon::GetTrueAnswerName(TImageWhatIsThisApplyContext& ctx) const {
    IAnswer* bestIntent = FindBestIntent(ctx);
    if (!bestIntent) {
        return "";
    }
    return bestIntent->GetTrueAnswerName(ctx);
}

TStringBuf TCommon::GetShortAnswerName(TImageWhatIsThisApplyContext& ctx) const {
    IAnswer* bestIntent = FindBestIntent(ctx);
    if (!bestIntent) {
        return "";
    }
    return bestIntent->GetShortAnswerName(ctx);
}

bool TCommon::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool /*force*/) const {
    IAnswer* bestIntent = FindBestIntent(ctx);
    if (!bestIntent) {
        return false;
    }

    return bestIntent->IsSuitable(ctx, /*force*/ false);
}

void TCommon::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    IAnswer* bestIntent = FindBestIntent(ctx);
    if (!bestIntent) {
        return;
    }

    return bestIntent->Compose(ctx);
}

IAnswer* TCommon::FindBestIntent(TImageWhatIsThisApplyContext& ctx) const {
    const TMaybe<IAnswer*>& bestIntent = ctx.GetBestIntent();
    if (bestIntent.Defined()) {
        return bestIntent.GetRef();
    }

    const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
    if (!imageAliceResponseMaybe.Defined()) {
        return nullptr;
    }

    for (auto answer : TAnswerLibrary::GetStubAnswers()) {
        if (answer->IsSuitable(ctx, false)) {
            LOG_INFO(ctx.Logger()) << "Selected stub answer " << answer->GetAnswerName() << Endl;
            ctx.SetBestIntent(answer);
            return answer;
        }
    }

    for (NImages::NCbir::ECbirIntents intent : ctx.GetCbirIntents()) {
        IAnswer* intentAnswer = TAnswerLibrary::GetAnswerByIntent(intent);
        if (!intentAnswer) {
            LOG_ERROR(ctx.Logger()) << "No answer for intent " << intent;
            continue;
        }
        if (!intentAnswer->IsSuitable(ctx, /*force*/ false)) {
            if (ctx.GetHandlerStage() == EHandlerStage::RENDER) {
                ctx.StatIncCounter("hollywood_computer_vision_error_cannot_apply_" + TString(intentAnswer->GetShortAnswerName(ctx)));
            }
            continue;
        }

        LOG_INFO(ctx.Logger()) << "Best answer " << intentAnswer->GetAnswerName() << Endl;
        ctx.SetBestIntent(intentAnswer);
        return intentAnswer;
    }

    LOG_INFO(ctx.Logger()) << "No best answer" << Endl;
    return nullptr;
}
