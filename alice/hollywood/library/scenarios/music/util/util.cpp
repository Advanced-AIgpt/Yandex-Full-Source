#include "util.h"

#include <alice/hollywood/library/capability_wrapper/capability_wrapper.h>
#include <alice/hollywood/library/crypto/aes.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/semantic_frames.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/common/origin.pb.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>
#include <alice/library/music/defs.h>
#include <alice/library/music/fairytale_linear_albums.h>
#include <alice/library/util/charchecker.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf BLOCKS = "blocks";
constexpr TStringBuf DATA = "data";
constexpr TStringBuf TYPE = "type";
constexpr TStringBuf CONTEXT = "context";
constexpr TStringBuf SCENARIO_ANALYTICS_INFO = "scenario_analytics_info";

bool IsNlgDisabled(const TScenarioRequestWrapper auto& request) {
    if (const auto frameProto = request.Input().FindSemanticFrame(MUSIC_PLAY_FRAME)) {
        const auto musicPlayFrame = TFrame::FromProto(*frameProto);
        if (const auto slot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_DISABLE_NLG)) {
            if (auto maybeValue = slot->Value.template As<bool>()) {
                return *maybeValue;
            }
        }
    }
    return false;
}

TMaybe<TRepeatAfterMeResponseBodyRenderer::TRedirectData> TryConstructRedirectData(const TScenarioRequestWrapper auto& request,
                                                                                   const TMusicArguments* applyArgs = nullptr) {
    const auto& baseRequest = request.BaseRequestProto();
    if (!baseRequest.HasOrigin()) {
        return Nothing();
    }

    const auto& origin = baseRequest.GetOrigin();
    TString puid;
    if (applyArgs) {
        puid = applyArgs->GetAccountStatus().GetUid();
    } else {
        if constexpr (requires { request.GetDataSource; }) {
            puid = TString{GetUid(request)};
        }
    }
    TRepeatAfterMeResponseBodyRenderer::TRedirectData data{
        .Puid = std::move(puid),
        .DeviceId = origin.GetDeviceId(),
        .Ttl = 5,
    };
    return data;
}

std::unique_ptr<IResponseBodyRenderer> ConstructCommonBodyRenderer(const TScenarioRequestWrapper auto& request,
                                                                   const TMusicArguments* applyArgs = nullptr,
                                                                   const bool forceNlg = false) {
    if (!forceNlg && IsNlgDisabled(request)) {
        return std::make_unique<TSilentResponseBodyRenderer>();
    }
    if (auto redirectData = TryConstructRedirectData(request, applyArgs)) {
        return std::make_unique<TRepeatAfterMeResponseBodyRenderer>(std::move(*redirectData));
    }
    return std::make_unique<TCommonResponseBodyRenderer>();
}

} // namespace

const THashMap<TString, TString> RADIO_STREAM_MOCKS = {
    {"user:onyourwave", ""},
    {"personal:recent-tracks", "recent_tracks"},
    {"personal:never-heard", "never_heard"},
    {"personal:missed-likes", "missed_likes"},
    {"personal:collection", "#personal"},
    {"personal:hits", "chart"},
};

std::unique_ptr<IResponseBodyRenderer> ConstructBodyRenderer(const TScenarioRunRequestWrapper& runRequest, const bool forceNlg) {
    return ConstructCommonBodyRenderer(runRequest, /* applyArgs = */ nullptr, forceNlg);
}

std::unique_ptr<IResponseBodyRenderer> ConstructBodyRenderer(const TScenarioApplyRequestWrapper& applyRequest, const bool forceNlg) {
    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();
    if (!forceNlg && applyArgs.GetExecutionFlowType() == TMusicArguments_EExecutionFlowType_BassRadio) {
        return std::make_unique<TSilentResponseBodyRenderer>();
    }

    // apply requests don't have data sources, so we take puid from applyArgs
    return ConstructCommonBodyRenderer(applyRequest, &applyArgs, forceNlg);
}

std::unique_ptr<IResponseBodyRenderer> ConstructBassBodyRenderer(const TScenarioApplyRequestWrapper& applyRequest) {
    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();
    if (applyArgs.GetExecutionFlowType() == TMusicArguments_EExecutionFlowType_BassRadio) {
        return std::make_unique<TSilentResponseBodyRenderer>();
    }
    if (auto redirectData = TryConstructRedirectData(applyRequest, &applyArgs)) {
        return std::make_unique<TRepeatAfterMeResponseBodyRenderer>(std::move(*redirectData));
    }
    return std::make_unique<TCommonResponseBodyRenderer>();
}

