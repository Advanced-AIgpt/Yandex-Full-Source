#include "answers.h"

#include <alice/bass/forms/urls_builder.h>
#include <alice/library/music/fairytale_linear_albums.h>
#include <alice/library/music/answer.h>

#include <dict/dictutil/str.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS::NMusic {

bool TBaseMusicAnswer::MakeOutputFromAnswer(NSc::TValue* value) {
    if (TStringBuf("track") == AnswerType) {
        return MakeBestTrack(value);
    } else if (TStringBuf("album") == AnswerType) {
        return MakeBestAlbum(Answer, value, /* firstTrack */ false);
    } else if (TStringBuf("artist") == AnswerType) {
        return MakeBestArtist(value);
    } else if (TStringBuf("playlist") == AnswerType) {
        return MakePlaylist(value);
    }
    return false;
}

bool TBaseMusicAnswer::MakeBestTrack(NSc::TValue* value) const {
    if (auto res = NAlice::NMusic::MakeBestTrack(ClientFeatures, ClientFeatures.SupportsIntentUrls(), NeedAutoplay, Answer.ToJsonValue()); !res.IsNull()) {
        (*value) = NSc::TValue::FromJsonValue(res);
        return true;
    }

    return false;
}

bool TBaseMusicAnswer::MakeBestAlbum(const NSc::TValue& album, NSc::TValue* value, bool firstTrack) const {
    if (auto res = NAlice::NMusic::MakeBestAlbum(ClientFeatures, ClientFeatures.SupportsIntentUrls(), NeedAutoplay, album.ToJsonValue(), firstTrack); !res.IsNull()) {
        (*value) = NSc::TValue::FromJsonValue(res);
        return true;
    }

    return false;
}

bool TBaseMusicAnswer::MakeBestArtist(NSc::TValue* value) const {
    if (auto res = NAlice::NMusic::MakeBestArtist(ClientFeatures, ClientFeatures.SupportsIntentUrls(), NeedAutoplay, Answer.ToJsonValue()); !res.IsNull()) {
        (*value) = NSc::TValue::FromJsonValue(res);
        return true;
    }

    return false;
}

bool TBaseMusicAnswer::MakePlaylist(NSc::TValue* value) const {
    if (auto res = NAlice::NMusic::MakePlaylist(ClientFeatures, ClientFeatures.SupportsIntentUrls(), NeedAutoplay, Answer.ToJsonValue()); !res.IsNull()) {
        (*value) = NSc::TValue::FromJsonValue(res);
        return true;
    }

    return false;
}

TString TBaseMusicAnswer::MakeCoverUri(TStringBuf origUri) {
    if (origUri.empty()) {
        return TString{DEFAULT_COVER};
    } else {
        TString coverUri(origUri);
        if (!coverUri.StartsWith("http")) {
            coverUri = TString::Join("https://", coverUri);
        }
        size_t pos = coverUri.find("%%");
        if (pos != TString::npos) {
            coverUri.replace(pos, 2, DEFAULT_COVER_SIZE);
        }
        return coverUri;
    }
}

TString TBaseMusicAnswer::MakeLink(const TPathVector& path) const {
    if (path.empty() || AnswerType.empty()) {
        return TString();
    }

    return MakeLinkForMusicObject(ClientFeatures, path, NeedAutoplay);
}

TString TBaseMusicAnswer::MakeTrackLink(const TPathVector& path, TStringBuf trackId) const {
    if (path.empty() || AnswerType.empty()) {
        return TString();
    }

    return MakeLinkForTrack(ClientFeatures, path, trackId, NeedAutoplay);
}

// static method
TString TBaseMusicAnswer::MakeLinkForMusicObject(const NAlice::TClientFeatures& clientFeatures, const TPathVector& path, bool needAutoplay) {
    if (path.empty()) {
        return TString();
    }

    TString urlPath = MakePath(path);
    TCgiParameters cgi;
    if (needAutoplay && !clientFeatures.IsSmartSpeaker()) {
        cgi.InsertEscaped(TStringBuf("play"), TStringBuf("1"));
    }
    return GenerateMusicAppUri(clientFeatures, EMusicUriType::Music, urlPath, cgi);
}

