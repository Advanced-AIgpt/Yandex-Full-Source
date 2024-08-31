#include <alice/library/music/common_special_playlists.h>
#include <alice/library/json/json.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>


namespace NAlice::NMusic{
namespace {

const TSpecialPlaylistInfo TEST_DATA_PLAYLIST{"Громкие новинки месяца", "1175", "music-blog", "103372440"};
const TSpecialPlaylistInfo TEST_DATA_ALBUM{"Yet Another New Year", "4924870", "Алиса", "5533796", "electronic"};

Y_UNIT_TEST_SUITE(TSpecialPlaylistConversionUnitTest) {
Y_UNIT_TEST(PlaylistJsonConversion) {
    auto json = TEST_DATA_PLAYLIST.Convert<NJson::TJsonValue>();
    UNIT_ASSERT_STRINGS_EQUAL(json["title"].GetStringSafe(), TEST_DATA_PLAYLIST.Title);
    const auto& playlist = std::get<TSpecialPlaylistInfo::TPlaylist>(TEST_DATA_PLAYLIST.Info);
    UNIT_ASSERT_STRINGS_EQUAL(json["kind"].GetStringSafe(), playlist.Kind);
    UNIT_ASSERT_STRINGS_EQUAL(json["owner"]["login"].GetStringSafe(), playlist.OwnerLogin);
    UNIT_ASSERT_STRINGS_EQUAL(json["owner"]["id"].GetStringSafe(), playlist.OwnerId);
}

Y_UNIT_TEST(AlbumJsonConversion) {
    auto json = TEST_DATA_ALBUM.Convert<NJson::TJsonValue>();
    UNIT_ASSERT_STRINGS_EQUAL(json["title"].GetStringSafe(), TEST_DATA_ALBUM.Title);
    const auto& album = std::get<TSpecialPlaylistInfo::TAlbum>(TEST_DATA_ALBUM.Info);
    UNIT_ASSERT_STRINGS_EQUAL(json["id"].GetStringSafe(), album.Id);
    UNIT_ASSERT_STRINGS_EQUAL(json["genre"].GetStringSafe(), album.Genre);
    UNIT_ASSERT_STRINGS_EQUAL(json["artists"][0]["name"].GetStringSafe(), album.ArtistName);
    UNIT_ASSERT_STRINGS_EQUAL(json["artists"][0]["id"].GetStringSafe(), album.ArtistId);
}

Y_UNIT_TEST(PlaylistSchemeConversion) {
    auto sc = TEST_DATA_PLAYLIST.Convert<NSc::TValue>();
    UNIT_ASSERT_STRINGS_EQUAL(sc["title"].GetString(), TEST_DATA_PLAYLIST.Title);
    const auto& playlist = std::get<TSpecialPlaylistInfo::TPlaylist>(TEST_DATA_PLAYLIST.Info);
    UNIT_ASSERT_STRINGS_EQUAL(sc["kind"].GetString(), playlist.Kind);
    UNIT_ASSERT_STRINGS_EQUAL(sc["owner"]["login"].GetString(), playlist.OwnerLogin);
    UNIT_ASSERT_STRINGS_EQUAL(sc["owner"]["id"].GetString(), playlist.OwnerId);
}

Y_UNIT_TEST(AlbumSchemeConversion) {
    auto sc = TEST_DATA_ALBUM.Convert<NSc::TValue>();
    UNIT_ASSERT_STRINGS_EQUAL(sc["title"].GetString(), TEST_DATA_ALBUM.Title);
    const auto& album = std::get<TSpecialPlaylistInfo::TAlbum>(TEST_DATA_ALBUM.Info);
    UNIT_ASSERT_STRINGS_EQUAL(sc["id"].GetString(), album.Id);
    UNIT_ASSERT_STRINGS_EQUAL(sc["genre"].GetString(), album.Genre);
    UNIT_ASSERT_STRINGS_EQUAL(sc["artists"][0]["name"].GetString(), album.ArtistName);
    UNIT_ASSERT_STRINGS_EQUAL(sc["artists"][0]["id"].GetString(), album.ArtistId);
}
} // Y_UNIT_TEST_SUITE(TSpecialPlaylistConversionUnitTest)
} // namespace
} // namespace NAlice::NMusic
