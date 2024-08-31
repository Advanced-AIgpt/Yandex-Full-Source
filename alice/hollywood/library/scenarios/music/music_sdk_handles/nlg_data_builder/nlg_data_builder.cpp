#include "nlg_data_builder.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/json/json.h>
#include <alice/library/url_builder/url_builder.h>

#include <util/charset/utf8.h>
#include <util/random/shuffle.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

namespace {

constexpr TStringBuf COVER_DOESNT_EXIST_MESSAGE = "cover doesn't exist";
constexpr int RGB_COLOR_LEN = 7;

NJson::TJsonValue ConstructArtistBlock(const NJson::TJsonValue& webAnswer) {
    NJson::TJsonValue res;
    res["name"] = webAnswer["name"];
    res["id"] = webAnswer["id"];
    res["is_various"] = webAnswer["various"].GetBooleanSafe(/* defaultValue = */ false);
    res["composer"] = webAnswer["composer"].GetBooleanSafe(/* defaultValue = */ false);
    return res;
}

NJson::TJsonValue ConstructArtistsList(const NJson::TJsonValue::TArray& webAnswers) {
    NJson::TJsonValue res;
    for (const auto& webAnswer : webAnswers) {
        res.AppendValue(ConstructArtistBlock(webAnswer));

        if (webAnswer.Has("decomposed")) {
            for (const auto& item : webAnswer["decomposed"].GetArray()) {
                if (item.IsMap()) {
                    res.AppendValue(ConstructArtistBlock(item));
                }
            }
        }
    }
    return res;
}

TMaybe<NJson::TJsonValue> ConstructAlbum(const NJson::TJsonValue& webAnswer) {
    const TString albumId = webAnswer["id"].IsInteger()
        ? ToString(webAnswer["id"].GetInteger())
        : webAnswer["id"].GetString();

    if (albumId.Empty()) {
        return Nothing();
    }

    NJson::TJsonValue res;
    res["type"] = "album";
    res["subtype"] = webAnswer["subtype"];
    res["title"] = webAnswer["title"];
    res["genre"] = webAnswer["genre"];
    res["childContent"] = webAnswer["childContent"];
    // TODO(sparkle): coverUri
    res["id"] = albumId;

    // TODO(sparkle): uri
    res["artists"] = ConstructArtistsList(webAnswer["artists"].GetArray());

    return res;
}

TMaybe<NJson::TJsonValue> ConstructArtist(const NJson::TJsonValue& webAnswer) {
    const TString artistId = webAnswer["id"].IsInteger()
        ? ToString(webAnswer["id"].GetInteger())
        : webAnswer["id"].GetString();

    if (artistId.Empty()) {
        return Nothing();
    }

    NJson::TJsonValue res;
    res["type"] = "artist";
    res["name"] = webAnswer["name"];
    // TODO(sparkle): coverUri
    if (const auto& genres = webAnswer["genres"].GetArray(); !genres.empty()) {
        res["genre"] = genres[0];
    }
    res["id"] = artistId;
    // TODO(sparkle): uri

    return res;
}

TStringBuf GetRawCoverUri(const NJson::TJsonValue& playlistObj) {
    const auto& cover = playlistObj["cover"];
    if (cover.IsMap()) {
        if (cover["error"].GetString() == COVER_DOESNT_EXIST_MESSAGE) {
            // a number of playlists don't have cover, return the default Alice cover
            return DEFAULT_COVER_URI;
        }

        const auto& type = cover["type"].GetString();
        if (type == "pic") {
            return cover["uri"].GetString();
        } else if (type == "mosaic") {
            // TODO(sparkle): it2-test for it, https://music.yandex.ru/users/ya.musicpop/playlists/1153
            return cover["itemsUri"][0].GetString();
        }
    }
    return DEFAULT_COVER_URI;
}

TString GetUserId(const NJson::TJsonValue& playlistObj) {
    if (playlistObj["owner"].Has("id")) {
        return playlistObj["owner"]["id"].GetStringRobust();
    }
    if (playlistObj.Has("uid")) {
        return playlistObj["uid"].GetStringRobust();
    }
    return TString();
}

const TString& GetUserLogin(const NJson::TJsonValue& playlistObj) {
    return playlistObj["owner"]["login"].GetString();
}

const NJson::TJsonValue* GetPlaylistObjectFromUsualPlaylist(const NJson::TJsonValue& webAnswer) {
    Y_ENSURE(webAnswer.Has("result"));
    const auto& results = webAnswer["result"]["playlists"]["results"];
    if (!results.IsArray() || results.GetArray().empty()) {
        return nullptr;
    }
    return &results[0];
}

const NJson::TJsonValue* GetPlaylistObjectFromSpecialPlaylist(const NJson::TJsonValue& webAnswer) {
    Y_ENSURE(webAnswer.Has("result"));
    return &webAnswer["result"]["data"];
}

const NJson::TJsonValue* GetPlaylistObjectFromPredefinedPlaylist(const NJson::TJsonValue& webAnswer) {
    Y_ENSURE(webAnswer.Has("result"));
    return &webAnswer["result"];
}

TString ConstructCoverUriForPlaylistObject(const NJson::TJsonValue& playlistObj) {
    return ConstructCoverUri(GetRawCoverUri(playlistObj));
}

TMaybe<NJson::TJsonValue> ConstructPlaylistCommon(const NScenarios::TInterfaces& interfaces,
                                                  const TClientInfo& clientInfo,
                                                  const NJson::TJsonValue& playlistObj)
{
    const TString userId = GetUserId(playlistObj);
    if (userId.Empty()) {
        return Nothing();
    }
    const TString& userLogin = GetUserLogin(playlistObj);

    NJson::TJsonValue res;
    res["type"] = "playlist";
    res["title"] = playlistObj["title"].GetString();
    res["coverUri"] = ConstructCoverUriForPlaylistObject(playlistObj);

    const TStringBuf kind = playlistObj["kind"].GetString();
    const TStringBuf specialPlaylistId = playlistObj["generatedPlaylistType"].GetString();
    res["id"] = TString::Join(userId, ":", kind);

    // make uri, old code:
    // https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/base_music_answer.cpp?rev=r8671477#L219

    TStringBuilder path;
    const auto tryAddToPath = [&path](const TStringBuf key, const TStringBuf id) {
        if (!key.Empty() && !id.Empty()) {
            path << key << "/" << id << "/";
        }
    };
    if (!specialPlaylistId.Empty()) {
        tryAddToPath("playlist", specialPlaylistId);
    } else {
        tryAddToPath("users", userLogin.Empty() ? userId : userLogin);
        tryAddToPath("playlists", kind);
    }

    TCgiParameters cgi;
    cgi.InsertUnescaped("play", "1");
    res["uri"] = GenerateMusicAppUri(interfaces.GetCanOpenLinkIntent(), clientInfo, EMusicUriType::Music, path, cgi);

    // TODO(sparkle): is this needed???
    //const auto& firstTrack = playlistObj["tracks"][0]["track"];
    //const auto& album = firstTrack["albums"][0];
    //const TStringBuf albumId = album["id"].GetString();
    //if (!albumId.Empty()) {
        //auto& resAlbum = res["firstTrack"]["album"];
        //resAlbum["title"] = album["title"].GetString();
        //resAlbum["id"] = albumId;
        //resAlbum["genre"] = album["genre"].GetString();
        //resAlbum["genre"] = album["genre"].GetString();
    //}

    // TODO(sparkle): childContent
    //                https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/base_music_answer.cpp?rev=r8671477#L169

    return res;
}

TMaybe<NJson::TJsonValue> ConstructPlaylist(const NScenarios::TInterfaces& interfaces,
                                            const TClientInfo& clientInfo,
                                            const NJson::TJsonValue& webAnswer)
{
    if (const auto* playlistObj = GetPlaylistObjectFromUsualPlaylist(webAnswer)) {
        return ConstructPlaylistCommon(interfaces, clientInfo, *playlistObj);
    }
    return Nothing();
}

TMaybe<NJson::TJsonValue> ConstructSpecialPlaylist(const NScenarios::TInterfaces& interfaces,
                                                   const TClientInfo& clientInfo,
                                                   const NJson::TJsonValue& webAnswer)
{
    if (const auto* playlistObj = GetPlaylistObjectFromSpecialPlaylist(webAnswer)) {
        return ConstructPlaylistCommon(interfaces, clientInfo, *playlistObj);
    }
    return Nothing();
}

TMaybe<NJson::TJsonValue> ConstructPredefinedPlaylist(const NScenarios::TInterfaces& interfaces,
                                                      const TClientInfo& clientInfo,
                                                      const NJson::TJsonValue& webAnswer)
{
    if (const auto* playlistObj = GetPlaylistObjectFromPredefinedPlaylist(webAnswer)) {
        return ConstructPlaylistCommon(interfaces, clientInfo, *playlistObj);
    }
    return Nothing();
}

TMaybe<TString> TryConstructRedirectUri(const TContentId& contentId, const TMaybe<TString>& playlistOwnerLogin) {
    // old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/base_provider.cpp?rev=r8722036#L338-344
    static const THashMap<TContentId_EContentType, TStringBuf> MUSIC_REDIRECTS = {
        {TContentId_EContentType_Album, "https://redirect.appmetrica.yandex.com/serve/675548979559187313"},
        {TContentId_EContentType_Artist, "https://redirect.appmetrica.yandex.com/serve/459376213719336466"},
        {TContentId_EContentType_Playlist, "https://redirect.appmetrica.yandex.com/serve/243203434541549406"},
        {TContentId_EContentType_Track, "https://redirect.appmetrica.yandex.com/serve/1107834798845505384"},
    };
    const auto type = contentId.GetType();
    if (const auto iter = MUSIC_REDIRECTS.find(type)) {
        if (type == TContentId_EContentType_Playlist) {
            if (!playlistOwnerLogin.Defined()) {
                return Nothing();
            }
            const auto playlistId = TPlaylistId::FromString(contentId.GetId());
            return TString::Join(iter->second, "?name=", *playlistOwnerLogin, "&id=", playlistId->Kind);
        } else {
            return TString::Join(iter->second, "?id=", contentId.GetId());
        }
    }
    return Nothing();
}

} // namespace

