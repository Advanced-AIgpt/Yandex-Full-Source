#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_int_handler.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer_library.h>

using namespace NAlice::NHollywood::NImage;

void TImageWhatIsThisIntHandle::Do(TScenarioHandleContext& ctx) const {
    TImageWhatIsThisApplyContext context(ctx, EHandlerStage::INT);
    context.ExtractImageUrl();

    const NAnswers::IAnswer* answer = nullptr;
    const TStringBuf forceAnswerName = context.GetState().GetForceAnswer();
    if (forceAnswerName) {
        answer = NAnswers::TAnswerLibrary::GetAnswerByName(forceAnswerName);
    }
    if (!answer) {
        ECaptureMode captureMode = context.GetCaptureMode();
        answer = NAnswers::TAnswerLibrary::GetAnswerByCaptureMode(captureMode);
    }

    if (!answer) {
        if (context.GetSemanticFrame().Defined()) {
            TStringBuf semanticFrameName = context.GetSemanticFrame().GetRef();
            answer = NAnswers::TAnswerLibrary::GetAnswerByName(semanticFrameName);
        }
    }

    if (!answer) {
        answer = NAnswers::TAnswerLibrary::GetDefaultAnswer();
    }

    answer->MakeRequests(context);
}
