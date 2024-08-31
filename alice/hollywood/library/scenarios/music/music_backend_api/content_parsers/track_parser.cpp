#include "track_parser.h"

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf REAL_ID = "realId";
constexpr TStringBuf ALBUMS = "albums";
constexpr TStringBuf ID = "id";
constexpr TStringBuf COVER_URI = "coverUri";
constexpr TStringBuf DURATION_MS = "durationMs";
constexpr TStringBuf GENRE = "genre";
constexpr TStringBuf ALBUM_TYPE = "type";
constexpr TStringBuf COMPOSER = "composer";
constexpr TStringBuf CONTENT_WARNING = "contentWarning";
constexpr TStringBuf EXPLICIT = "explicit";
constexpr TStringBuf IS_SUITABLE_FOR_CHILDREN = "isSuitableForChildren";
constexpr TStringBuf TYPE = "type";
constexpr TStringBuf R128_NORMALIZATION = "r128";
constexpr TStringBuf R128_INTENGRATED_LOUDNESS = "i";
constexpr TStringBuf R128_TRUE_PEAK = "tp";
constexpr TStringBuf REMEMBER_POSITION = "rememberPosition";
constexpr TStringBuf AVAILABLE = "available";
constexpr TStringBuf AVAILABLE_FOR_PREMIUM_USERS = "availableForPremiumUsers";
constexpr TStringBuf VARIOUS = "various";
constexpr TStringBuf LYRICS_INFO = "lyricsInfo";
constexpr TStringBuf YEAR = "year";
constexpr TStringBuf LIKES_COUNT = "likesCount";

void ParseArtist(TArtistInfo& artistObj, const NJson::TJsonValue& artistJson) {
    artistObj.SetId(artistJson[ID].GetStringRobust());
    artistObj.SetName(artistJson[NAME].GetString());
    artistObj.SetComposer(artistJson[COMPOSER].GetBoolean());
    artistObj.SetVarious(artistJson[VARIOUS].GetBoolean());
}

template<typename TRepeatedArtistInfo>
void ParseArtists(TRepeatedArtistInfo& artistsObj, const NJson::TJsonValue& artistsJson) {
    for(const auto& artistJson : artistsJson.GetArray()) {
        ParseArtist(*artistsObj.Add(), artistJson);
    }
}

const NJson::TJsonValue& PickAlbum(const NJson::TJsonValue& albums, const TStringBuf albumId) {
    // pick the specific album
    for (const auto& albumItem : albums.GetArray()) {
        if (albumItem[ID].GetStringRobust() == albumId) {
            return albumItem;
        }
    }
    // just in case - a fallback
    return albums[0];
}

const NJson::TJsonValue& PickOldestAlbum(const NJson::TJsonValue& albums) {
    // the oldest album is most likely the original, not a compilation
    ui64 minYear = Max<ui64>();
    const NJson::TJsonValue* bestAlbum = &albums[0];
    for (const auto& albumItem : albums.GetArray()) {
        const auto year = albumItem[YEAR].GetUInteger();
        if (year > 0 && year < minYear) {
            bestAlbum = &albumItem;
            minYear = year;
        }
    }
    return *bestAlbum;
}

} // namespace

TQueueItem ParseTrack(const NJson::TJsonValue& trackJson, TParseTrackParams params) {
    TQueueItem item;
    if (params.Position) {
        item.MutableTrackInfo()->SetPosition(*params.Position);
    }
    item.SetTrackId(trackJson[REAL_ID].GetStringRobust());
    item.SetTitle(trackJson[TITLE].GetString());
    item.SetType(trackJson[TYPE].GetString());
    if (const auto& artists = trackJson[ARTISTS]; artists.IsArray() && !artists.GetArray().empty()) {
        const auto& artist = artists.GetArray().front();
        item.MutableTrackInfo()->SetArtistId(artist[ID].GetStringRobust());
        ParseArtists(*item.MutableTrackInfo()->MutableArtists(), artists);
    }
    if (const auto& albums = trackJson[ALBUMS]; albums.IsArray() && !albums.GetArray().empty()) {
        const auto& album = params.AlbumId ? PickAlbum(albums, *params.AlbumId) : PickOldestAlbum(albums);
        item.MutableTrackInfo()->SetAlbumTitle(album[TITLE].GetString());
        item.MutableTrackInfo()->SetAlbumId(album[ID].GetStringRobust());
        item.MutableTrackInfo()->SetGenre(album[GENRE].GetString());
        item.MutableTrackInfo()->SetAlbumType(album[ALBUM_TYPE].GetString());
        item.MutableTrackInfo()->SetAlbumCoverUrl(album[COVER_URI].GetString());
        item.MutableTrackInfo()->SetAlbumYear(album[YEAR].GetUInteger());
        item.MutableTrackInfo()->SetAlbumLikes(album[LIKES_COUNT].GetUInteger());
        if (const auto& artists = album[ARTISTS]; artists.IsArray() && !artists.GetArray().empty()) {
            ParseArtists(*item.MutableTrackInfo()->MutableAlbumArtists(), artists);
        }
        if(album[CHILD_CONTENT].GetBoolean() || params.ForceChildSafe) {
            item.SetContentWarning(EContentWarning::ChildSafe);
        }
    }
    item.MutableTrackInfo()->SetAvailable(trackJson[AVAILABLE].GetBooleanSafe(false));
    item.MutableTrackInfo()->SetAvailableForPremiumUsers(
        trackJson[AVAILABLE_FOR_PREMIUM_USERS].GetBooleanSafe(item.GetTrackInfo().GetAvailable()));

    item.SetCoverUrl(trackJson[COVER_URI].GetString());
    item.SetDurationMs(trackJson[DURATION_MS].GetInteger());
    if (trackJson[CONTENT_WARNING].GetString() == EXPLICIT) {
        item.SetContentWarning(EContentWarning::Explicit);
    } else if(trackJson[IS_SUITABLE_FOR_CHILDREN].GetBoolean() || params.ForceChildSafe) {
        item.SetContentWarning(EContentWarning::ChildSafe);
    }

    if (trackJson.Has(R128_NORMALIZATION)) {
        const auto& r128 = trackJson[R128_NORMALIZATION];
        auto& norm = *item.MutableNormalization();
        norm.SetIntegratedLoudness(r128[R128_INTENGRATED_LOUDNESS].GetDoubleSafe());
        norm.SetTruePeak(r128[R128_TRUE_PEAK].GetDoubleSafe());
    }

    if (trackJson.Has(REMEMBER_POSITION)) {
        item.SetRememberPosition(trackJson[REMEMBER_POSITION].GetBoolean());
    }

    if (trackJson.Has(LYRICS_INFO)) {
        const auto& lyricsInfoJson = trackJson[LYRICS_INFO];
        auto& lyricsInfo = *item.MutableTrackInfo()->MutableLyricsInfo();
        lyricsInfo.SetHasAvailableSyncLyrics(lyricsInfoJson["hasAvailableSyncLyrics"].GetBooleanSafe(false));
        lyricsInfo.SetHasAvailableTextLyrics(lyricsInfoJson["hasAvailableTextLyrics"].GetBooleanSafe(false));
    }

    return item;
}

void ParseSingleTrack(const NJson::TJsonValue& resultJson, TMusicQueueWrapper& mq, bool hasMusicSubscription) {
    if (auto contentInfo = TryConstructContentInfo(resultJson[0])) {
        mq.SetContentInfo(*contentInfo);
    }
    mq.TryAddItem(ParseTrack(resultJson[0]), hasMusicSubscription);
}

} // namespace NAlice::NHollywood::NMusic