TContentJsonInfoBuilder::TContentJsonInfoBuilder(const TStringBuf answerType,
                                                 const NScenarios::TInterfaces& interfaces,
                                                 const TClientInfo& clientInfo,
                                                 const NJson::TJsonValue& webAnswer)
    : AnswerType_(answerType)
    , Interfaces_(interfaces)
    , ClientInfo_(clientInfo)
    , WebAnswer_(webAnswer)
{
}

TMaybe<NJson::TJsonValue> TContentJsonInfoBuilder::Build() {
    if (AnswerType_ == "album") {
        return ConstructAlbum(WebAnswer_);
    } else if (AnswerType_ == "artist") {
        return ConstructArtist(WebAnswer_);
    } else if (AnswerType_ == "playlist") {
        return ConstructPlaylist(Interfaces_, ClientInfo_, WebAnswer_);
    } else if (AnswerType_ == "special_playlist") {
        return ConstructSpecialPlaylist(Interfaces_, ClientInfo_, WebAnswer_);
    } else if (AnswerType_ == "predefined_playlist") {
        return ConstructPredefinedPlaylist(Interfaces_, ClientInfo_, WebAnswer_);
    }
    return Nothing();
}

TNlgDataBuilder::TNlgDataBuilder(NAlice::TRTLogger& logger,
                                 const TScenarioBaseRequestWrapper& request)
    : Request_{request}
    , ContinueArgs_{nullptr}
    , NlgData_{logger, request}
    , AnswerSlot_{NlgData_.Context["slots"]["answer"]}
{
    // To prevent NullPointerExpection errors at NLG
    NlgData_.Context["attentions"].SetType(NJson::JSON_MAP);
    NlgData_.ReqInfo["experiments"].SetType(NJson::JSON_MAP);

    // Fill experiments & utterance data (instead of legacy nlgData.ReqInfo)
    NlgData_.Context["experiments"].SetType(NJson::JSON_MAP);
    for (const auto& [name, value] : request.ExpFlags()) {
        if (value.Defined()) {
            NlgData_.Context["experiments"][name] = *value;
        }
    }
    // XXX(sparkle): Not needed in music NLGs
    // NlgData_.Context["utterance"]["text"] = request.Input().Utterance();

    // Fill attentions
    NlgData_.AddAttention("supports_music_player");
}

