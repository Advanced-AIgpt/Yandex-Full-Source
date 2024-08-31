#pragma once

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/nlg/nlg_data.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/library/client/client_features.h>
#include <alice/library/util/rng.h>

#include <library/cpp/json/writer/json_value.h>
#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

// old nlg data builder - https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/base_music_answer.cpp?rev=r8469411#L34-45
// builds json for TNlgData based on content
class TContentJsonInfoBuilder {
public:
    TContentJsonInfoBuilder(const TStringBuf answerType,
                            const NScenarios::TInterfaces& interfaces,
                            const TClientInfo& clientInfo,
                            const NJson::TJsonValue& webAnswer);

    TMaybe<NJson::TJsonValue> Build();

private:
    const TStringBuf AnswerType_;
    const NScenarios::TInterfaces& Interfaces_;
    const TClientInfo& ClientInfo_;
    const NJson::TJsonValue& WebAnswer_;
};

class TNlgDataBuilder {
public:
    TNlgDataBuilder(NAlice::TRTLogger& logger,
                    const TScenarioBaseRequestWrapper& request);

    TNlgDataBuilder(NAlice::TRTLogger& logger,
                    const TScenarioApplyRequestWrapper& request);

    TNlgDataBuilder(NAlice::TRTLogger& logger,
                    const TScenarioBaseRequestWrapper& request,
                    const TFrame& musicPlayFrame);

    TNlgDataBuilder(NAlice::TRTLogger& logger,
                    const TScenarioApplyRequestWrapper& request,
                    const TFrame& musicPlayFrame);

    void AddAttention(const TStringBuf attention);
    bool HasAttention(const TStringBuf attention) const;
    void AddMusicSdkUri(const TContentId& contentId,
                        const TString& musicSdkUri,
                        const TStringBuf logId = "music_play_alicesdk");
    void AddMusicVerticalUri(const TString& musicVerticalUri);
    void SetLikesCount(int likesCount);
    void ReplaceAnswerSlot(NJson::TJsonValue&& value);
    void CopyToAnswerSlotWithShuffle(NAlice::IRng& rng, const TStringBuf key,
                                     const TVector<NJson::TJsonValue>& datas);
    void AddLyricsSearchUri(const TString& artistName, const TString& title);
    void AddBackgroundGradient(const TString& mainColor, const TString& secondColor);
    void SetPlaylistOwnerLogin(const TString& playlistOwnerLogin);

    void SetErrorCode(const TString& errorCode);

    TStringBuf GetAnswerUri() const;

    const TNlgData& GetNlgData() const;

private:
    const TScenarioBaseRequestWrapper& Request_;
    TMusicArguments const* ContinueArgs_;
    TNlgData NlgData_;
    NJson::TJsonValue& AnswerSlot_;
    TMaybe<TString> PlaylistOwnerLogin_; // for playlists' redirect uri
};

TMaybe<TStringBuf> TryGetCoverUriFromBassState(const NJson::TJsonValue& bassState);

namespace NUsualPlaylist {

TString ConstructCoverUri(const NJson::TJsonValue& webAnswer);

} // namespace NUsualPlaylist

namespace NSpecialPlaylist {

TString ConstructCoverUri(const NJson::TJsonValue& webAnswer);

} // namespace NSpecialPlaylist

namespace NPredefinedPlaylist {

TString ConstructCoverUri(const NJson::TJsonValue& webAnswer);

} // namespace NPredefinedPlaylist

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
