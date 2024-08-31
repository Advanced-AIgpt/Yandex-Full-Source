#include "run_render_handle.h"

#include "commands.h"
#include "common.h"
#include "onboarding.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/bass_adapter/bass_stats.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/metrics/metrics.h>
#include <alice/hollywood/library/multiroom/multiroom.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/scenarios/music/analytics_info/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/intents.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/semantic_frames.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/multiroom.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/unauthorized_user_directives.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>
#include <alice/library/metrics/histogram.h>
#include <alice/library/music/common_special_playlists.h>
#include <alice/library/music/defs.h>
#include <alice/megamind/library/util/slot.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <alice/protos/data/scenario/data.pb.h>

#include <library/cpp/json/json_reader.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMusic {

namespace NImpl {

namespace {

constexpr TStringBuf DATA = "data";
constexpr TStringBuf ERROR = "error";
constexpr TStringBuf FEATURES_DATA = "features_data";
constexpr TStringBuf IS_PLAYER_COMMAND = "is_player_command";
constexpr TStringBuf SEARCH_RESULT = "search_result";
constexpr TStringBuf SEARCH_TEXT = "search_text";
constexpr TStringBuf CONTEXT = "context";

const TVector<double> NORM_COUNT_INTERVALS = {1,  2,  3,  4,  5,  6,  7,  8,  9,   10,  11,
                                              12, 15, 18, 22, 30, 40, 50, 70, 100, 200, 300};

const THashSet<TStringBuf> IRRELEVANT_ERROR_CODES = {
    TStringBuf("irrelevant_player_command"),
    TStringBuf("nothing_is_playing")
};
constexpr TStringBuf MUSIC_NOT_FOUND_ERROR_CODE = "music_not_found";
constexpr TStringBuf MUSIC_UNAVAILABLE_FOR_LEGAL_REASONS_CODE = "unavailable_for_legal_reasons";

constexpr uint64_t SUBSCRIPTION_REGION_ID_NO_SUBSCRIPTION = 0;
constexpr uint64_t SUBSCRIPTION_REGION_ID_RUSSIA = 225; // https://geoadmin.yandex-team.ru/#region:225

const NJson::TJsonValue* GetSlotFromBassResponse(const NJson::TJsonValue& bassResponse, const TStringBuf name) {
    return FindIfPtr(
        bassResponse[CONTEXT]["form"]["slots"].GetArray(),
        [name](const NJson::TJsonValue& slot) {
            return slot["name"] == name;
        }
    );
}

void AddFeatureIntent(THwFrameworkRunResponseBuilder& builder, const NJson::TJsonValue& bassResponse) {
    // TODO(a-square): remove MM's postclassifier's dependency on the intent feature for non-VINS scenarios
    if (const auto* frameSlot = GetSlotFromBassResponse(bassResponse, ORIGINAL_INTENT)) {
        builder.SetFeaturesIntent((*frameSlot)["value"].GetString());
    }
}

bool TryReplaceUnavailableForLegalReasonsError(NJson::TJsonValue& bassResponse, const NHollywood::TScenarioRunRequestWrapper& runRequest) {
    const auto* blackBoxUserInfo = GetUserInfoProto(runRequest);
    if (!blackBoxUserInfo) {
        // user doesn't have blackbox info (this is a rare case)
        return false;
    }

    if (blackBoxUserInfo->GetMusicSubscriptionRegionId() == SUBSCRIPTION_REGION_ID_NO_SUBSCRIPTION ||
        blackBoxUserInfo->GetMusicSubscriptionRegionId() == SUBSCRIPTION_REGION_ID_RUSSIA)
    {
        // user doesn't have a subscription or doesn't have a foreign subscription
        return false;
    }

    auto* errorBlock = FindMutableBlock(bassResponse, ERROR);
    if (!errorBlock) {
        // user doesn't have error block, don't replace the error
        return false;
    }

    auto& code = (*errorBlock)[DATA]["code"];
    if (code.GetString() != MUSIC_NOT_FOUND_ERROR_CODE) {
        // user doesn't have `music_not_found` error, don't replace the error
        return false;
    }

    // replace the error
    code.SetValue(MUSIC_UNAVAILABLE_FOR_LEGAL_REASONS_CODE);
    return true;
}

bool HasLikeDislikeAction(const NJson::TJsonValue& bassResponse, bool& isLike) {
    if (const auto* actionSlot = GetSlotFromBassResponse(bassResponse, NAlice::NMusic::SLOT_ACTION_REQUEST)) {
        return IsLikeDislikeAction((*actionSlot)["value"].GetString(), isLike);
    }
    return false;
}

class TMusicRenderImpl final {
public:
    TMusicRenderImpl(const TScenario::THandleBase& handle, const NHollywood::TScenarioRunRequestWrapper& runRequest,
                     const NJson::TJsonValue& bassResponse, TScenarioHandleContext& ctx, TNlgWrapper& nlgWrapper)
        : Handle{handle}
        , RunRequest{runRequest}
        , BassResponse{bassResponse}
        , NlgWrapper{nlgWrapper}
        , Ctx{ctx}
        , Logger{ctx.Ctx.Logger()}
        , Sensors{ctx.Ctx.GlobalContext().Sensors()}
    {
    }

