#include "requests_helper.h"

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/stream/file.h>
#include <util/folder/path.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

namespace {

constexpr TStringBuf TEST_DATA_DIR =
    "alice/hollywood/library/scenarios/music/music_sdk_handles/ut/data";
constexpr TStringBuf ARTIST_BRIEF_INFO_FILENAME = "artist-brief-info.json";
constexpr TStringBuf ARTIST_TRACKS_INFO_FILENAME = "artist-tracks.json";
constexpr TStringBuf GENRE_OVERVIEW_FILENAME = "genre-overview.json";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

} // namespace

Y_UNIT_TEST_SUITE(ContinueRenderHandleTest) {

Y_UNIT_TEST(ArtistBriefInfoTest) {
    auto data = JsonFromString(ReadTestData(ARTIST_BRIEF_INFO_FILENAME));
    const TVector<const NJson::TJsonValue*> similar = NImpl::FindArtistBriefInfoSimilarArtists(data, /* limit= */ 3);
    const TVector<const NJson::TJsonValue*> expected = {
        &data["result"]["similarArtists"][0],
        &data["result"]["similarArtists"][1],
        &data["result"]["similarArtists"][2],
    };
    UNIT_ASSERT_EQUAL(similar, expected);
}

Y_UNIT_TEST(AlbumsBriefInfoTest) {
    auto data = JsonFromString(ReadTestData(ARTIST_BRIEF_INFO_FILENAME));
    const TVector<const NJson::TJsonValue*> others = NImpl::FindArtistBriefInfoOtherAlbums(data, /* originalAlbumId= */ 3542);
    const TVector<const NJson::TJsonValue*> expected = {
        &data["result"]["albums"][1],
        &data["result"]["albums"][2],
        &data["result"]["albums"][3],
    };
    UNIT_ASSERT_EQUAL(others, expected);
}

Y_UNIT_TEST(TracksTest) {
    auto data = JsonFromString(ReadTestData(ARTIST_TRACKS_INFO_FILENAME));
    const TVector<const NJson::TJsonValue*> others = NImpl::FindArtistOtherTracks(data, /* originalTrackId= */ 22769);
    TVector<const NJson::TJsonValue*> expected;
    for (size_t i = 0; i < 20; ++i) {
        expected.push_back(&data["result"]["tracks"][i]);
    }
    UNIT_ASSERT_EQUAL(others, expected);
}

Y_UNIT_TEST(GenreOverviewTest) {
    auto data = JsonFromString(ReadTestData(GENRE_OVERVIEW_FILENAME));
    const TVector<const NJson::TJsonValue*> others = NImpl::FindGenreOverviewTopArtists(data);
    const TVector<const NJson::TJsonValue*> expected = {
        &data["result"]["artists"][0],
        &data["result"]["artists"][1],
        &data["result"]["artists"][2],
    };
    UNIT_ASSERT_EQUAL(others, expected);
}

}

} //namespace NAlice::NHollywood::NMusic
