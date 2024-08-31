#include "image_what_is_this_handler.h"
#include "image_what_is_this_render.h"

#include <alice/hollywood/library/scenarios/image_what_is_this/proto/image_what_is_this.pb.h>

#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_int_handler.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer_library.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/library/logger/logger.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/string/split.h>

using namespace NAlice::NScenarios;


namespace NAlice::NHollywood::NImage {

TImageWhatIsThisHandle::TImageWhatIsThisHandle() {
    NAnswers::TAnswerLibrary::Init();
}

void TImageWhatIsThisHandle::Do(TScenarioHandleContext& ctx) const {
    TImageWhatIsThisRunContext context(ctx, EHandlerStage::RUN);

    if (context.ShouldStopHandling()) {
        context.MakeResponse();
        return;
    }

    const NAlice::NScenarios::TCallbackDirective* callback = context.GetCallback();
    bool isRepeat = false;
    if (callback) {
        if (callback->GetName() == "alice.image_what_is_this_feedback") {
            context.RenderFeedbackAnswer();
            context.MakeResponse();
            return;
        }

        const auto& payloadFields = callback->GetPayload().fields();
        if (payloadFields.count("repeat")) {
            isRepeat = true;
        }
    }

    const bool hasImage = context.ExtractImageUrl();

    bool hasAgainSameScenario = false;
    bool canSwitchTo = true;
    bool smartCameraRequest = false;
    TMaybe<TStringBuf> semanticFrameName = context.GetSemanticFrame();
    if (semanticFrameName.Defined()) {
        hasAgainSameScenario = semanticFrameName.GetRef() == context.GetState().GetLastAnswer();
        if (!callback) {
            NAnswers::IAnswer* answer = NAnswers::TAnswerLibrary::GetAnswerByNameOrDefault(semanticFrameName.GetRef());
            canSwitchTo = answer->GetIsSwitchableTo();

            ECaptureMode captureMode = ECaptureMode::TInput_TImage_ECaptureMode_Photo;
            TMaybe<ECaptureMode> answerCaptureMode = answer->GetOpenCaptureMode();
            if (answerCaptureMode.Defined()) {
                captureMode = answerCaptureMode.GetRef();
            }

            smartCameraRequest = answer->GetIsSupportSmartCamera() && context.SupportSmartMode();
        }
    }


    if (!isRepeat && semanticFrameName.Defined() && (!hasImage || hasAgainSameScenario || !canSwitchTo || smartCameraRequest)) {
        NAnswers::IAnswer* answer = NAnswers::TAnswerLibrary::GetAnswerByNameOrDefault(semanticFrameName.GetRef());
        context.RenderPhotoRequest(answer);
        context.MakeResponse();

        return;
    }

    context.AddContinueRequest();
    return;
}


}