    std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse> Do();

private:
    THistogramScope MakeHistogramScope(EScenarioStage stage) {
        return THistogramScope(Sensors, ScenarioStageTime(Handle, stage), THistogramScope::ETimeUnit::Millis);
    }

    void AddNormalizationSensors(ui32 normalizations) {
        NMonitoring::TLabels labels = ScenarioLabels(Handle);
        labels.Add("name", "normalizations_count");
        Sensors.AddHistogram(labels, normalizations, NORM_COUNT_INTERVALS);
    }

    ui32 FillFeaturesData(THwFrameworkRunResponseBuilder& builder, const NJson::TJsonValue& data);

    void RenderResponseBody(const NHollywood::TScenarioRunRequestWrapper& request, THwFrameworkRunResponseBuilder& builder,
                            const NJson::TJsonValue& bassResponse);

private:
    const TScenario::THandleBase& Handle;
    const NHollywood::TScenarioRunRequestWrapper& RunRequest;
    const NJson::TJsonValue& BassResponse;
    TNlgWrapper& NlgWrapper;
    TScenarioHandleContext& Ctx;
    TRTLogger& Logger;
    NMetrics::ISensors& Sensors;
};

ui32 TMusicRenderImpl::FillFeaturesData(THwFrameworkRunResponseBuilder& builder, const NJson::TJsonValue& data) {
    if (!data.IsDefined()) {
        LOG_WARNING(Logger) << "Features data not found in the BASS response";
        return 0;
    }
    LOG_INFO(Logger) << "Filling feature data";
    auto scope = MakeHistogramScope(EScenarioStage::FillFeatures);
    return builder.FillMusicFeatures(data[SEARCH_TEXT].GetString(),
                                     data[SEARCH_RESULT],
                                     data[IS_PLAYER_COMMAND].GetBoolean());
}

bool HasUnknownFairyTaleAttention(const NJson::TJsonValue& bassResponse) {
    const auto& blocks = bassResponse["blocks"].GetArray();
    for (const auto& block: blocks) {
        if (block["type"].GetString() == "attention" &&
            block["attention_type"].GetString() == "unknown_fairy_tale")
        {
            return true;
        }
    }
    return false;
}

void TMusicRenderImpl::RenderResponseBody(const NHollywood::TScenarioRunRequestWrapper& request,
                                          THwFrameworkRunResponseBuilder& builder, const NJson::TJsonValue& bassResponse) {
    LOG_INFO(Logger) << "Rendering the response body";
    {
        TScenarioState scState;
        const bool hasState = ReadScenarioState(request.BaseRequestProto(), scState);
        TryInitPlaybackContextBiometryOptions(Logger, scState);

        TString analyticsScenarioName = Default<TString>();
        TString intent = Default<TString>();
        LOG_DEBUG(Logger) << "BASS response: " << NJson::WriteJson(bassResponse);
        if (hasState && FindPlayerFrame(request)) {
            analyticsScenarioName = scState.GetProductScenarioName();
        } else if (HasUnknownFairyTaleAttention(bassResponse)) {
            analyticsScenarioName = MUSIC_FAIRYTALE_SCENARIO_NAME;
            intent = MUSIC_FAIRYTALE_INTENT;
        }

        auto scope = MakeHistogramScope(EScenarioStage::BassRender);
        TBassResponseRenderer bassRenderer(RunRequest, RunRequest.Input(), builder, Logger,
                                           /* suggestAutoAction= */ false);
        bassRenderer.Render(TEMPLATE_MUSIC_PLAY, "render_result", bassResponse, intent, analyticsScenarioName);
    }

    if (const auto* featuresBlock = FindBlock(bassResponse, FEATURES_DATA)) {
        ui32 normalizations = FillFeaturesData(builder, (*featuresBlock)[DATA]);
        AddNormalizationSensors(normalizations);
    } else {
        LOG_WARNING(Logger) << "Features data not found in the VINS-like BASS response";
    }

    if (const auto* errorBlock = FindBlock(bassResponse, ERROR)) {
        LOG_INFO(Logger) << "Found the error block";
        const auto& code = (*errorBlock)[DATA]["code"].GetString();
        if (IRRELEVANT_ERROR_CODES.contains(code)) {
            LOG_INFO(Logger) << "Found the " << code << " error, setting the Irrelevant flag";
            builder.SetIrrelevant();
        }
    }

    {
        auto scope = MakeHistogramScope(EScenarioStage::FillAnalyticsInfo);
        FillAnalyticsInfoMusicEvent(Logger, bassResponse, builder.GetResponseBodyBuilder(), RunRequest);
    }
}

// Use thin client for on demand content & playlists, but only if multiroom is not enabled
bool IsThinClientOnDemandOrPlaylistRequest(TRTLogger& logger, const TMaybe<TFrame>& frame,
                                           const NJson::TJsonValue& webAnswer, bool allowPlaylist,
                                           bool hasForbiddenMultiroom) {
    if (!frame) {
        LOG_INFO(logger) << "IsThinClientOnDemandOrPlaylistRequest=false, reason: no frame";
        return false;
    }
    using namespace NAlice::NMusic;
    const bool hasSearch = frame->FindSlot(SLOT_SEARCH_TEXT);

    const bool hasPlaylist = frame->FindSlot(SLOT_PLAYLIST) ||
                             (hasSearch && webAnswer["type"].GetString() == "playlist");
    const bool hasSpecialPlaylist = frame->FindSlot(SLOT_SPECIAL_PLAYLIST);
    const bool hasNovelty = frame->FindSlot(SLOT_NOVELTY);
    const bool hasPersonality = frame->FindSlot(SLOT_PERSONALITY);
    const bool hasSpecialAnswerInfo = frame->FindSlot(SLOT_SPECIAL_ANSWER_INFO);

    const bool hasStream = frame->FindSlot(SLOT_STREAM);

    LOG_INFO(logger) << "hasForbiddenMultiroom=" << hasForbiddenMultiroom << ", allowPlaylist=" << allowPlaylist
                     << ", hasSearch=" << hasSearch << ", hasPlaylist=" << hasPlaylist
                     << ", hasSpecialPlaylist=" << hasSpecialPlaylist << ", hasNovelty=" << hasNovelty
                     << ", hasPersonality=" << hasPersonality << ", webAnswer.IsDefined()=" << webAnswer.IsDefined()
                     << ", hasSpecialAnswerInfo=" << hasSpecialAnswerInfo
                     << ", hasStream=" << hasStream;

    if (allowPlaylist) {
        if(hasPlaylist || hasSpecialPlaylist || hasNovelty || hasPersonality) {
            return !hasSpecialAnswerInfo && !hasForbiddenMultiroom;
        }
    }

    return
        hasSearch &&
        !hasPlaylist &&
        !hasSpecialPlaylist &&
        !(hasNovelty && !webAnswer.IsDefined()) &&
        !(hasPersonality && !webAnswer.IsDefined()) &&
        !hasSpecialAnswerInfo &&
        !hasForbiddenMultiroom &&
        !hasStream;
}

bool IsThinFairyTaleRequest(const TScenarioRunRequestWrapper& request) {
    const bool isThinPlaylistRequest = request.Input().FindSemanticFrame(MUSIC_PLAY_FAIRYTALE_FRAME) != nullptr &&
        request.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_FAIRY_TALE_PLAYLISTS);
    const bool isThinOndemandRequest = request.Input().FindSemanticFrame(ALICE_FAIRY_TALE_ONDEMAND_FRAME) != nullptr &&
        request.HasExpFlag(NExperiments::EXP_HW_MUSIC_FAIRY_TALES_ENABLE_ONDEMAND) &&
        request.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_FAIRY_TALE_ONDEMAND);
    return isThinPlaylistRequest || isThinOndemandRequest;
}

