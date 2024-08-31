#include "poetry.h"

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_poetry";
    constexpr TStringBuf SHORT_ANSWER_NAME = "poetry";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_poetry";

    constexpr TStringBuf ALICE_SMART_MODE = "poetry";
}

namespace NAlice::NHollywood::NImage::NAnswers {

    TPoetry::TPoetry(): IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
    {
        CaptureMode = ECaptureMode::TInput_TImage_ECaptureMode_Poetry;
        IsSupportSmartCamera = true;
        IsSwitchableTo = false;
        IsForceable = false;

        AliceSmartMode = ALICE_SMART_MODE;
    }

    bool TPoetry::IsSuitableAnswer(TImageWhatIsThisApplyContext&, bool) const {
        return false;
    }

    void TPoetry::ComposeAnswer(TImageWhatIsThisApplyContext&) const {
    }

    TPoetry* TPoetry::GetPtr() {
        static TPoetry* answer = new TPoetry;
        return answer;
    }

}