// static method
TString TBaseMusicAnswer::MakeLinkForTrack(const NAlice::TClientFeatures& clientFeatures, const TPathVector& path, TStringBuf trackId, bool needAutoplay) {
    if (path.empty()) {
        return TString();
    }

    TString urlPath = MakePath(path);
    TCgiParameters cgi;
    if (needAutoplay) {
        if (clientFeatures.IsIOS()) {
            cgi.InsertEscaped(TStringBuf("playTrack"), trackId);
        } else if (!clientFeatures.IsSmartSpeaker()) {
            cgi.InsertEscaped(TStringBuf("play"), TStringBuf("1"));
        }
    }
    return GenerateMusicAppUri(clientFeatures, EMusicUriType::Music, urlPath, cgi);
}

// static method
TString TBaseMusicAnswer::MakePath(const TPathVector& path) {
    if (path.empty()) {
        return TString();
    }

    TStringBuilder res;

    for (const auto& p : path) {
        if (p.Key.empty() || p.Id.empty()) {
            break;
        }
        res << p.Key << '/' << p.Id << '/';
    }

    return res;
}

// static method
TCgiParameters TBaseMusicAnswer::MakeDeeplinkParams(const NAlice::TClientFeatures& clientFeatures, const NSc::TValue& directive) {
    TCgiParameters params;
    TStringBuf type = directive["type"].GetString();
    const NSc::TValue& id = directive["id"];
    if (type == TStringBuf("playlist")) {
        params.InsertUnescaped(TStringBuf("kind"), id["kind"].ForceString());
        params.InsertUnescaped(TStringBuf("owner"), id["uid"].ForceString());
    } else if (type == TStringBuf("radio")) {
        if (clientFeatures.IsNavigator()) {
            params.InsertUnescaped(type, id["type"].GetString());
            params.InsertUnescaped(TStringBuf("tag"), id["tag"].GetString());
        } else {
            params.InsertUnescaped(type, TStringBuilder() << id["type"].GetString() << ':' << id["tag"].GetString());
        }
    } else {
        params.InsertUnescaped(type, id["id"].ForceString());
    }

    const NSc::TValue& playerSettings = directive["playerSettings"];
    params.InsertUnescaped(TStringBuf("shuffle"), playerSettings["shuffle"].GetBool(false)
        ? TStringBuf("true")
        : TStringBuf("false")
    );
    params.InsertUnescaped(TStringBuf("repeat"), playerSettings["repeat"].GetString());
    params.InsertUnescaped(ALICE_SESSION_ID, directive[ALICE_SESSION_ID].GetString());

    params.InsertUnescaped(TStringBuf("play"), TStringBuf("true"));

    if (clientFeatures.IsSearchApp() && clientFeatures.SupportsMusicSDKPlayer() &&
        clientFeatures.HasExpFlag(EXPERIMENTAL_FLAG_MUSIC_FULL_SCREEN_PLAYER))
    {
        params.InsertUnescaped(TStringBuf("fullScreen"), TStringBuf("true"));
    }

    TString clientName = clientFeatures.Name;
    ReplaceAll(clientName, '.', '_');
    TStringBuilder from;
    from << "musicsdk-" << clientName << "-alice-" << type;
    params.InsertUnescaped("from", from);

    return params;
}

// static method
TString TBaseMusicAnswer::MakeDeeplink(const NAlice::TClientFeatures& clientFeatures, const NSc::TValue& directive) {
    if (directive.IsNull()) {
        return {};
    }

    return TStringBuilder() << "musicsdk://?" << MakeDeeplinkParams(clientFeatures, directive).Print();
}

TString TBaseMusicAnswer::MakeLinkToTrackForShazam(const NAlice::TClientFeatures& clientFeatures, const NSc::TValue& answer) {
    if (!clientFeatures.IsYaMusic()) {
        return {};
    }
    TCgiParameters params;
    params.InsertUnescaped(TStringBuf("music_recognizer"), TStringBuf("true"));
    params.InsertUnescaped(TStringBuf("title"), answer.Get("title").GetString());

    return TStringBuilder() << "yandexmusic://track/" << answer.Get("id").ForceString() << "?" << params.Print();
}

} // namespace NBASS:NMusic
