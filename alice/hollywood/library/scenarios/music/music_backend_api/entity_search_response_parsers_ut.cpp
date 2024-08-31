#include "entity_search_response_parsers.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_config/music_config.h>
#include <alice/protos/data/scenario/music/content_info.pb.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/stream/file.h>
#include <util/folder/path.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf TEST_DATA_DIR =
    "alice/hollywood/library/scenarios/music/music_backend_api/ut/data";
constexpr TStringBuf SEARCH_RESULT_FILENAME = "search_result.json";
constexpr TStringBuf PERSONAL_PLAYLIST_FILENAME = "personal_playlist.json";
constexpr TStringBuf NOVELTY_ALBUM_SEARCH_FILENAME = "novelty_album_search_result.json";
constexpr TStringBuf NOVELTY_ALBUM_SEARCH_EMPTY_FILENAME = "novelty_album_search_result_empty.json";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

} // namespace

Y_UNIT_TEST_SUITE(PlaylistResponseParsersTest) {

Y_UNIT_TEST(SearchResult) {
    auto data = ReadTestData(SEARCH_RESULT_FILENAME);
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    TString playlistName;
    ParsePlaylistSearchResponse(data, mq, playlistName);
    UNIT_ASSERT_STRINGS_EQUAL(playlistName, "abrakadabrasimsalabim");
    UNIT_ASSERT_EQUAL(mq.ContentId().GetType(), TContentId_EContentType_Playlist);
    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentId().GetId(), "1035351314:1000");
}

Y_UNIT_TEST(PersonalPlaylist) {
    auto data = ReadTestData(PERSONAL_PLAYLIST_FILENAME);
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    TString playlistName;
    ParseSpecialPlaylistResponse(data, mq, playlistName);
    UNIT_ASSERT_STRINGS_EQUAL(playlistName, "Плейлист дня");
    UNIT_ASSERT_EQUAL(mq.ContentId().GetType(), TContentId_EContentType_Playlist);
    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentId().GetId(), "503646255:124523836");
}

Y_UNIT_TEST(NoveltyAlbumSearch) {
    auto data = ReadTestData(NOVELTY_ALBUM_SEARCH_FILENAME);
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));


    ParseNoveltyAlbumSearchResponse(data, mq);
    UNIT_ASSERT_EQUAL(mq.ContentId().GetType(), TContentId_EContentType_Album);
    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentId().GetId(), "14221099");
}

Y_UNIT_TEST(NoveltyAlbumSearchEmpty) {
    auto data = ReadTestData(NOVELTY_ALBUM_SEARCH_EMPTY_FILENAME);
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    TContentId id;
    id.SetType(TContentId_EContentType_Track);
    id.SetId("123456");
    TRng rng;
    mq.InitPlayback(id, rng);

    ParseNoveltyAlbumSearchResponse(data, mq);
    UNIT_ASSERT_EQUAL(mq.ContentId().GetType(), TContentId_EContentType_Track);
    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentId().GetId(), "123456");
}

}

} //namespace NAlice::NHollywood::NMusic
