#include "impl.h"

#include <alice/hollywood/library/scenarios/music/util/util.h>

namespace NAlice::NHollywood::NMusic::NImpl {

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::CreateIrrelevantSilentResponse() {
    THwFrameworkRunResponseBuilder response{Ctx_, &Nlg_, ConstructBodyRenderer(Request_)};
    response.SetIrrelevant();
    return *std::move(response).BuildResponse();
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::CreateIrrelevantResponseMusicNotFound() {
    // disable_nlg doesn't work at irrelevant responses
    THwFrameworkRunResponseBuilder response{Ctx_, &Nlg_, ConstructBodyRenderer(Request_, /* forceNlg = */ true)};
    response.SetIrrelevant();

    auto& bodyBuilder = response.CreateResponseBodyBuilder();
    TNlgData nlgData{Logger_, Request_};
    nlgData.Context["error"]["data"]["code"] = "music_not_found";
    nlgData.Context["attentions"].SetType(NJson::JSON_MAP);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, "render_error__musicerror",
                                                   /* buttons = */ {}, nlgData);
    return *std::move(response).BuildResponse();
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::CreateNotSupportedResponse(const TStringBuf phraseName) {
    THwFrameworkRunResponseBuilder response{Ctx_, &Nlg_, ConstructBodyRenderer(Request_)};

    auto& bodyBuilder = response.CreateResponseBodyBuilder();
    TNlgData nlgData{Logger_, Request_};
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, phraseName,
                                                   /* buttons = */ {}, nlgData);

    auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfo.SetIntentName("personal_assistant.scenarios.common.prohibition_error");

    return *std::move(response).BuildResponse();
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::MakeBassRadioResponseWithContinueArguments(TStringBuf radioStationId) {
    THwFrameworkRunResponseBuilder response{Ctx_, &Nlg_, ConstructBodyRenderer(Request_)};
    auto args = MakeMusicArgumentsImpl(TMusicArgumentsParams{
        TMusicArguments_EExecutionFlowType_BassRadio,
        /* .IsNewContentRequestedByUser= */ true,
    }); // don't pass datasources
    args.SetRadioStationId(radioStationId.data(), radioStationId.size());

    if (auto fromSlot = Frame_.FindSlot(NAlice::NMusic::SLOT_FROM)) {
        args.SetFrom(fromSlot->Value.AsString());
    }
    if (auto startFromTrackIdSlot = Frame_.FindSlot(NAlice::NMusic::SLOT_START_FROM_TRACK_ID)) {
        args.MutablePlaybackOptions()->SetStartFromTrackId(startFromTrackIdSlot->Value.AsString());
    }

    response.SetContinueArguments(args);
    return *std::move(response).BuildResponse();
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::MakeThinClientDefaultResponseWithEmptyContinueArguments() {
    THwFrameworkRunResponseBuilder response{Ctx_, &Nlg_, ConstructBodyRenderer(Request_)};
    auto args = MakeMusicArguments(Logger_, Request_, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         /* isNewContentRequestedByUser= */ false);
    response.SetContinueArguments(args);
    return *std::move(response).BuildResponse();
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::MakeThinClientDefaultResponseWithEmptyApplyArguments() {
    THwFrameworkRunResponseBuilder response{Ctx_, &Nlg_, ConstructBodyRenderer(Request_)};
    auto args = MakeMusicArguments(Logger_, Request_, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         /* isNewContentRequestedByUser= */ false);
    response.SetApplyArguments(args);
    return *std::move(response).BuildResponse();
}

} // namespace NAlice::NHollywood::NMusic::NImpl