TVector<std::pair<TStringBuf, TStringBuf>> GetAllRadioStationIdsPairs(const TFrame& musicPlayFrame, NAlice::TRTLogger& logger) {
    // order of slot names is important (the first is the most preferred)
    // not all slots are supported in music backend recommender
    static const TVector<TStringBuf> SLOTS_NAMES = {
        NAlice::NMusic::SLOT_GENRE,
        NAlice::NMusic::SLOT_EPOCH,
        NAlice::NMusic::SLOT_MOOD,
        NAlice::NMusic::SLOT_ACTIVITY,
        NAlice::NMusic::SLOT_LANGUAGE,
        NAlice::NMusic::SLOT_VOCAL,
    };

    TVector<std::pair<TStringBuf, TStringBuf>> idsPairs;
    if (const auto slot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_STREAM)) {
        TStringBuf slotValue = slot->Value.AsString();
        if (RADIO_STREAM_MOCKS.FindPtr(slotValue)) {
            TStringBuf key, value;
            TStringBuf{slotValue}.Split(":", key, value);
            idsPairs.emplace_back(key, value);
        } else {
            LOG_WARN(logger) << "Found unexpected stream value: " << slotValue;
        }
    }
    for (const TStringBuf slotName : SLOTS_NAMES) {
        if (const auto slot = musicPlayFrame.FindSlot(slotName)) {
            idsPairs.emplace_back(slotName, slot->Value.AsString());
        }
    }
    return idsPairs;
}

TVector<TString> ConvertRadioStationIdsPairsToIds(const TVector<std::pair<TStringBuf, TStringBuf>>& pairs) {
    TVector<TString> ids;
    for (const auto& [slotName, slotValue] : pairs) {
        ids.push_back(TString::Join(slotName, ":", slotValue));
    }
    return ids;
}

TVector<TString> GetAllRadioStationIds(const TFrame& musicPlayFrame, NAlice::TRTLogger& logger) {
    return ConvertRadioStationIdsPairsToIds(GetAllRadioStationIdsPairs(musicPlayFrame, logger));
}

const NJson::TJsonValue* FindBlock(const NJson::TJsonValue& context, const TStringBuf name) {
    const auto& blocks = context[BLOCKS].GetArray();
    return FindIfPtr(blocks, [name](const NJson::TJsonValue& block) { return block[TYPE].GetString() == name; });
}

NJson::TJsonValue* FindMutableBlock(NJson::TJsonValue& context, const TStringBuf name) {
    if (auto& blocks = context[BLOCKS]; blocks.IsArray()) {
        return FindIfPtr(blocks.GetArraySafe(),
                         [name](const NJson::TJsonValue& block) { return block[TYPE].GetString() == name; });
    }
    return nullptr;
}

const NJson::TJsonValue* TryGetArtist(const NJson::TJsonValue& webAnswer) {
    const auto& artists = webAnswer["artists"].GetArray();
    if (artists.empty()) {
        return nullptr;
    }
    // album/track
    // NOTE: Artists who have is_various=true, are not realy artists.
    // Some albums-compilations have such artists (dunno why)
    if (artists[0]["is_various"].GetBoolean()) {
        return nullptr;
    }
    return &artists[0];
}

NJson::TJsonValue GetCommonPlaybackOptions(const TFrame& musicPlayFrame) {
    NJson::TJsonValue opts;
    if (const auto slot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_DISABLE_AUTOFLOW)) {
        if (auto maybeValue = slot->Value.As<bool>()) {
            opts["disable_autoflow"] = *maybeValue;
        }
    }
    if (const auto slot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_DISABLE_NLG)) {
        if (auto maybeValue = slot->Value.As<bool>()) {
            opts["disable_nlg"] = *maybeValue;
        }
    }
    if (const auto slot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_PLAY_SINGLE_TRACK)) {
        if (auto maybeValue = slot->Value.As<bool>()) {
            opts["play_single_track"] = *maybeValue;
        }
    }
    if (const auto slot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_TRACK_OFFSET_INDEX)) {
        if (auto maybeValue = slot->Value.As<ui32>()) {
            opts["track_offset_index"] = *maybeValue;
        }
    }
    if (const auto slot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_FROM)) {
        if (auto maybeValue = slot->Value.As<TString>()) {
            opts["from"] = *maybeValue;
        }
    }
    if (const auto slot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_DISABLE_HISTORY)) {
        if (auto maybeValue = slot->Value.As<bool>()) {
            opts["disable_history"] = *maybeValue;
        }
    }
    return opts;
}

