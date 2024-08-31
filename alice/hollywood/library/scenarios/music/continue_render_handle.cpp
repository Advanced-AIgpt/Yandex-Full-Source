#include "continue_render_handle.h"

#include "common.h"

#include <alice/hollywood/library/scenarios/music/analytics_info/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/bedtime_tales.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/child_age_settings.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/intents.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/semantic_frames.h>
#include <alice/hollywood/library/scenarios/music/commands.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/unauthorized_user_directives.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_memento_scenario_data.pb.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/bass_adapter/bass_stats.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/sound/sound_change.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/json/json.h>
#include <alice/library/util/rng.h>
#include <alice/memento/proto/api.pb.h>

namespace NAlice::NHollywood::NMusic {

namespace {

// Only needed for thin client withouth thin radio on Smart TV, maybe better to remove later
std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse>
ApplyThickClientRenderUnauthorizedHandleImpl(
    TRTLogger& logger,
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    TNlgWrapper& nlgWrapper,
    const NJson::TJsonValue& stateJson,
    TScenarioState scState)
{
    LOG_INFO(logger) << "Rendering unauthorization if is needed";
    TApplyResponseBuilder builder(&nlgWrapper, ConstructBodyRenderer(applyRequest, /* forceNlg = */ true));
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    if (TryAddUnauthorizedUserDirectivesForThickClient(applyRequest, stateJson, bodyBuilder)) {
        TNlgData nlgData{logger, applyRequest};
        nlgData.Context["error"]["data"]["code"] = "music_authorization_problem";
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, "render_error__unauthorized", {}, nlgData);
    } else {
        return nullptr;
    }

    bodyBuilder.SetState(scState);

    return std::move(builder).BuildResponse();
}

} // namespace

