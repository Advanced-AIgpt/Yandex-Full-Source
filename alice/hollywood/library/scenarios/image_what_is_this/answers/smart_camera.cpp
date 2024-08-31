#include "smart_camera.h"

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_smart_camera";
    constexpr TStringBuf SHORT_ANSWER_NAME = "smart_camera";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_smart_camera";

    constexpr TStringBuf ALICE_SMART_MODE = "smartcamera";
}

namespace NAlice::NHollywood::NImage::NAnswers {

    TSmartCamera::TSmartCamera(): IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
    {
        CaptureMode = ECaptureMode::TInput_TImage_ECaptureMode_SmartCamera;
        IsSupportSmartCamera = true;
        IsSwitchableTo = false;
        IsForceable = false;

        AliceSmartMode = ALICE_SMART_MODE;
    }

    bool TSmartCamera::IsSuitableAnswer(TImageWhatIsThisApplyContext&, bool) const {
        return false;
    }

    void TSmartCamera::ComposeAnswer(TImageWhatIsThisApplyContext&) const {
    }

    TSmartCamera* TSmartCamera::GetPtr() {
        static TSmartCamera* answer = new TSmartCamera;
        return answer;
    }

}