const TSlot* GetSpecialPlaylistOrNoveltySlot(const TMaybe<TFrame>& frame) {
    if (!frame) {
        return nullptr;
    }
    return GetSpecialPlaylistOrNoveltySlot(frame.GetRef());
}

const TSlot* GetSpecialPlaylistOrNoveltySlot(const TFrame& frame) {
    using namespace NAlice::NMusic;
    if (auto specialPlaylist = frame.FindSlot(SLOT_SPECIAL_PLAYLIST)) {
        return specialPlaylist.Get();
    }
    if (auto novelty = frame.FindSlot(SLOT_NOVELTY)) {
        return novelty.Get();
    }
    return nullptr;
}

bool IsSpecialPlaylistItem(const NAlice::NMusic::TSpecialPlaylistInfo& info) {
    return std::holds_alternative<NAlice::NMusic::TSpecialPlaylistInfo::TPlaylist>(info.Info);
}

const NAlice::NMusic::TSpecialPlaylistInfo::TPlaylist& GetSpecialPlaylistItem(
    const NAlice::NMusic::TSpecialPlaylistInfo& info) {
    return std::get<NAlice::NMusic::TSpecialPlaylistInfo::TPlaylist>(info.Info);
}

const NAlice::NMusic::TSpecialPlaylistInfo::TAlbum&
GetSpecialAlbumItem(const NAlice::NMusic::TSpecialPlaylistInfo& info) {
    return std::get<NAlice::NMusic::TSpecialPlaylistInfo::TAlbum>(info.Info);
}

bool SupportsClientBiometry(const TScenarioApplyRequestWrapper &request) {
    if (request.HasExpFlag(EXP_HW_MUSIC_DISABLE_SERVER_BIOMETRY)) {
        return true;
    }

    if (!request.Proto().HasArguments()) {
        return false;
    }

    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    return TCapabilityWrapper<TScenarioApplyRequestWrapper>{
        request,
        &applyArgs.GetEnvironmentState()
    }.SupportsClientBiometry();
}

bool ValidateGuestCredentials(TRTLogger& logger, const TMusicArguments_TGuestCredentials& guestCredentials) {
    if (guestCredentials.GetUid().Empty()) {
        LOG_ERROR(logger) << "Got empty UserID in GuestCredentials";
        return false;
    }
    if (guestCredentials.GetOAuthTokenEncrypted().Empty()) {
        LOG_ERROR(logger) << "Got empty encrypted OAuth token in GuestCredentials";
        return false;
    }
    return true;
}

bool IsIncognitoModeRunRequest(const NAlice::TGuestOptions& guestOptions) {
    return guestOptions.GetIsOwnerEnrolled() && guestOptions.GetStatus() == TGuestOptions::NoMatch;
}

bool IsClientBiometryModeApplyRequest(TRTLogger& logger, const TMusicArguments& applyArgs) {
    return applyArgs.HasGuestCredentials() && ValidateGuestCredentials(logger, applyArgs.GetGuestCredentials());
}

bool IsIncognitoModeApplyRequest(const TMusicArguments& applyArgs) {
    return applyArgs.GetIsOwnerEnrolled() && !applyArgs.HasGuestCredentials();
}

void ClearBiometryOptions(TScenarioState& scState) {
    if (scState.HasQueue() && scState.GetQueue().HasPlaybackContext()) {
        scState.MutableQueue()->MutablePlaybackContext()->ClearBiometryOptions();
    }

    scState.ClearBiometryUserId();
    scState.ClearGuestOAuthTokenEncrypted();
    scState.ClearPlaybackMode();
    scState.ClearIsOwnerEnrolled();
    scState.ClearIncognito();
}