namespace NImpl {

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse> MusicApplyRenderDoImpl(
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    const NJson::TJsonValue& bassResponse,
    NHollywood::TContext& ctx,
    TNlgWrapper& nlgWrapper,
    IRng& rng)
{
    auto& logger = ctx.Logger();

    const auto& state = bassResponse["State"];
    Y_ENSURE(state.IsDefined()); // otherwise we have an internal error response
    auto stateJson = JsonFromString(state.GetStringSafe());

    TScenarioState scState;
    const bool hasState = ReadScenarioState(applyRequest.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    const TMusicArguments applyArgs = applyRequest.UnpackArguments<TMusicArguments>();

    if (auto resp = ApplyThickClientRenderUnauthorizedHandleImpl(logger, applyRequest, nlgWrapper, stateJson, scState)) {
        return resp;
    }

    auto& sensors = ctx.GlobalContext().Sensors();

    LOG_INFO(logger) << "Initializing the BASS renderer";
    // TODO(sparkle, zhigan): disable_nlg doesn't work properly at BASS backend
    TApplyResponseBuilder builder(&nlgWrapper, ConstructBassBodyRenderer(applyRequest));

    const auto form = NAlice::NHollywood::NImpl::ParseBassForm(stateJson["form"]);
    auto* bodyBuilder = builder.GetResponseBodyBuilder() ? builder.GetResponseBodyBuilder()
                                                         : &builder.CreateResponseBodyBuilder(form.Get());

    TMaybe<TString> soundFrameName =
        NSound::RenderSoundChangeIfExists(logger, applyRequest, applyRequest.Input(), builder);

    const auto& analyticsInfoData = applyArgs.GetAnalyticsInfoData();

    const bool addFistTrackObject = analyticsInfoData.GetPlayerCommand() == TMusicArguments_EPlayerCommand_None;

    TBassResponseRenderer bassRenderer(applyRequest, applyRequest.Input(), builder, logger,
        /* suggestAutoAction= */ false, /* reduceWhitespaceInCards= */ false,
        /* addFistTrackObject= */ addFistTrackObject);

    {
        const auto fastData = ctx.GlobalContext().FastData().GetFastData<TMusicFastData>();
        AddAudiobrandingAttention(stateJson, *fastData, applyRequest);
    }

    {
        const auto stationPromoFastData = ctx.GlobalContext().FastData().GetFastData<TStationPromoFastData>();
        TryAddStationPromoAttention(stateJson, *stationPromoFastData, applyRequest, rng);
    }

    ProcessBassResponseUpdateSensors(logger, sensors, stateJson, "music", "apply");

    const auto& userConfigs = applyRequest.BaseRequestProto().GetMemento().GetUserConfigs();

    if (ShouldAddChildAgePromo(logger, applyRequest, rng)) {
        bassRenderer.SetContextValue("is_fairy_tale_subscenario", true);
        AddChildAgePromoAttention(stateJson);
        AddChildAgePromoDirectives(logger, applyRequest, *bodyBuilder);
    }
    if (ShouldAddBedtimeTalesOnboarding(logger, applyRequest)) {
        bassRenderer.SetContextValue("is_bedtime_tales", true);
        AddBedtimeTalesOnboardingAttention(stateJson, false);
        AddBedtimeTalesOnboardingDirective(logger, applyRequest, *bodyBuilder);
    }
    bassRenderer.SetContextValue("is_fairy_tale_subscenario", applyArgs.GetFairyTaleArguments().GetIsFairyTaleSubscenario());
    bassRenderer.SetContextValue("is_ondemand_fairytale", applyArgs.GetFairyTaleArguments().GetIsOndemand());

    const auto playerFrame = FindPlayerFrame(applyRequest);
    const TString& parentScenarioName = hasState && playerFrame
        ? scState.GetProductScenarioName() : Default<TString>();

    const auto& fixlist = applyArgs.GetFixlist();
    const NJson::TJsonValue fixlistJson = JsonFromString(fixlist);
    if (fixlistJson.Has("nlg")) {
        bassRenderer.SetContextValue("fixlist", fixlistJson);
        bassRenderer.Render(TEMPLATE_MUSIC_PLAY, "fixlist", stateJson, Default<TString>(), parentScenarioName);
    } else if (applyArgs.GetFairyTaleArguments().GetIsFairyTaleSubscenario()) {
        bassRenderer.SetContextValue("is_fairy_tale_subscenario", true);
        if (IsBedtimeTales(applyRequest.Input(), applyRequest.HasExpFlag(NExperiments::EXP_FAIRY_TALES_BEDTIME_TALES))) {
            bassRenderer.SetContextValue("is_bedtime_tales", true);
        }

        if (applyRequest.HasExpFlag(NExperiments::EXP_MUSIC_INTERACTIVE_FAIRYTALES)) {
            bassRenderer.SetContextValue("max_reasks_exceeded", true);
            bassRenderer.Render(TEMPLATE_MUSIC_PLAY, "interactive_fairytale", stateJson, MUSIC_FAIRYTALE_INTENT, MUSIC_FAIRYTALE_SCENARIO_NAME);
        } else {
            bassRenderer.SetContextValue("has_child_age_setting", ChildAgeIsSet(userConfigs.GetChildAge()));
            bassRenderer.Render(TEMPLATE_MUSIC_PLAY, "render_result", stateJson, MUSIC_FAIRYTALE_INTENT, MUSIC_FAIRYTALE_SCENARIO_NAME);
        }

    } else {
        bassRenderer.Render(TEMPLATE_MUSIC_PLAY, "render_result", stateJson, Default<TString>(), parentScenarioName);
    }

    if (const auto analyticsInfoPlayerCommand = analyticsInfoData.GetPlayerCommand(); analyticsInfoPlayerCommand == TMusicArguments_EPlayerCommand_None) {
        FillAnalyticsInfoMusicEvent(logger, stateJson, bodyBuilder, applyRequest);
    } else {
        LOG_INFO(logger) << "Filling action with command " << TMusicArguments_EPlayerCommand_Name(analyticsInfoPlayerCommand)
                         << " while turning on fallback radio in thick player";
        const TString& scenarioName = hasState ? scState.GetProductScenarioName() : Default<TString>();
        if (analyticsInfoPlayerCommand == TMusicArguments_EPlayerCommand_Rewind) {
            const auto [rewindType, rewindMs] = GetRewindArguments(applyRequest);
            CreateAndFillAnalyticsInfoForPlayerCommandRewind(*bodyBuilder, rewindType, rewindMs,
                                                             applyRequest.ServerTimeMs(), scenarioName);
        } else {
            CreateAndFillAnalyticsInfoForPlayerCommand(analyticsInfoPlayerCommand, *bodyBuilder,
                                                       applyRequest.ServerTimeMs(), scenarioName);
        }
    }
    NSound::FillAnalyticsInfoForSoundChangeIfExists(soundFrameName, *bodyBuilder);

    if (IsBedtimeTales(applyRequest.Input(), applyRequest.HasExpFlag(NExperiments::EXP_FAIRY_TALES_BEDTIME_TALES))) {
        bodyBuilder->AddSetTimerDirectiveForTurnOff(BEDTIME_TALES_PLAY_DURATION_SEC);
        bodyBuilder->AddClientActionDirective("tts_play_placeholder", NJson::TJsonValue());
    }

    // NOTE(ardulat): the music client doesn't fill state in response_body yet.
    // Make sure to clear reask_state in case you wish to use scenario_state here
    if (hasState) {
        scState.ClearReaskState();
        scState.ClearFairytaleReaskState();
        bodyBuilder->SetState(scState);
    }

    if (applyArgs.GetFairyTaleArguments().GetIsFairyTaleSubscenario()) {
        scState.SetProductScenarioName(MUSIC_FAIRYTALE_SCENARIO_NAME);
        bodyBuilder->SetState(scState);
    } else if (hasState && !playerFrame) {
        scState.ClearProductScenarioName();
        bodyBuilder->SetState(scState);
    }

    return std::move(builder).BuildResponse();
}

} // namespace NImpl

void TBassMusicContinueRenderHandle::Do(TScenarioHandleContext& ctx) const {
    // TODO(sparkle): make Y_ENSURE after graph release
    if (!ctx.ServiceCtx.HasProtobufItem(BASS_REQUEST_RTLOG_TOKEN_ITEM) ||
        !ctx.ServiceCtx.HasProtobufItem(BASS_RESPONSE_ITEM))
    {
        return;
    }

    const auto bassResponseBody = RetireBassRequest(ctx);
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    auto response = NImpl::MusicApplyRenderDoImpl(request, bassResponseBody, ctx.Ctx, nlgWrapper, ctx.Rng);
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NMusic