TNlgDataBuilder::TNlgDataBuilder(NAlice::TRTLogger& logger,
                                 const TScenarioBaseRequestWrapper& request,
                                 const TFrame& musicPlayFrame)
    : TNlgDataBuilder{logger, request}
{
    // Fill slots data (instead of legacy nlgData.Form)
    for (const auto& slot: musicPlayFrame.Slots()) {
        NJson::TJsonValue slotJson;
        if (NJson::ReadJsonFastTree(slot.Value.AsString(), &slotJson)) {
            NlgData_.Context["slots"][slot.Name] = slotJson;
        } else {
            NlgData_.Context["slots"][slot.Name] = slot.Value.AsString();
        }
    }

}

TNlgDataBuilder::TNlgDataBuilder(NAlice::TRTLogger& logger,
                                 const TScenarioApplyRequestWrapper& request)
    : TNlgDataBuilder{logger, static_cast<const TScenarioBaseRequestWrapper&>(request)}
{
    // add "answer" slot
    ContinueArgs_ = &request.UnpackArgumentsAndGetRef<TMusicArguments>();
    NJson::TJsonValue bassState = JsonFromString(ContinueArgs_->GetBassScenarioState());
    AnswerSlot_ = std::move(bassState["apply_arguments"]["web_answer"]);
}