bool GetShouldLikeDislikeArtist(const TScenarioRunRequestWrapper& request, const TOnboardingState& onboardingState) {
    if (IsAskingFavorite(onboardingState)) {
        // Checked in run_prepare
        Y_ASSERT(request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ONBOARDING));
        // Only asking artist is supported for now
        Y_ENSURE(GetAskingFavorite(onboardingState).GetType() == TOnboardingState::TAskingFavorite::Artist);
        return true;
    }
    // Checked in run_prepare
    Y_ASSERT(request.HasExpFlag(NExperiments::EXP_HW_MUSIC_COMPLEX_LIKE_DISLIKE));
    return false;
}

void FillAlbumTarget(NJson::TJsonValue& likeDislikeRequest, const NJson::TJsonValue& album) {
    auto& albumTarget = likeDislikeRequest["album_target"];
    albumTarget["id"] = album["id"].GetString();
    albumTarget["title"] = album["title"].GetString();
}

void FillArtistTarget(NJson::TJsonValue& likeDislikeRequest, const NJson::TJsonValue& artist) {
    auto& artistTarget = likeDislikeRequest["artist_target"];
    artistTarget["id"] = artist["id"].GetString();
    artistTarget["name"] = artist["name"].GetString();
}

void FillTrackTarget(NJson::TJsonValue& likeDislikeRequest, const NJson::TJsonValue& track) {
    auto& trackTarget = likeDislikeRequest["track_target"];
    trackTarget["id"] = track["id"].GetString();
    trackTarget["title"] = track["title"].GetString();
    if (const auto* albumId = track.GetValueByPath("album.id")) {
        // Needed for track like/dislike directives
        trackTarget["album_id"] = albumId->GetString();
    }
}

