#include "playlist_id.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

Y_UNIT_TEST_SUITE(PlaylistIdTest) {

Y_UNIT_TEST(TestToString) {
    TPlaylistId id{TStringBuf("owner"), TStringBuf("kind")};
    UNIT_ASSERT_STRINGS_EQUAL(id.ToString(), "owner:kind");
    UNIT_ASSERT_STRINGS_EQUAL(id.ToStringForRadio(), "owner_kind");
}

Y_UNIT_TEST(TestFromStringPositive) {
    auto id = TPlaylistId::FromString("owner:kind");
    UNIT_ASSERT(id);
    UNIT_ASSERT_STRINGS_EQUAL(id->Owner, "owner");
    UNIT_ASSERT_STRINGS_EQUAL(id->Kind, "kind");
}

Y_UNIT_TEST(TestFromStringNegative) {
    auto id = TPlaylistId::FromString("there_is_no_spoon_neo");
    UNIT_ASSERT(!id);
}

Y_UNIT_TEST(TestConvertSpecialPlaylistId) {
    UNIT_ASSERT_STRINGS_EQUAL(ConvertSpecialPlaylistId("unchanged"), "unchanged");
    UNIT_ASSERT_STRINGS_EQUAL(ConvertSpecialPlaylistId("hello_world"), "helloWorld");
    UNIT_ASSERT_STRINGS_EQUAL(ConvertSpecialPlaylistId("foo_bar_baz"), "fooBarBaz");
    UNIT_ASSERT_STRINGS_EQUAL(ConvertSpecialPlaylistId("trailing_underscore_"), "trailingUnderscore");
    UNIT_ASSERT_STRINGS_EQUAL(ConvertSpecialPlaylistId("___"), "");
    UNIT_ASSERT_STRINGS_EQUAL(ConvertSpecialPlaylistId("morningShow"), "morningShow");
}

}

} // namespace NAlice::NHollywood::NMusic