void TryInitPlaybackContextBiometryOptions(TRTLogger& logger, TScenarioState& scState) {
    if (scState.GetQueue().GetPlaybackContext().HasBiometryOptions()) {
        return;
    }

    auto& biometryOpts = *scState.MutableQueue()->MutablePlaybackContext()->MutableBiometryOptions();
    biometryOpts.SetUserId(scState.GetBiometryUserId());
    biometryOpts.SetGuestOAuthTokenEncrypted(scState.GetGuestOAuthTokenEncrypted());
    biometryOpts.SetIsOwnerEnrolled(scState.GetIsOwnerEnrolled());

    auto playbackMode = scState.GetPlaybackMode();
    if (scState.GetIncognito() != (playbackMode == TScenarioState_EPlaybackMode_IncognitoMode)) {
        LOG_WARN(logger) << "Inconsistent playback mode in scenario state: Incognito=" << scState.GetIncognito()
                         << ", PlaybackMode=" << TScenarioState::EPlaybackMode_Name(playbackMode);
    }
    switch (playbackMode) {
        case TScenarioState_EPlaybackMode_OwnerMode:
            biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_OwnerMode);
            break;
        case TScenarioState_EPlaybackMode_GuestMode:
            biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_GuestMode);
            break;
        case TScenarioState_EPlaybackMode_IncognitoMode:
            biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_IncognitoMode);
            break;
        default:
            throw yexception() << "Unexpected PlaybackMode in scenario state: "
                               << TScenarioState::EPlaybackMode_Name(playbackMode);
    }

    LOG_INFO(logger) << "BiometryOptions in PlaybackContext was successfully initialized";
}

void FillEnvironmentState(
    NJson::TJsonValue& musicArguments,
    const NAlice::TEnvironmentState& environmentStateProto
)
{
    musicArguments["environment_state"] = JsonFromProto(environmentStateProto);
}

void FillGuestCredentials(
    NJson::TJsonValue& musicArguments,
    const NAlice::TGuestOptions& guestOptions
)
{
    auto& guestCredentials = musicArguments["guest_credentials"];
    guestCredentials["uid"] = guestOptions.GetYandexUID();
    
    TString guestOAuthTokenEncrypted;
    Y_ENSURE(NCrypto::AESEncryptWeakWithSecret(MUSIC_GUEST_OAUTH_TOKEN_AES_ENCRYPTION_KEY_SECRET,
                                               guestOptions.GetOAuthToken(), guestOAuthTokenEncrypted),
                                               "Error while encrypting guest OAuth token");
    guestCredentials["oauth_token_encrypted"] = Base64Encode(guestOAuthTokenEncrypted);
    musicArguments["is_owner_enrolled"] = guestOptions.GetIsOwnerEnrolled();
}

void FillFairyTaleInfo(
    NJson::TJsonValue& musicArguments,
    const NJson::TJsonValue& bassState,
    const NHollywood::TScenarioRunRequestWrapper& runRequest)
{
    const auto& webAnswer = bassState["apply_arguments"]["web_answer"];
    const bool isFairyTalePlaylistRequest = runRequest.Input().FindSemanticFrame(MUSIC_PLAY_FAIRYTALE_FRAME);
    const bool isFairyTaleOndemandRequest = runRequest.HasExpFlag(NExperiments::EXP_HW_MUSIC_FAIRY_TALES_ENABLE_ONDEMAND) &&
        runRequest.Input().FindSemanticFrame(ALICE_FAIRY_TALE_ONDEMAND_FRAME) != nullptr &&
        webAnswer["is_child_content"].GetBoolean();
    if (isFairyTalePlaylistRequest || isFairyTaleOndemandRequest) {
        musicArguments["fairy_tale_arguments"]["is_fairy_tale_subscenario"] = true;
        musicArguments["fairy_tale_arguments"]["is_ondemand"] = isFairyTaleOndemandRequest;
    }
}

