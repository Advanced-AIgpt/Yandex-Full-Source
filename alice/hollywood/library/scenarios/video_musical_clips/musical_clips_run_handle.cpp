#include "musical_clips_run_handle.h"

#include "musical_clips_defs.h"
#include "utils.h"

#include <alice/hollywood/library/scenarios/video_musical_clips/nlg/register.h>
#include <alice/hollywood/library/scenarios/video_musical_clips/proto/musical_clips.pb.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/player_features/last_play_timestamp.h>
#include <alice/hollywood/library/player_features/player_features.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/video_common/age_restriction.h>
#include <alice/library/video_common/defs.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>


using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMusicalClips {

void TMusicalClipsRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    const auto serverTimeMs = requestProto.GetBaseRequest().GetServerTimeMs();
    TMusicalClipsRequest requestArgs;

    if (request.Input().FindSemanticFrame(ALICE_SHOW_MUSICAL_CLIPS)) {
        LOG_INFO(ctx.Ctx.Logger()) << "Run stage proceeds to START flow...";
        // Check slots
        const auto frame = request.Input().CreateRequestFrame(ALICE_SHOW_MUSICAL_CLIPS);
        if (!CheckScenarioAppropriateSlots(ctx, &frame)) {
            AddIrrelevantResponse(ctx);
            return;
        }

        // Age restriction
        NAlice::NVideoCommon::TAgeRestrictionCheckerParams params;
        params.MinAge = 18u;
        params.RestrictionLevel = GetContentRestrictionLevel(request.ContentRestrictionLevel());
        if (!NAlice::NVideoCommon::PassesAgeRestriction(params)) {
            auto& bodyBuilder = builder.CreateResponseBodyBuilder();
            TNlgData nlgData(ctx.Ctx.Logger(), request);
            nlgData.AddAttention(NAlice::NVideoCommon::ATTENTION_ALL_RESULTS_FILTERED);
            bodyBuilder.AddRenderedTextWithButtonsAndVoice("video_musical_clips", "filtered_result", /* buttons = */ {}, nlgData);
            auto response = std::move(builder).BuildResponse();
            ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
            return;
        }

        requestArgs.SetType(NMusicalClips::TMusicalClipsRequest_ERequestContinueType_START);

    } else if (
               request.Input().FindSemanticFrame(ALICE_PLAYER_FINISHED)
            || request.Input().FindSemanticFrame(ALICE_PLAYER_NEXT_TRACK))  {
        LOG_INFO(ctx.Ctx.Logger()) << "Run stage proceeds to second call...";

        // Check clips scenario
        const auto currentlyPlaying = GetCurrentlyPlaying(request);
        if (!currentlyPlaying.Has("item") || !currentlyPlaying["item"].Has("musical_track_id")) {
            LOG_INFO(ctx.Ctx.Logger()) << "CurrentlyPlaying data source is empty.";
            AddIrrelevantResponse(ctx);
            return;
        }

        requestArgs.SetMusicalTrackId(currentlyPlaying["item"]["musical_track_id"].GetStringRobust());
        requestArgs.SetTotalPlayedSeconds(currentlyPlaying["progress"]["played"].GetUIntegerSafe());
        requestArgs.SetStartTimeMs(currentlyPlaying["last_play_timestamp"].GetUIntegerSafe());
        requestArgs.SetEndTimeMs(serverTimeMs);

        if (request.Input().FindSemanticFrame(ALICE_PLAYER_FINISHED)) {
            LOG_INFO(ctx.Ctx.Logger()) << "Run stage have got alice.quasar.video_player.finished semantic frame.";
            requestArgs.SetType(NMusicalClips::TMusicalClipsRequest_ERequestContinueType_NEXT);

        } else if (request.Input().FindSemanticFrame(ALICE_PLAYER_NEXT_TRACK)) {
            LOG_INFO(ctx.Ctx.Logger()) << "Run stage have got personal_assistant.scenarios.player.next_track semantic frame.";
            requestArgs.SetType(NMusicalClips::TMusicalClipsRequest_ERequestContinueType_SKIP);

        } else {
            AddIrrelevantResponse(ctx);
            return;
        }

        builder.AddPlayerFeatures(CalcPlayerFeatures(ctx.Ctx.Logger(), request, TInstant::Seconds(request.BaseRequestProto().GetDeviceState().GetVideo().GetLastPlayTimestamp())));
    } else {
        AddIrrelevantResponse(ctx);
        return;
    }

    builder.SetContinueArguments(requestArgs);
    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}
} // namespace NAlice::NHollywood::NMusicalClips