bool TryFillLikeDislikeRequest(TRTLogger& logger, const TScenarioRunRequestWrapper& request, const TOnboardingState& onboardingState,
                               NJson::TJsonValue& likeDislikeRequest, const NJson::TJsonValue& webAnswer, const bool isLike, TString& failurePhrase) {
    const auto onFail = [&logger, &failurePhrase](const auto logText, const auto phrase) {
        LOG_INFO(logger) << logText;
        failurePhrase = phrase;
        return false;
    };
    const auto onSuccess = [&logger](const auto logText) {
        LOG_INFO(logger) << logText;
        return true;
    };

    const bool shouldTargetArtist = GetShouldLikeDislikeArtist(request, onboardingState);
    if (shouldTargetArtist) {
        LOG_INFO(logger) << "Asking for a favorite artist";
    }
    LOG_INFO(logger) << "Got a " << (isLike ? "like" : "dislike") << " request";

    if (!webAnswer.IsDefined()) {
        return onFail("No music web answer", shouldTargetArtist ? "artist__not_found" : "nothing_found");
    }

    likeDislikeRequest["is_like"] = isLike;
    const auto& objType = webAnswer["type"].GetString();
    if (objType == "artist") {
        FillArtistTarget(likeDislikeRequest, webAnswer);
        return onSuccess("Found an artist");
    }
    if (shouldTargetArtist) {
        if (const auto* artist = TryGetArtist(webAnswer);
            artist && !request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ONBOARDING_NO_ARTIST_FROM_TRACK)
        ) {
            FillArtistTarget(likeDislikeRequest, *artist);
            return onSuccess("Inferred an artist from a track");
        }
        return onFail("No artist in web answer", "artist__not_found");
    }
    if (objType == "track") {
        FillTrackTarget(likeDislikeRequest, webAnswer);
        return onSuccess("Found a track");
    }
    if (objType == "album" && isLike) {
        FillAlbumTarget(likeDislikeRequest, webAnswer);
        return onSuccess("Found an album");
    }
    return onFail(
        "Web answer has an unsupported object type",
        TString::Join("unsupported_found__", isLike ? "like" : "dislike"));
}

} // namespace

