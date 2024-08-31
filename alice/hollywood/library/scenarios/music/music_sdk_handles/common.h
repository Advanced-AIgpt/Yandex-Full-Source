#pragma once

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/music/music_resources.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/music_sdk_uri_builder/music_sdk_uri_builder.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

// subgraph methods
extern const TString SUBGRAPH_APPHOST_FLAG;
extern const TString PLAYLIST_SETDOWN_RESPONSE_FAILED_APPHOST_FLAG;

bool ShouldInvokeSubgraph(const TScenarioRunRequestWrapper& request);

// logic flow methods
bool IsRadioSupported(const TScenarioBaseRequestWrapper& request);

bool IsNavigatorSupported(const TScenarioBaseRequestWrapper& request);

bool IsSearchAppSupported(const TScenarioBaseRequestWrapper& request);

bool CanRenderDiv2Cards(const TScenarioBaseRequestWrapper& request);

bool IsUserAuthorized(const NScenarios::TRequestMeta& meta);

bool UserHasSubscription(const TScenarioRunRequestWrapper& request); // aka "has Ya.Plus"

// response methods and variables
NScenarios::TScenarioRunResponse CreateErrorResponse(
    TRTLogger& logger,
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    const TString& errorType,
    const TString& errorCode,
    const bool setIrrelevant = false);

NScenarios::TScenarioRunResponse CreateIrrelevantResponse(
    TRTLogger& logger,
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg);

NScenarios::TScenarioRunResponse CreateYouNeedAuthorizeResponse(
    TRTLogger& logger,
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg);

template<typename TResponse = NScenarios::TScenarioRunResponse>
TResponse ConstructFallbackToMusicVerticalResponse(
    TRTLogger& logger,
    const TScenarioBaseRequestWithInputWrapper& request,
    TNlgWrapper& nlg,
    const TFrame& musicPlayFrame,
    const bool isGeneral = false);

template<typename TResponse>
TResponse As(const NScenarios::TScenarioRunResponse& runResponse) {
    Y_ENSURE(runResponse.HasResponseBody());

    TResponse response;
    response.MutableResponseBody()->CopyFrom(runResponse.GetResponseBody());
    return response;
}

void AddOpenUriSuggest(TResponseBodyBuilder& bodyBuilder, TNlgWrapper& nlg, const TNlgData& nlgData,
                       const TStringBuf type, const TStringBuf directiveName);

void AddOpenUriDirective(TResponseBodyBuilder& bodyBuilder,
                         TString uri,
                         const TStringBuf name = "music_internal_player_play");

void TryAddSearchSuggest(const TClientInfo& clientInfo, const TScenarioInputWrapper& input,
                         TResponseBodyBuilder& bodyBuilder);

void AddOnboardingSuggest(TResponseBodyBuilder& bodyBuilder, TNlgWrapper& nlg, const TNlgData& nlgData);

bool IsVariousArtistsCase(const NJson::TJsonValue& bassState);

bool NeedShuffle(const TFrame& musicPlayFrame);

TMusicSdkUriBuilder::ERepeatMode GetRepeatMode(const TFrame& musicPlayFrame);

namespace NUsualPlaylist {

TString FindOwnerLogin(const NJson::TJsonValue& playlistObj);
TString FindOwnerUid(const NJson::TJsonValue& playlistObj);
bool FindOwnerIsVerified(const NJson::TJsonValue& playlistObj);
TString FindPlaylistId(const NJson::TJsonValue& playlistObj);

} // namespace NUsualPlaylist

namespace NSpecialPlaylist {

TString FindOwnerLogin(const NJson::TJsonValue& playlistObj);
TString FindOwnerUid(const NJson::TJsonValue& playlistObj);
bool FindOwnerIsVerified(const NJson::TJsonValue& playlistObj);
TMaybe<TString> FindPlaylistId(const NJson::TJsonValue& playlistObj);

} // namespace NSpecialPlaylist

namespace NPredefinedPlaylist {

TString FindOwnerLogin(const NJson::TJsonValue& playlistObj);
TString FindOwnerUid(const NJson::TJsonValue& playlistObj);
bool FindOwnerIsVerified(const NJson::TJsonValue& playlistObj);

} // namespace NPredefinedPlaylist

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