TNlgDataBuilder::TNlgDataBuilder(NAlice::TRTLogger& logger,
                                 const TScenarioApplyRequestWrapper& request,
                                 const TFrame& musicPlayFrame)
    : TNlgDataBuilder{logger, static_cast<const TScenarioBaseRequestWrapper&>(request), musicPlayFrame}
{
    // add "answer" slot
    ContinueArgs_ = &request.UnpackArgumentsAndGetRef<TMusicArguments>();
    NJson::TJsonValue bassState = JsonFromString(ContinueArgs_->GetBassScenarioState());
    AnswerSlot_ = std::move(bassState["apply_arguments"]["web_answer"]);
}

void TNlgDataBuilder::AddAttention(const TStringBuf attention) {
    NlgData_.AddAttention(attention);
}

bool TNlgDataBuilder::HasAttention(const TStringBuf attention) const {
    return NlgData_.Context["attentions"].Has(attention);
}

void TNlgDataBuilder::AddMusicSdkUri(const TContentId& contentId,
                                     const TString& musicSdkUri,
                                     const TStringBuf logId)
{
    auto& data = NlgData_.Context["data"];
    data["not_available"] = false;
    data["play_inside_app"] = true;
    if (const auto& coverUri = AnswerSlot_["coverUri"].GetStringRobust(); !coverUri.Empty()) {
        data["coverUri"] = coverUri;
    }
    data["playUri"] = musicSdkUri;
    data["log_id"] = logId;
    if (auto redirectUri = TryConstructRedirectUri(contentId, PlaylistOwnerLogin_)) {
        data["redirectUri"] = std::move(*redirectUri);
    }
}

void TNlgDataBuilder::AddMusicVerticalUri(const TString& musicVerticalUri) {
    NlgData_.Context["fallback_to_music_vertical"]["data"]["uri"] = musicVerticalUri;
}

void TNlgDataBuilder::SetLikesCount(int likesCount) {
    AnswerSlot_["likesCount"] = ToString(likesCount);
}

void TNlgDataBuilder::ReplaceAnswerSlot(NJson::TJsonValue&& value) {
    AnswerSlot_ = std::move(value);
}

void TNlgDataBuilder::CopyToAnswerSlotWithShuffle(NAlice::IRng& rng, const TStringBuf key,
                                                  const TVector<NJson::TJsonValue>& datas)
{
    // intentionally copy data to nlgData
    auto& answerSlotDatas = AnswerSlot_[key].SetType(NJson::JSON_ARRAY).GetArraySafe();
    for (const auto& data : datas) {
        answerSlotDatas.push_back(data);
    }
    Shuffle(answerSlotDatas.begin(), answerSlotDatas.end(), rng);
}