[[nodiscard]] std::unique_ptr<TScenarioRunResponse> TMusicRenderImpl::Do() {

    TScenarioState scState;
    const bool hasState = ReadScenarioState(RunRequest.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(Logger, scState);
    const bool forceNlg = FindBlock(BassResponse, ERROR) != nullptr;

    THwFrameworkRunResponseBuilder builder(Ctx, &NlgWrapper, ConstructBodyRenderer(RunRequest, forceNlg));

    // TODO(a-square): generalize so that it's easy for other scenarios to reuse BASS continuations
    // TODO(a-square): is it possible that no continuation is present in the prepare response?
    const TStringBuf continuationName = BassResponse["ObjectTypeName"].GetString();
    if (!continuationName) {
        LOG_INFO(Logger) << "Got no continuation, assuming a VINS-like run response";
        ProcessBassResponseUpdateSensors(Logger, Sensors, BassResponse, "music", "run");
        RenderResponseBody(RunRequest, builder, BassResponse);
        AddFeatureIntent(builder, BassResponse);

        TryAddUnauthorizedUserDirectivesForVinsRunResponse(RunRequest, BassResponse, *builder.GetResponseBodyBuilder());

        if (hasState) {
            auto* bodyBuilder = builder.GetResponseBodyBuilder();
            bodyBuilder->SetState(scState);
        }
        return std::move(builder).BuildResponse();
    }

    LOG_DEBUG(Logger) << "Unpacking the continuation state";
    const TString& stateStr = BassResponse["State"].GetStringSafe();
    NJson::TJsonValue state;
    {
        auto scope = MakeHistogramScope(EScenarioStage::UnpackJson);
        state = JsonFromString(stateStr);
    }
    LOG_DEBUG(Logger) << "Unpacked the continuation state";

    AddFeatureIntent(builder, state);
    if (TryReplaceUnavailableForLegalReasonsError(state, RunRequest)) {
        LOG_INFO(Logger) << "Replaced \"music_not_found\" error with \"unavailable_for_legal_reasons\" error";
    }

    if (continuationName == TStringBuf("TCompletedContinuation")) {
        LOG_INFO(Logger) << "Got a TCompletedContinuation";
        ProcessBassResponseUpdateSensors(Logger, Sensors, state, "music", "run");
        RenderResponseBody(RunRequest, builder, state);

        if (hasState) {
            auto* bodyBuilder = builder.GetResponseBodyBuilder();
            bodyBuilder->SetState(scState);
        }
        return std::move(builder).BuildResponse();
    }

    if (continuationName == MUSIC_CONTINUATION) {
        LOG_INFO(Logger) << "Got a " << MUSIC_CONTINUATION;
        Y_ENSURE(!BassResponse["IsFinished"].GetBoolean());

        if (const auto& featuresData = state[FEATURES_DATA]; featuresData.IsDefined()) {
            ui32 normalizations = FillFeaturesData(builder, featuresData);
            AddNormalizationSensors(normalizations);
        }

        // NOTE(a-square): the music scenario doesn't need to deal with scenario_analytics_info
        // because it is only filled in the run phase and preserved along with other blocks in the apply phase.
        // A scenario that augments analytics info in the apply phase, by contrast, would need
        // to reconstruct the analytics info builder from the block and then delete the block inside BASS

        LOG_INFO(Logger) << "Packing apply arguments";
        auto scope = MakeHistogramScope(EScenarioStage::PackApplyArgs);
        NJson::TJsonValue musicArguments;

        TMaybe<TFrame> musicPlayFrame;
        TMaybe<TString> fixlistValue;

        if (const auto fixlistFrameProto = RunRequest.Input().FindSemanticFrame(MUSIC_PLAY_FIXLIST_FRAME)) {
            const auto fixlistFrame = TFrame::FromProto(*fixlistFrameProto);
            const auto slot = fixlistFrame.FindSlot(NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO);
            Y_ENSURE(slot); // otherwise it would have ended in run-prepare
            fixlistValue = slot->Value.AsString();
            const auto fixlist = NJson::ReadJsonFastTree(*fixlistValue);
            musicPlayFrame = CreateSpecialAnswerFrame(fixlist, RunRequest.Interfaces().GetHasAudioClient());
        }

        const auto musicPlayFrameProto = RunRequest.Input().FindSemanticFrame(MUSIC_PLAY_FRAME);
        if (musicPlayFrameProto && !musicPlayFrame.Defined()) {
            musicPlayFrame = TFrame::FromProto(*musicPlayFrameProto);
        }

        const auto& webAnswer = state["apply_arguments"]["web_answer"];
        auto* blackBoxUserInfo = GetUserInfoProto(RunRequest);
        Y_ENSURE(blackBoxUserInfo);

        bool isFairyTaleRequest =
            (RunRequest.Input().FindSemanticFrame(MUSIC_PLAY_FAIRYTALE_FRAME) != nullptr ||
             RunRequest.Input().FindSemanticFrame(ALICE_FAIRY_TALE_ONDEMAND_FRAME) != nullptr)
            && RunRequest.Input().FindSemanticFrame(MUSIC_PLAY_FIXLIST_FRAME) == nullptr; // fixlists ARE NOT fairy tales!

        bool allowPlaylist = RunRequest.HasExpFlag(EXP_HW_MUSIC_THIN_CLIENT_PLAYLIST) ||
            (RunRequest.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_FAIRY_TALE_PLAYLISTS) && isFairyTaleRequest) ||
            (RunRequest.HasExpFlag(NExperiments::EXP_HW_MUSIC_FAIRY_TALES_ENABLE_ONDEMAND) &&
                RunRequest.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_FAIRY_TALE_ONDEMAND) &&
                isFairyTaleRequest &&
                webAnswer.IsDefined());
        bool hasForbiddenMultiroom = HasForbiddenMultiroom(Logger, RunRequest, *musicPlayFrame);
        bool isThinClientOnDemandOrPlaylistRequest =
            IsThinClientOnDemandOrPlaylistRequest(Logger, musicPlayFrame, webAnswer, allowPlaylist, hasForbiddenMultiroom)
            && (!isFairyTaleRequest || IsThinFairyTaleRequest(RunRequest));

        LOG_INFO(Logger) << "isThinClientOnDemandOrPlaylistRequest = " << isThinClientOnDemandOrPlaylistRequest;

        bool isNewContentRequestedByUser = false;

        bool hasLikeAction;
        const auto isLikeDislikeRequest = musicPlayFrame && HasLikeDislikeAction(state, hasLikeAction);
        LOG_INFO(Logger) << "LikeDislikeRequest = " << isLikeDislikeRequest;

        FillFairyTaleInfo(musicArguments, state, RunRequest);

        TScenarioState scState;
        ReadScenarioState(RunRequest.BaseRequestProto(), scState);
        TryInitPlaybackContextBiometryOptions(Logger, scState);

        const bool hasNeedSimilar = musicPlayFrame.Defined() && musicPlayFrame->FindSlot(NAlice::NMusic::SLOT_NEED_SIMILAR);

        const auto* guestOptions = GetGuestOptionsProto(RunRequest);
        auto isClientBiometryMode = IsClientBiometryModeRunRequest(Logger, RunRequest, guestOptions);

        if (isLikeDislikeRequest) {
            LOG_INFO(Logger) << "Handling complex like/dislike";

            musicArguments["execution_flow_type"] =
                TMusicArguments_EExecutionFlowType_Name(TMusicArguments_EExecutionFlowType_ComplexLikeDislike);

            TString failurePhrase;
            if (!TryFillLikeDislikeRequest(Logger, RunRequest, scState.GetOnboardingState(),
                                           musicArguments["complex_like_dislike_request"],
                                           webAnswer, hasLikeAction, failurePhrase)) {
                auto paramsBuilder = TOnboardingResponseParamsBuilder{}
                    .SetTryReask(true)
                    .SetTryNextMasterOnboardingStage(true);
                FillOnboardingResponse(builder, Ctx, RunRequest, scState, failurePhrase, paramsBuilder.Build());
                return std::move(builder).BuildResponse();
            }
        } else if (isThinClientOnDemandOrPlaylistRequest && RunRequest.Interfaces().GetHasAudioClient() &&
            RunRequest.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT) &&
            (!hasNeedSimilar || (hasNeedSimilar && IsThinRadioSupported(RunRequest))))
        {
            isNewContentRequestedByUser = true;
            const auto& requesterUserId = isClientBiometryMode ? guestOptions->GetYandexUID() : blackBoxUserInfo->GetUid();
            FillMusicSearchResult(
                Logger,
                musicArguments,
                state,
                musicPlayFrame,
                requesterUserId,
                RunRequest.HasExpFlag(EXP_HW_MUSIC_THIN_CLIENT_PLAYLIST)
            );

            musicArguments["execution_flow_type"] =
                TMusicArguments_EExecutionFlowType_Name(TMusicArguments_EExecutionFlowType_ThinClientDefault);
        } else {
            musicArguments["bass_scenario_state"] = stateStr;
            musicArguments["execution_flow_type"] = TMusicArguments_EExecutionFlowType_Name(
                TMusicArguments_EExecutionFlowType_BassDefault);
        }
        musicArguments["puid"] = GetUid(RunRequest);
        musicArguments["account_status"]["uid"] = blackBoxUserInfo->GetUid();
        musicArguments["account_status"]["has_plus"] = blackBoxUserInfo->GetHasYandexPlus();
        musicArguments["account_status"]["has_music_subscription"] = blackBoxUserInfo->GetHasMusicSubscription();
        if (const auto* iotUserInfoPtr = RunRequest.GetDataSource(EDataSourceType::IOT_USER_INFO)) {
            musicArguments["has_smart_devices"] = !iotUserInfoPtr->GetIoTUserInfo().GetDevices().empty();
        }

        if (fixlistValue) {
            musicArguments["fixlist"] = *fixlistValue;
        }

        if (isClientBiometryMode) {
            FillGuestCredentials(musicArguments, *guestOptions);
        }

        {
            auto scope = MakeHistogramScope(EScenarioStage::PackProto);

            LOG_DEBUG(Logger) << "Pre JsonToProto MusicArguments: " << JsonToString(musicArguments);
            auto applyArgs = JsonToProto<TMusicArguments>(musicArguments, /* validateUtf8= */ true,
                                                          /* ignoreUnknownFields= */ true);
            applyArgs.MergeFrom(MakeMusicArguments(Logger, RunRequest, applyArgs.GetExecutionFlowType(), isNewContentRequestedByUser));
            LOG_DEBUG(Logger) << "Post JsonToProto MusicArguments: " << JsonStringFromProto(applyArgs);
            if (isLikeDislikeRequest) {
                FillLikeDislikeResponse(builder, Ctx, RunRequest, scState, applyArgs);
            } else {
                builder.SetContinueArguments(std::move(applyArgs));
            }
        }

        return std::move(builder).BuildResponse();
    }

    ythrow yexception() << "Unknown continuation: " << continuationName;
}

