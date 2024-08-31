#include "music_sdk_uri_builder.h"

#include <dict/dictutil/str.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

namespace {

constexpr TStringBuf ALBUM_PARAM = "album";
constexpr TStringBuf ARTIST_PARAM = "artist";
constexpr TStringBuf FROM_PARAM = "from";
constexpr TStringBuf FULLSCREEN_PARAM = "fullScreen";
constexpr TStringBuf PLAYLIST_OWNER_PARAM = "owner";
constexpr TStringBuf PLAYLIST_KIND_PARAM = "kind";
constexpr TStringBuf PLAY_PARAM = "play";
constexpr TStringBuf RADIO_PARAM = "radio";
constexpr TStringBuf REPEAT_PARAM = "repeat";
constexpr TStringBuf SHUFFLE_PARAM = "shuffle";
constexpr TStringBuf TAG_PARAM = "tag";
constexpr TStringBuf TRACK_PARAM = "track";

TString ConstructFromParam(const TStringBuf clientName, const TStringBuf type) {
    TString name(clientName);
    ReplaceAll(name, '.', '_');
    return TString::Join("musicsdk-", name, "-alice-", type);
}

void FillDefaultParams(TCgiParameters& params) {
    params.InsertUnescaped(PLAY_PARAM, "true");
    if (!params.Has(REPEAT_PARAM)) {
        params.InsertUnescaped(REPEAT_PARAM, ToString(TMusicSdkUriBuilder::ERepeatMode::REPEAT_OFF));
    }
    if (!params.Has(SHUFFLE_PARAM)) {
        params.InsertUnescaped(SHUFFLE_PARAM, "false");
    }
}

} // namespace

TMusicSdkUriBuilder::TMusicSdkUriBuilder(const TStringBuf clientName, const TStringBuf type)
    : Type_(type)
{
    Params_.InsertUnescaped(FROM_PARAM, ConstructFromParam(clientName, type));
}

TMusicSdkUriBuilder& TMusicSdkUriBuilder::SetPlaylistOwnerAndKind(const TStringBuf owner, const TStringBuf kind) {
    Y_ENSURE(Type_ == "playlist");
    Params_.InsertUnescaped(PLAYLIST_OWNER_PARAM, owner);
    Params_.InsertUnescaped(PLAYLIST_KIND_PARAM, kind);
    return *this;
}

TMusicSdkUriBuilder& TMusicSdkUriBuilder::SetRadioTypeAndTag(const TStringBuf radioType, const TStringBuf radioTag) {
    Y_ENSURE(Type_ == "radio");
    Params_.InsertUnescaped(RADIO_PARAM, radioType);
    Params_.InsertUnescaped(TAG_PARAM, radioTag);
    return *this;
}

TMusicSdkUriBuilder& TMusicSdkUriBuilder::SetTrackId(const TStringBuf trackId) {
    Y_ENSURE(Type_ == "track");
    Params_.InsertUnescaped(TRACK_PARAM, trackId);
    return *this;
}

TMusicSdkUriBuilder& TMusicSdkUriBuilder::SetAlbumId(const TStringBuf albumId) {
    Y_ENSURE(Type_ == "album");
    Params_.InsertUnescaped(ALBUM_PARAM, albumId);
    return *this;
}

TMusicSdkUriBuilder& TMusicSdkUriBuilder::SetArtistId(const TStringBuf artistId) {
    Y_ENSURE(Type_ == "artist");
    Params_.InsertUnescaped(ARTIST_PARAM, artistId);
    return *this;
}

TMusicSdkUriBuilder& TMusicSdkUriBuilder::SetRepeatMode(const ERepeatMode repeatMode) {
    Y_ENSURE(!Params_.Has(REPEAT_PARAM));
    Params_.InsertUnescaped(REPEAT_PARAM, ToString(repeatMode));
    return *this;
}

TMusicSdkUriBuilder& TMusicSdkUriBuilder::SetShuffle(const bool shuffle) {
    Y_ENSURE(!Params_.Has(SHUFFLE_PARAM));
    Params_.InsertUnescaped(SHUFFLE_PARAM, shuffle ? "true" : "false");
    return *this;
}

TMusicSdkUriBuilder& TMusicSdkUriBuilder::SetFullscreen(const bool fullscreen) {
    Y_ENSURE(!Params_.Has(FULLSCREEN_PARAM));
    Params_.InsertUnescaped(FULLSCREEN_PARAM, fullscreen ? "true" : "false");
    return *this;
}

TString TMusicSdkUriBuilder::Build() {
    FillDefaultParams(Params_);
    return TString::Join("musicsdk://?", Params_.Print());
}

} // NAlice::NHollywood::NMusic::NMusicSdk