void FillMusicSearchResult(
    TRTLogger& logger,
    NJson::TJsonValue& musicArguments,
    const NJson::TJsonValue& bassState,
    const TMaybe<TFrame>& musicPlayFrame,
    const TString& requesterUserId,
    const bool supportsPlaylists,
    bool* const isSpecialAlbumPtr
)
{
    auto& musicSearchResult = musicArguments["music_search_result"];
    if (const auto* analyticsInfo = FindBlock(bassState[CONTEXT], SCENARIO_ANALYTICS_INFO)) {
        musicSearchResult["scenario_analytics_info"] = (*analyticsInfo)[DATA];
    }

    const auto& webAnswer = bassState["apply_arguments"]["web_answer"];

    bool isSingleTrack = false;
    bool isOnDemand = false;
    bool isForcedAsPartOfAlbum = false;
    if (webAnswer.IsDefined()) {
        const auto& objType = webAnswer["type"].GetString();
        const auto& objId = webAnswer["id"].GetString();

        musicSearchResult["content_type"] = objType;
        musicSearchResult["content_id"] = objId;
        musicSearchResult["name"] = webAnswer["name"].GetString();
        musicSearchResult["title"] = webAnswer["title"].GetString();
        musicSearchResult["subtype"] = webAnswer["subtype"].GetString();

        if (objType == "track") {
            bool hasContainer = webAnswer.Has("allPartsContainer") && webAnswer.GetValueByPath("allPartsContainer.type")->GetString() == "album";
            isForcedAsPartOfAlbum = hasContainer || NAlice::NMusic::IsTalesAlbumWithChapters(webAnswer.GetValueByPath("album.id")->GetString());
            if (isForcedAsPartOfAlbum) {
                musicSearchResult["content_type"] = "album";
                musicSearchResult["content_id"] = hasContainer ?
                                                  ToString(webAnswer.GetValueByPath("allPartsContainer.data.id")->GetUInteger()) :
                                                  webAnswer.GetValueByPath("album.id")->GetString();
                if (const auto *genre = webAnswer.GetValueByPath(hasContainer ? "allPartsContainer.data.genre" : "album.genre")) {
                    musicSearchResult["album_genre"] = genre->GetString();
                }
            } else if (const auto* genre = webAnswer.GetValueByPath("album.genre")) {
                musicSearchResult["track_genre"] = genre->GetString();
            }
        } else if (objType == "album") {
            if (const auto* genre = webAnswer.GetValueByPath("genre")) {
                musicSearchResult["album_genre"] = genre->GetString();
            }
        } else if (objType == "artist") {
            if (const auto* genre = webAnswer.GetValueByPath("genre")) {
                musicSearchResult["artist_genre"] = genre->GetString();
            }
        }

        isSingleTrack = objType == "track" && !isForcedAsPartOfAlbum;
        isOnDemand = !objType.empty();

        auto& onDemandRequest = musicArguments["on_demand_request"];
        if (objType == "artist") {
            onDemandRequest["artist_id"] = objId;
        } else if (const auto* artist = TryGetArtist(webAnswer)) {
            onDemandRequest["artist_id"] = (*artist)["id"].GetString();
        } else {
            LOG_INFO(logger) << "Could not find artist_id in web_answer from BASS, continuing without it...";
        }
    }

    if (musicPlayFrame) {
        if (supportsPlaylists && !isOnDemand) {
            using NAlice::NHollywood::NMusic::TPlaylistId;
            if (auto playlistSlot = musicPlayFrame->FindSlot(NAlice::NMusic::SLOT_PLAYLIST)) {
                auto& playlistRequest = musicArguments["playlist_request"];
                // This is the kind of playlist request that will be searched by name using music API
                // later in continue phase
                playlistRequest["playlist_type"] = "Normal";
                playlistRequest["playlist_name"] = playlistSlot->Value.AsString();
            } else if (const auto* special = GetSpecialPlaylistOrNoveltySlot(musicPlayFrame)) {
                // All special and common special playlists are listed here:
                // https://a.yandex-team.ru/arc/trunk/arcadia/alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/entities/music_ex/special_playlist.json
                auto& playlistRequest = musicArguments["playlist_request"];
                const auto& playlistMap = NAlice::NMusic::GetCommonSpecialPlaylists();
                TStringBuf specialValue = special->Value.AsString();
                if (specialValue == "alice") {
                    specialValue = "origin"; // ALICE-11848
                }
                if (const auto* commonPlaylist = playlistMap.FindPtr(specialValue)) {
                    if (IsSpecialPlaylistItem(*commonPlaylist)) {
                        // These are some pre-defined playlists.
                        // They have known owner & kind, so the are process identical to onDemand content
                        const auto& playlistInfo = GetSpecialPlaylistItem(*commonPlaylist);
                        musicSearchResult["content_id"] =
                            TPlaylistId(playlistInfo.OwnerId, playlistInfo.Kind).ToString();
                        musicSearchResult["content_type"] = "playlist";
                    } else {
                        // Some of pre-defined 'playlists' are actually album, these
                        // requests will start playback of specified album.
                        const auto& albumInfo = GetSpecialAlbumItem(*commonPlaylist);
                        musicSearchResult["content_id"] = albumInfo.Id;
                        musicSearchResult["content_type"] = "album";
                        if (isSpecialAlbumPtr) {
                            *isSpecialAlbumPtr = true;
                        }
                    }
                    musicSearchResult["title"] = commonPlaylist->Title;
                } else {
                    // Special playlists are actually personal playlists.
                    // To get owner:kind of these playlists a separate request
                    // to music API is required.
                    playlistRequest["playlist_type"] = "Special";
                    playlistRequest["playlist_name"] = specialValue;
                }
            } else if (musicPlayFrame->FindSlot(NAlice::NMusic::SLOT_PERSONALITY)) {
                // "включи мою музыку" is user specific playlist with fixed kind.
                musicSearchResult["content_id"] = TPlaylistId(requesterUserId, NMusic::PLAYLIST_LIKE_KIND).ToString();
                musicSearchResult["content_type"] = "playlist";
                musicSearchResult["is_personal"] = true;
            }
        }

        auto opts = GetCommonPlaybackOptions(*musicPlayFrame);

        if (isForcedAsPartOfAlbum) {
            opts["start_from_track_id"] = webAnswer["id"].GetString();
        } else if (webAnswer.IsDefined() && webAnswer["type"].GetString() == "album"
            && webAnswer.GetValueByPath("firstTrack.id") && ::NAlice::NMusic::IsTalesAlbumWithChapters(webAnswer["id"].GetString())) {
            opts["start_from_track_id"] = webAnswer.GetValueByPath("firstTrack.id")->GetString();
        }
        if (auto orderSlot = musicPlayFrame->FindSlot(NAlice::NMusic::SLOT_ORDER)) {
            opts["shuffle"] = orderSlot->Value.AsString() == "shuffle";
        }
        if (auto offsetSlot = musicPlayFrame->FindSlot(NAlice::NMusic::SLOT_OFFSET)) {
            opts["offset"] = offsetSlot->Value.AsString();
        }
        if (auto repeatSlot = musicPlayFrame->FindSlot(NAlice::NMusic::SLOT_REPEAT)) {
            if (repeatSlot->Value.AsString() == "repeat") {
                NMusic::ERepeatType repeatType;
                if (isSingleTrack) {
                    repeatType = NMusic::RepeatTrack;
                } else {
                    repeatType = NMusic::RepeatAll;
                }
                opts["repeat_type"] = NMusic::ERepeatType_Name(repeatType);
            }
        }

        musicArguments["playback_options"] = std::move(opts);
    }
}

