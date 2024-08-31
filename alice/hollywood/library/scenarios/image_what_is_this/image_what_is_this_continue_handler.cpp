#include "image_what_is_this_continue_handler.h"

#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer_library.h>

using namespace NAlice::NScenarios;


namespace NAlice::NHollywood::NImage {

void TImageWhatIsThisContinueHandle::Do(TScenarioHandleContext& ctx) const {
    TImageWhatIsThisApplyContext context(ctx, EHandlerStage::CONTINUE);
    context.ExtractImageUrl();

    const NAnswers::IAnswer* answer = nullptr;
    answer = context.GetForceAnswer();
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
    LOG_INFO(ctx.Ctx.Logger()) << "Answer is" << answer->GetAnswerName() << Endl;

    answer->MakeRequests(context);
}

}
