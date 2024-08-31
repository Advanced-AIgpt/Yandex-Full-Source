#include "continue_prepare_handle.h"
#include "common.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_request_init.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/music/defs.h>

namespace NAlice::NHollywood::NMusic {

void TBassMusicContinuePrepareHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    LOG_INFO(logger) << "TMusicArguments are "
                     << SerializeProtoText(applyArgs, /* singleLineMode= */ true, /* expandAny= */ true);

    auto execFlowType = applyArgs.GetExecutionFlowType();
    if (execFlowType == TMusicArguments_EExecutionFlowType_ThinClientDefault) {
        LOG_INFO(logger) << "Continue stage proceeds to ThinClientDefault flow...";
        AddMusicContentRequest(ctx, request);
    } else if (execFlowType == TMusicArguments_EExecutionFlowType_BassRadio) {
        const TString startFromTrackId = applyArgs.GetPlaybackOptions().GetStartFromTrackId();
        const auto frame = request.Input().FindSemanticFrame(MUSIC_PLAY_FRAME);
        bool isMuspult = frame && TFrame::FromProto(*frame).FindSlot(NAlice::NMusic::SLOT_OBJECT_TYPE) != nullptr;
        if (applyArgs.HasMusicSearchResult()) {
            LOG_INFO(logger) << "Continue stage proceeds to BassRadio flow with MusicSearchResult...";
            const auto& musicSearchResult = applyArgs.GetMusicSearchResult();
            const auto bassRequest = PrepareBassRadioSimilarToObjContinueRequest(
                logger, request, musicSearchResult.GetContentType(),
                musicSearchResult.GetContentId(), isMuspult,
                applyArgs.GetRadioStationId(), startFromTrackId, ctx.RequestMeta,
                MUSIC_CONTINUATION, ctx.AppHostParams
            );
            AddBassRequestItems(ctx, bassRequest);
        } else {
            // Empty radioStationId means user's personal station (here, in BASS request)
            LOG_INFO(logger) << "Continue stage proceeds to BassRadio flow with RadioStationId='"
                             << applyArgs.GetRadioStationId() << "'";
            const auto bassRequest = PrepareBassRadioStationIdContinueRequest(
                logger, request, applyArgs.GetRadioStationId(),
                applyArgs.GetFrom(), isMuspult, startFromTrackId,
                ctx.RequestMeta, MUSIC_CONTINUATION, ctx.AppHostParams
            );
            AddBassRequestItems(ctx, bassRequest);
        }
    } else if (execFlowType == TMusicArguments_EExecutionFlowType_BassDefault) {
        LOG_INFO(logger) << "Continue stage proceeds to BassDefault flow...";
        const auto bassRequest = PrepareBassApplyRequest(logger, request, applyArgs.GetBassScenarioState(),
                                                         ctx.RequestMeta, MUSIC_CONTINUATION, /* imageSearch= */ false,
                                                         ctx.AppHostParams);
        AddBassRequestItems(ctx, bassRequest);
    } else if (execFlowType == TMusicArguments_EExecutionFlowType_MusicSdkSubgraph) {
        LOG_INFO(logger) << "Continue stage proceeds to MusicSdkSubgraph flow...";
        ctx.ServiceCtx.AddFlag(NMusicSdk::SUBGRAPH_APPHOST_FLAG);
    } else {
        ythrow yexception() << "Unsupported ExecutionFlowType=" << TMusicArguments_EExecutionFlowType_Name(execFlowType);
    }
}

} // namespace NAlice::NHollywood::NMusic