bool IsUgcTrackId(const TStringBuf id) {
    return CheckUuid(id);
}

NMusic::TMusicScenarioMementoData ParseMementoData(const NHollywood::TScenarioApplyRequestWrapper& applyRequest) {
    NMusic::TMusicScenarioMementoData mementoScenarioData;
    applyRequest.BaseRequestProto().GetMemento().GetScenarioData().UnpackTo(&mementoScenarioData);
    return mementoScenarioData;
}

void AddAttentionToJsonState(NJson::TJsonValue& stateJson, const TString& attentionType) {
    Y_ENSURE(stateJson.Has("blocks"));
    NJson::TJsonValue attention;
    attention["type"] = "attention";
    attention["attention_type"] = attentionType;
    stateJson["blocks"].AppendValue(std::move(attention));
}

TString ConstructCoverUri(const TStringBuf coverUriTemplate) {
    // old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/base_music_answer.cpp?rev=r8709318#L226
    if (coverUriTemplate.Empty()) {
        return TString{DEFAULT_COVER_URI};
    }
    TString coverUri{coverUriTemplate};
    if (!coverUri.StartsWith("http")) {
        coverUri = TString::Join("https://", coverUri);
    }
    size_t pos = coverUri.find("%%");
    if (pos != TString::npos) {
        coverUri.replace(pos, 2, DEFAULT_COVER_SIZE);
    }
    return coverUri;
}

} // namespace NAlice::NHollywood::NMusic