void TNlgDataBuilder::AddLyricsSearchUri(const TString& artistName, const TString& title) {
    Y_ENSURE(ContinueArgs_);

    const TString lowerArtistName = ToLowerUTF8(artistName);
    const TString lowerTitleName = ToLowerUTF8(title);
    const TString query = TString::Join(lowerArtistName, " ", lowerTitleName, " текст песни");

    const auto& tz = Request_.BaseRequestProto().GetClientInfo().GetTimezone();
    TUserLocation userLocation = ContinueArgs_->HasUserLocation() ?
        TUserLocation{ContinueArgs_->GetUserLocation(), tz} :
        TUserLocation{tz, /* tld */ "ru"};

    TString searchUri = NAlice::GenerateSearchUri(Request_.ClientInfo(), userLocation,
                                                  Request_.ContentRestrictionLevel(), query,
                                                  Request_.Interfaces().GetCanOpenLinkSearchViewport());
    AnswerSlot_["lyricsSearchUri"] = std::move(searchUri);
}

void TNlgDataBuilder::AddBackgroundGradient(const TString& mainColor, const TString& secondColor) {
    if (!mainColor.Empty()) {
        AnswerSlot_["mainColor"] = mainColor;
    }
    if (!secondColor.Empty()) {
        AnswerSlot_["secondColor"] = secondColor;
    }
    AnswerSlot_["blackLogoNeeded"] = false;

    if (mainColor.Size() == RGB_COLOR_LEN && secondColor.Size() == RGB_COLOR_LEN) {
        auto green = std::stoi(mainColor.substr(3, 2), 0, 16) +
                         std::stoi(secondColor.substr(3, 2), 0, 16);
        auto blue = std::stoi(mainColor.substr(5, 2), 0, 16) +
                         std::stoi(secondColor.substr(5, 2), 0, 16);

        // if green and blue in RGB are both between 225 and 255 then a black logo is needed (450 is 225 * 2)
        if (green >= 450 && blue >= 450)  {
            AnswerSlot_["blackLogoNeeded"] = true;
        }
    }
}

void TNlgDataBuilder::SetPlaylistOwnerLogin(const TString& playlistOwnerLogin) {
    PlaylistOwnerLogin_.ConstructInPlace(playlistOwnerLogin);
}

void TNlgDataBuilder::SetErrorCode(const TString& errorCode) {
    NlgData_.Context["error"]["data"]["code"] = errorCode;
}

TStringBuf TNlgDataBuilder::GetAnswerUri() const {
    return AnswerSlot_["uri"].GetString();
}

const TNlgData& TNlgDataBuilder::GetNlgData() const {
    return NlgData_;
}

TMaybe<TStringBuf> TryGetCoverUriFromBassState(const NJson::TJsonValue& bassState) {
    if (const auto* coverUri = bassState.GetValueByPath("apply_arguments.web_answer.coverUri"); coverUri && coverUri->IsString()) {
        return coverUri->GetString();
    }
    return Nothing();
}

namespace NUsualPlaylist {

TString ConstructCoverUri(const NJson::TJsonValue& webAnswer) {
    if (const auto* playlistObj = GetPlaylistObjectFromUsualPlaylist(webAnswer)) {
        return ConstructCoverUriForPlaylistObject(*playlistObj);
    }
    return TString{DEFAULT_COVER_URI};
}

} // namespace NUsualPlaylist

namespace NSpecialPlaylist {

TString ConstructCoverUri(const NJson::TJsonValue& webAnswer) {
    if (const auto* playlistObj = GetPlaylistObjectFromSpecialPlaylist(webAnswer)) {
        return ConstructCoverUriForPlaylistObject(*playlistObj);
    }
    return TString{DEFAULT_COVER_URI};
}

} // namespace NSpecialPlaylist

namespace NPredefinedPlaylist {

TString ConstructCoverUri(const NJson::TJsonValue& webAnswer) {
    if (const auto& result = webAnswer["result"]; result.IsDefined()) {
        return ConstructCoverUriForPlaylistObject(result);
    }
    return TString{DEFAULT_COVER_URI};
}

} // namespace NPredefinedPlaylist

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
