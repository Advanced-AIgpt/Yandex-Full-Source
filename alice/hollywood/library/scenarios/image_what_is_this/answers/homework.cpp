#include "homework.h"

namespace {
    constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_homework";
    constexpr TStringBuf SHORT_ANSWER_NAME = "homework";
    constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_homework";

    constexpr TStringBuf ALICE_SMART_MODE = "smartcamera";
    constexpr TStringBuf ANDROID_HOMEWORK = "homework";
    constexpr TStringBuf IOS_HOMEWORK = "gdz";
}

namespace NAlice::NHollywood::NImage::NAnswers {

    THomeWork::THomeWork(): IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
    {
        CaptureMode = ECaptureMode::TInput_TImage_ECaptureMode_Homework;
        IsSupportSmartCamera = true;
        IsSwitchableTo = false;
        IsForceable = false;
    }

    bool THomeWork::IsSuitableAnswer(TImageWhatIsThisApplyContext&, bool) const {
        return false;
    }

    void THomeWork::ComposeAnswer(TImageWhatIsThisApplyContext&) const {
    }

    THomeWork* THomeWork::GetPtr() {
        static THomeWork* answer = new THomeWork;
        return answer;
    }

    TMaybe<TStringBuf> THomeWork::GetAliceSmartMode(const TImageWhatIsThisRunContext& ctx) const {
        const TClientInfo& clientInfo = ctx.GetClientInfo();
        if (clientInfo.IsSearchApp()) {
            if (clientInfo.IsAndroidAppOfVersionOrNewer(21, 8)) {
                return ANDROID_HOMEWORK;
            }

            if (clientInfo.IsIOSAppOfVersionOrNewer(8100)) {
                return IOS_HOMEWORK;
            }
        } else if (clientInfo.IsYaBrowser()) {
            if (clientInfo.IsAndroidAppOfVersionOrNewer(21, 6, 6)) {
                return ANDROID_HOMEWORK;
            }

            if (clientInfo.IsIOSAppOfVersionOrNewer(2201, 6)) {
                return IOS_HOMEWORK;
            }
        }

        return ALICE_SMART_MODE;
    }

}
