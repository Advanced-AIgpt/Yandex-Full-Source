#include "track_album_id.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

Y_UNIT_TEST_SUITE(TrackAlbumIdTest) {

Y_UNIT_TEST(TestToString) {
    TTrackAlbumId id{TStringBuf("trackId"), TStringBuf("albumId")};
    UNIT_ASSERT_STRINGS_EQUAL(id.ToString(), "trackId:albumId");
    TTrackAlbumId ugcId{TStringBuf("01234567-89ab-cdef-1234-567890ABCDEF")};
    UNIT_ASSERT_STRINGS_EQUAL(ugcId.ToString(), "01234567-89ab-cdef-1234-567890ABCDEF");
}

Y_UNIT_TEST(TestFromString) {
    auto id = TTrackAlbumId::FromString("trackId:albumId");
    UNIT_ASSERT_STRINGS_EQUAL(id.TrackId, "trackId");
    UNIT_ASSERT_STRINGS_EQUAL(id.AlbumId, "albumId");

    auto ugcId = TTrackAlbumId::FromString("01234567-89ab-cdef-1234-567890ABCDEF");
    UNIT_ASSERT_STRINGS_EQUAL(ugcId.TrackId, "01234567-89ab-cdef-1234-567890ABCDEF");
    UNIT_ASSERT_STRINGS_EQUAL(ugcId.AlbumId, "");
}

}

} // namespace NAlice::NHollywood::NMusic