std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse>
MusicRenderDoImpl(const TScenario::THandleBase& handle, const NHollywood::TScenarioRunRequestWrapper& runRequest,
                  const NJson::TJsonValue& bassResponse, TScenarioHandleContext& ctx, TNlgWrapper& nlgWrapper) {
    return NImpl::TMusicRenderImpl{handle, runRequest, bassResponse, ctx, nlgWrapper}.Do();
}

} // namespace NImpl

TBassMusicRenderHandle::TBassMusicRenderHandle() {
    // NLU warmup, TRequestNormalizer is used by the music features calculator,
    // see flame graphs in MEGAMIND-873.
    //
    // XXX(a-square): Consider making this singleton an actual global variable
    // so that the initialization is always eager.
    (void)Singleton<NNlu::TRequestNormalizer>();
}

void TBassMusicRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto runRequest = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request(runRequest, ctx.ServiceCtx);

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    if (!ctx.ServiceCtx.HasProtobufItem(BASS_RESPONSE_ITEM))
    {
        LOG_INFO(ctx.Ctx.Logger()) << "Nothing to render in music run_render_handle";
        return;
    }
    const auto bassResponseBody = RetireBassRequest(ctx);

    auto response = NImpl::MusicRenderDoImpl(*this, request, bassResponseBody, ctx, nlgWrapper);
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NMusic
