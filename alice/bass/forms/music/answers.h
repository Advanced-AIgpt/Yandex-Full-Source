#pragma once

#include "cache.h"

#include <alice/library/client/client_features.h>

namespace NBASS::NMusic {

extern const THashMap<TStringBuf, NSc::TValue> PERSONAL_SPECIAL_PLAYLISTS_DATA;
inline constexpr TStringBuf ALICE_SESSION_ID = "aliceSessionId";
inline constexpr TStringBuf DEFAULT_COVER_SIZE = "200x200";
inline constexpr TStringBuf DEFAULT_COVER = "https://avatars.mds.yandex.net/get-bass/469429/music_default_img/orig";
inline constexpr TStringBuf ORIGIN_SPECIAL_PLAYLIST_COVER_URI = "https://avatars.mds.yandex.net/get-bass/787408/orig_playlist/orig";

struct TPathElement {
    TString Key;
    TString Id;

    TPathElement(TString key, TString id)
        : Key(key)
        , Id(id)
    {};
};

using TPathVector = TVector<TPathElement>;

class TBaseMusicAnswer {
public:
    explicit TBaseMusicAnswer(const NAlice::TClientFeatures& clientFeatures)
        : ClientFeatures(clientFeatures)
    {
    }

    virtual ~TBaseMusicAnswer() = default;

    static TString MakeLinkForTrack(const NAlice::TClientFeatures& clientFeatures, const TPathVector& path, TStringBuf trackId, bool needAutoplay);
    static TString MakeLinkForMusicObject(const NAlice::TClientFeatures& clientFeatures, const TPathVector& path, bool needAutoplay);
    static TString MakePath(const TPathVector& path);

    static TCgiParameters MakeDeeplinkParams(const NAlice::TClientFeatures& clientFeatures, const NSc::TValue& directive);
    static TString MakeDeeplink(const NAlice::TClientFeatures& clientFeatures, const NSc::TValue& directive);
    static TString MakeLinkToTrackForShazam(const NAlice::TClientFeatures& clientFeatures, const NSc::TValue& answer);

    static TString MakeCoverUri(TStringBuf origUri);

protected:
    bool MakeOutputFromAnswer(NSc::TValue* value);

    bool MakeBestTrack(NSc::TValue* value) const;
    bool MakeBestAlbum(const NSc::TValue& album, NSc::TValue* value, bool firstTrackId) const;
    bool MakeBestArtist(NSc::TValue* value) const;
    bool MakePlaylist(NSc::TValue* value) const;
    bool MakeAlbumByBestTrack(NSc::TValue* value, TString trackId) const; 

    TString MakeLink(const TPathVector& path) const;
    TString MakeTrackLink(const TPathVector& path, TStringBuf trackId) const;


    const NAlice::TClientFeatures& ClientFeatures;
    NSc::TValue Answer;
    TString AnswerType;
    bool NeedAutoplay = false;
};

class TYandexMusicAnswer : public TBaseMusicAnswer {
public:
    explicit TYandexMusicAnswer(const NAlice::TClientFeatures& clientFeatures)
        : TBaseMusicAnswer(clientFeatures)
    {
    }

    void InitWithSearchAnswer(const NSc::TValue& answer, bool autoplay);
    void InitWithShazamAnswer(const TString& section, const NSc::TValue& answer, bool autoplay);
    void InitWithRelatedAnswer(const TString& section, const NSc::TValue& answer, bool autoplay);
    bool AnswerWithSpecialPlaylist(const TString& playlistName, bool autoplay, NSc::TValue* out);
    bool MakeSpecialAnswer(TStringBuf specialAnswerRawInfo, bool autoplay, NSc::TValue* out);
    bool ConvertAnswerToOutputFormat(NSc::TValue* value);

private:
    TStringBuf GetBestResultType() const;
};

class TQuasarMusicAnswer : public TBaseMusicAnswer {
public:
    explicit TQuasarMusicAnswer(const NAlice::TClientFeatures& clientFeatures)
        : TBaseMusicAnswer(clientFeatures)
    {
    }

    bool Init(const NSc::TValue& result);
    bool ConvertAnswerToOutputFormat(NSc::TValue* value);

private:
    bool MakeFiltersAnswer(NSc::TValue* value);
};

class TYandexRadioAnswer {
public:
    TYandexRadioAnswer(const NAlice::TClientFeatures& clientFeatures, const NMusic::TStationsData& radioStations)
        : ClientFeatures(clientFeatures)
        , RadioStations(radioStations)
    {
    }
    void InitCommonStation(const TString& stationTag);
    void InitPersonalStation(const NSc::TValue& status = NSc::TValue());
    bool ConvertAnswerToOutputFormat(NSc::TValue* value);

    static bool MakeOutputFromServiceData(const NAlice::TClientFeatures& clientFeatures, const NSc::TValue& answer, NSc::TValue* output);

private:
    TString MakeLink() const;

    const NAlice::TClientFeatures& ClientFeatures;
    const NMusic::TStationsData& RadioStations;
    bool IsPersonal = false;
    TStation Station;
};

} // namespace NBASS::NMusic
