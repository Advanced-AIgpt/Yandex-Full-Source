#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_memento_scenario_data.pb.h>

#include <alice/hollywood/library/biometry/client_biometry.h>
#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/logger/logger.h>
#include <alice/library/music/common_special_playlists.h>

#include <alice/megamind/protos/guest/guest_options.pb.h>

#include <library/cpp/json/writer/json_value.h>

namespace NAlice::NHollywood::NMusic {

constexpr TStringBuf DEFAULT_COVER_SIZE = "200x200";
constexpr TStringBuf DEFAULT_COVER_URI = "https://avatars.mds.yandex.net/get-bass/469429/music_default_img/orig";

std::unique_ptr<IResponseBodyRenderer> ConstructBodyRenderer(const TScenarioRunRequestWrapper& runRequest, const bool forceNlg = false);
std::unique_ptr<IResponseBodyRenderer> ConstructBodyRenderer(const TScenarioApplyRequestWrapper& applyRequest, const bool forceNlg = false);

std::unique_ptr<IResponseBodyRenderer> ConstructBassBodyRenderer(const TScenarioApplyRequestWrapper& applyRequest);

// Radio requests methods
// { rup_stream, { rup_stream, mock_playlist } }
extern const THashMap<TString, TString> RADIO_STREAM_MOCKS;

// returns {{"genre", "rock"}, {"epoch", "nineties"}, ...}
TVector<std::pair<TStringBuf, TStringBuf>> GetAllRadioStationIdsPairs(const TFrame& musicPlayFrame, NAlice::TRTLogger& logger);

// {{"genre", "rock"}, {"epoch", "nineties"}, ...} -> {"genre:rock", "epoch:nineties", ...}
TVector<TString> ConvertRadioStationIdsPairsToIds(const TVector<std::pair<TStringBuf, TStringBuf>>& pairs);

TVector<TString> GetAllRadioStationIds(const TFrame& musicPlayFrame, NAlice::TRTLogger& logger);

// Bass search response methods
const NJson::TJsonValue* FindBlock(const NJson::TJsonValue& context, const TStringBuf name);
NJson::TJsonValue* FindMutableBlock(NJson::TJsonValue& context, const TStringBuf name);

const NJson::TJsonValue* TryGetArtist(const NJson::TJsonValue& webAnswer);

NJson::TJsonValue GetCommonPlaybackOptions(const TFrame& musicPlayFrame);

const TSlot* GetSpecialPlaylistOrNoveltySlot(const TMaybe<TFrame>& frame);

const TSlot* GetSpecialPlaylistOrNoveltySlot(const TFrame& frame);

bool IsSpecialPlaylistItem(const NAlice::NMusic::TSpecialPlaylistInfo& info);

const NAlice::NMusic::TSpecialPlaylistInfo::TPlaylist& GetSpecialPlaylistItem(
    const NAlice::NMusic::TSpecialPlaylistInfo& info);

const NAlice::NMusic::TSpecialPlaylistInfo::TAlbum&
GetSpecialAlbumItem(const NAlice::NMusic::TSpecialPlaylistInfo& info);

bool SupportsClientBiometry(const TScenarioApplyRequestWrapper &request);

bool ValidateGuestCredentials(TRTLogger& logger, const TMusicArguments_TGuestCredentials& guestCredentials);

template <class TRunRequest>
bool IsClientBiometryModeRunRequest(TRTLogger& logger,
                                    const TRunRequest& runRequest,
                                    const NAlice::TGuestOptions* guestOptions)
{
    if (!NHollywood::SupportsClientBiometry(runRequest)) {
        return false;
    }

    return guestOptions &&
           ValidateGuestOptionsDataSource(logger, *guestOptions) &&
           guestOptions->GetStatus() == TGuestOptions::Match;
}

bool IsIncognitoModeRunRequest(const NAlice::TGuestOptions& guestOptions);

bool IsClientBiometryModeApplyRequest(TRTLogger& logger, const TMusicArguments& applyArgs);

bool IsIncognitoModeApplyRequest(const TMusicArguments& applyArgs);

void ClearBiometryOptions(TScenarioState& scState);

void TryInitPlaybackContextBiometryOptions(TRTLogger& logger, TScenarioState& scState);

void FillEnvironmentState(
    NJson::TJsonValue& musicArguments,
    const NAlice::TEnvironmentState& environmentStateProto
);

void FillGuestCredentials(
    NJson::TJsonValue& musicArguments,
    const NAlice::TGuestOptions& guestOptions
);

void FillFairyTaleInfo(
    NJson::TJsonValue& musicArguments,
    const NJson::TJsonValue& bassState,
    const NHollywood::TScenarioRunRequestWrapper& runRequest
);

void FillMusicSearchResult(
    TRTLogger& logger,
    NJson::TJsonValue& musicArguments,
    const NJson::TJsonValue& bassState,
    const TMaybe<TFrame>& musicPlayFrame,
    const TString& requesterUserId,
    const bool supportsPlaylists,
    bool* const isSpecialAlbumPtr = nullptr
);

bool IsUgcTrackId(const TStringBuf id);

NMusic::TMusicScenarioMementoData ParseMementoData(const NHollywood::TScenarioApplyRequestWrapper& applyRequest);

void AddAttentionToJsonState(NJson::TJsonValue& stateJson, const TString& attentionType);

TString ConstructCoverUri(TStringBuf coverUriTemplate);

} // namespace NAlice::NHollywood::NMusic
