#include "fallback_playlists.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

Y_UNIT_TEST_SUITE(FallbackPlaylistsTest) {

Y_UNIT_TEST(SmokeTest) {
    UNIT_ASSERT(TryGetFallbackPlaylist("trololololo").Empty());

    const auto industrialPlaylist = TryGetFallbackPlaylist("genre:industrial");
    UNIT_ASSERT(industrialPlaylist.Defined());
    UNIT_ASSERT_VALUES_EQUAL(industrialPlaylist->Owner, "414787002");
    UNIT_ASSERT_VALUES_EQUAL(industrialPlaylist->Kind, "1052");

    const auto ninetiesPlaylist = TryGetFallbackPlaylist("epoch:nineties");
    UNIT_ASSERT(ninetiesPlaylist.Defined());
    UNIT_ASSERT_VALUES_EQUAL(ninetiesPlaylist->Owner, "837761439");
    UNIT_ASSERT_VALUES_EQUAL(ninetiesPlaylist->Kind, "1122");
}

}

} //namespace NAlice::NHollywood::NMusic
