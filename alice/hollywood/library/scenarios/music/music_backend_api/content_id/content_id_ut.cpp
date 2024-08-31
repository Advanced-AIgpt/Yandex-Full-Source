#include "content_id.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

Y_UNIT_TEST_SUITE(ContentIdTest) {

Y_UNIT_TEST(PositiveArtistId) {
    constexpr TStringBuf artistType = "artist";
    constexpr TStringBuf artistId = "123321";
    auto contentId = ContentIdFromText(artistType, artistId);
    UNIT_ASSERT(contentId);
    UNIT_ASSERT_EQUAL(contentId->GetType(), TContentId_EContentType_Artist);
    UNIT_ASSERT_EQUAL(contentId->GetId(), artistId);
}

Y_UNIT_TEST(PositiveAlbumId) {
    constexpr TStringBuf albumType = "album";
    constexpr TStringBuf albumId = "112233";
    auto contentId = ContentIdFromText(albumType, albumId);
    UNIT_ASSERT(contentId);
    UNIT_ASSERT_EQUAL(contentId->GetType(), TContentId_EContentType_Album);
    UNIT_ASSERT_EQUAL(contentId->GetId(), albumId);
}

Y_UNIT_TEST(PositiveTrackId) {
    constexpr TStringBuf trackType = "track";
    constexpr TStringBuf trackId = "321321";
    auto contentId = ContentIdFromText(trackType, trackId);
    UNIT_ASSERT(contentId);
    UNIT_ASSERT_EQUAL(contentId->GetType(), TContentId_EContentType_Track);
    UNIT_ASSERT_EQUAL(contentId->GetId(), trackId);
}

Y_UNIT_TEST(PositivePlaylistId) {
    constexpr TStringBuf playlistType = "playlist";
    constexpr TStringBuf playlistId = "owner:kind";
    auto contentId = ContentIdFromText(playlistType, playlistId);
    UNIT_ASSERT(contentId);
    UNIT_ASSERT_EQUAL(contentId->GetType(), TContentId_EContentType_Playlist);
    UNIT_ASSERT_EQUAL(contentId->GetId(), playlistId);
}

Y_UNIT_TEST(Negative) {
    auto contentId = ContentIdFromText("somethingsomething", "321123");
    UNIT_ASSERT(!contentId);
}

Y_UNIT_TEST(ContentTypeToText) {
    UNIT_ASSERT_STRINGS_EQUAL("artist", ContentTypeToText(TContentId_EContentType_Artist));
    UNIT_ASSERT_STRINGS_EQUAL("album", ContentTypeToText(TContentId_EContentType_Album));
    UNIT_ASSERT_STRINGS_EQUAL("track", ContentTypeToText(TContentId_EContentType_Track));
    UNIT_ASSERT_STRINGS_EQUAL("playlist", ContentTypeToText(TContentId_EContentType_Playlist));
    UNIT_ASSERT_STRINGS_EQUAL("radio", ContentTypeToText(TContentId_EContentType_Radio));
}

}

} //namespace NAlice::NHollywood::NMusic
