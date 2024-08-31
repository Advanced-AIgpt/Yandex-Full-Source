#pragma once

#include <library/cpp/cgiparam/cgiparam.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

// old musicsdk uri builder - https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/base_music_answer.cpp?rev=r8447215#L289-330
class TMusicSdkUriBuilder {
public:
    enum class ERepeatMode {
        REPEAT_OFF /* "repeatOff" */,
        REPEAT_ALL /* "repeatAll" */,
        REPEAT_ONE /* "repeatOne" */,
    };

public:
    TMusicSdkUriBuilder(const TStringBuf clientName, const TStringBuf type);

    TMusicSdkUriBuilder& SetPlaylistOwnerAndKind(const TStringBuf owner, const TStringBuf kind);
    TMusicSdkUriBuilder& SetRadioTypeAndTag(const TStringBuf radioType, const TStringBuf radioTag);
    TMusicSdkUriBuilder& SetTrackId(const TStringBuf trackId);
    TMusicSdkUriBuilder& SetAlbumId(const TStringBuf artistId);
    TMusicSdkUriBuilder& SetArtistId(const TStringBuf artistId);
    TMusicSdkUriBuilder& SetRepeatMode(const ERepeatMode repeatMode);
    TMusicSdkUriBuilder& SetShuffle(const bool shuffle);
    TMusicSdkUriBuilder& SetFullscreen(const bool fullscreen);

    TString Build();

private:
    const TString Type_;
    TCgiParameters Params_;
};

} // NAlice::NHollywood::NMusic::NMusicSdk
