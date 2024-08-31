#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_render.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer_library.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/utils.h>

using namespace NAlice::NHollywood::NImage;

void TImageWhatIsThisRender::Do(TScenarioHandleContext& ctx) const {
    TImageWhatIsThisApplyContext context(ctx, EHandlerStage::RENDER);
    context.ExtractImageUrl();

    const NAnswers::IAnswer* answer = context.GetForceAnswer();

    if (!answer) {
        if (context.GetSemanticFrame().Defined()) {
            TStringBuf semanticFrameName = context.GetSemanticFrame().GetRef();
            answer = NAnswers::TAnswerLibrary::GetAnswerByName(semanticFrameName);
        }
    }

    if (!answer) {
        answer = NAnswers::TAnswerLibrary::GetDefaultAnswer();
    }

    if (!answer->IsSuitable(context, true)) {
        answer->RenderError(context);
    } else {
        answer->Compose(context);
    }


    if (!answer->GetIsRepeatable()) {
        context.SetLastAnswer(TString(answer->GetTrueAnswerName(context)));
    } else {
        context.SetLastAnswer("");
    }

    context.GetState().SetImageUrl(context.GetImageUrl());
    if (context.GetCbirId().Defined()) {
        context.GetState().SetCbirId(context.GetCbirId().GetRef());
    }

    NAlice::NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder = context.GetAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetIntentName(TString(answer->GetTrueAnswerName(context)));
    analyticsInfoBuilder.AddObject("image_url", "image_url", context.GetImageUrl());
    TString captureMode;
    if (context.GetCallback()) {
        captureMode = context.GetCallback()->GetName();
    } else {
        captureMode = CaptureModeToString(context.GetCaptureMode());
    }
    analyticsInfoBuilder.AddObject("capture_mode", "capture_mode", captureMode);
    TMaybe<TString> imageAliceReqid = context.GetImageAliceReqid();
    if (imageAliceReqid.Defined()) {
        analyticsInfoBuilder.AddObject("image_alice_reqid", "image_alice_reqid", imageAliceReqid.GetRef());
        LOG_INFO(context.Logger()) << "Image Alice ReqId: " << imageAliceReqid.GetRef() << Endl;
    }

    answer->CleanUp(context);

    context.GetState().SetForceAnswer("");
    context.MakeResponse();
}
