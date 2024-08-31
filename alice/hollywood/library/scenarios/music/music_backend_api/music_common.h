#pragma once

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/fast_data.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/library/music/defs.h>
#include <alice/library/logger/logger.h>
#include <alice/library/util/rng.h>
#include <alice/megamind/protos/common/device_state.pb.h>

#include <apphost/api/service/cpp/service_context.h>

namespace NAlice::NHollywood::NMusic {

using THttpProxyRequestItemPair = std::pair<NAppHostHttp::THttpRequest, TString>;
using THttpProxyRequestItemPairs = TVector<THttpProxyRequestItemPair>;

const TString DISCOVERY_RADIO_STATION_ID = "mood:discovery";
const TString USER_RADIO_STATION_ID = "user:onyourwave";
const TString FORCHILDREN_STATION_ID = "genre:forchildren";

const TString HINT_FRAME_CONFIRM = "alice.proactivity.confirm";
const TString HINT_FRAME_DECLINE = "alice.proactivity.decline";

const TString CACHALOT_MUSIC_SCENARIO_STORAGE_TAG = "MusicScenario";

TMusicContext GetMusicContext(NAppHost::IServiceContext& ctx);

void TryAddStationPromoAttention(
    NJson::TJsonValue& stateJson,
    const TStationPromoFastData& stationPromoFastData,
    const TScenarioApplyRequestWrapper& applyRequest,
    IRng& rng);

void AddAudiobrandingAttention(NJson::TJsonValue& stateJson, const TMusicFastData& fastData, const TScenarioApplyRequestWrapper& applyRequest);

void AddMusicContext(NAppHost::IServiceContext& ctx, TMusicContext& mCtx);

void AddMusicThinClientFlag(NAppHost::IServiceContext& ctx);

void AddNeedDislikeTrackFlag(NAppHost::IServiceContext& ctx);

void AddTrackFullInfoFlag(NAppHost::IServiceContext& ctx);

void AddTrackSearchFlag(NAppHost::IServiceContext& ctx);

void AddPlaylistSearchFlag(NAppHost::IServiceContext& ctx);

void AddSpecialPlaylistFlag(NAppHost::IServiceContext& ctx);

void AddNoveltyAlbumSearchFlag(NAppHost::IServiceContext& ctx);

void AddFeedbackRadioStartedFlag(NAppHost::IServiceContext& ctx);

void AddFindTrackIdxFlag(NAppHost::IServiceContext& ctx);

TString GenerateRandomString(IRng& rng, size_t size);

bool IsGuestPlaybackMode(TBiometryOptions::EPlaybackMode playbackMode);

bool IsIncognitoPlaybackMode(TBiometryOptions::EPlaybackMode playbackMode);

bool IsMusicThinClientNextTrackCallback(const TStringBuf name);

bool IsMusicTrackPlayLifeCycleCallback(const TStringBuf name);

bool IsMusicThinClientRecoveryCallback(const TStringBuf name);

bool IsMusicOwnerOfAudioPlayer(const NAlice::TDeviceState& deviceState);

bool IsAudioPlayerPlaying(const NAlice::TDeviceState& deviceState);

void FillPlayerFeatures(TRTLogger& logger, const TScenarioRunRequestWrapper& request, THwFrameworkRunResponseBuilder& response);

TAtomicSharedPtr<IRequestMetaProvider> MakeGuestRequestMetaProvider(const NScenarios::TRequestMeta& meta,
                                                                    const TString& guestOAuthTokenEncrypted);

TAtomicSharedPtr<IRequestMetaProvider> MakeRequestMetaProviderFromMusicArgs(const NScenarios::TRequestMeta& meta,
                                                                            const TMusicArguments& applyArgs,
                                                                            bool isClientBiometryModeRequest);

TAtomicSharedPtr<IRequestMetaProvider> MakeRequestMetaProviderFromPlaybackBiometry(
    const NScenarios::TRequestMeta& meta,
    const TBiometryOptions& biometryOpts
);

TMusicRequestModeInfo MakeMusicRequestModeInfo(
    EAuthMethod authMethod,
    const TStringBuf ownerUserId,
    const TScenarioState& scState
);

TMusicRequestModeInfo MakeMusicRequestModeInfoFromMusicArgs(
    const TMusicArguments& applyArgs,
    const TScenarioState& scState,
    EAuthMethod authMethod,
    bool isClientBiometryModeRequest
);

void SetUpPlaybackModeUsingClientBiometryDeprecated(TRTLogger& logger,
                                          TScenarioState& scState,
                                          const NHollywood::TScenarioApplyRequestWrapper& request,
                                          const TMusicArguments& applyArgs,
                                          bool isClientBiometryModeApplyRequest);

void SetUpPlaybackModeUsingServerBiometryDeprecated(TRTLogger& logger,
                                          TScenarioState& scState,
                                          const NHollywood::TScenarioApplyRequestWrapper& request);

// Does HollywoodMusic need to request new content from music backend?
bool ShouldRequestNewContent(const TMusicContext& mCtx, const TMusicQueueWrapper& mq);

// Should HollywoodMusic except to receive new content from the queue or music backend?
bool ShouldReceiveNewContent(const TMusicContext& mCtx);

// Should HollywoodMusic return new content to the user?
bool ShouldReturnContentResponse(const TMusicContext& mCtx, const TMusicQueueWrapper& mq);

template<typename T>
std::pair<NScenarios::TAudioRewindDirective_EType, ui32> GetRewindArguments(const T& request) {
    const auto frameProto = request.Input().FindSemanticFrame(NAlice::NMusic::PLAYER_REWIND);
    Y_ENSURE(frameProto); // Because FindPlayerCommand has found this frame already

    const auto frame = TFrame::FromProto(*frameProto);
    TDuration dur;
    auto timeSlot = frame.FindSlot(::NAlice::NMusic::SLOT_TIME);
    if (timeSlot) {
        NJson::TJsonValue timeJson = JsonFromString(timeSlot->Value.AsString());
        dur += TDuration::Hours(1) * timeJson["hours"].GetDoubleRobust();
        dur += TDuration::Minutes(1) * timeJson["minutes"].GetDoubleRobust();
        dur += TDuration::Seconds(1) * timeJson["seconds"].GetDoubleRobust();
    } else {
        dur = TDuration::Seconds(10); // It is the "перемотай немного вперед/назад" case
    }

    ui32 rewindMs = dur.MilliSeconds();
    auto rewindType = NScenarios::TAudioRewindDirective_EType_Absolute;
    auto rewindTypeSlot = frame.FindSlot(NAlice::NMusic::SLOT_REWIND_TYPE);
    if (rewindTypeSlot) {
        const auto& rewindTypeStr = rewindTypeSlot->Value.AsString();
        if (rewindTypeStr == "backward") {
            rewindType = NScenarios::TAudioRewindDirective_EType_Backward;
        } else if (rewindTypeStr == "forward") {
            rewindType = NScenarios::TAudioRewindDirective_EType_Forward;
        } else {
            Y_ENSURE(false, TStringBuilder{} << "Unexpected value in " << NAlice::NMusic::SLOT_REWIND_TYPE << " slot: "
                                             << rewindTypeStr);
        }
    }

    if (!timeSlot && !rewindTypeSlot) {
        rewindType = NScenarios::TAudioRewindDirective_EType_Forward; // "перемотай немного" case
    }

    return {rewindType, rewindMs};
}

bool CommandSupportsFrontalLedImage(const TMusicArguments_EPlayerCommand playerCommand);

TString GetFrontalLedImage(const TMusicArguments_EPlayerCommand playerCommand);

NScenarios::TDirective BuildDrawLedScreenDirective(const TString& frontalLedImage);

bool IsOnYourWaveRequest(const TFrame& frame); // in run phase
bool IsOnYourWaveRequest(const TMusicArguments& args); // in continue phase

} // namespace NAlice::NHollywood::NMusic
