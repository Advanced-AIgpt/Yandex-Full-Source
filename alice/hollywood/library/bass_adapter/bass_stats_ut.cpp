#include "bass_stats.h"

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

using NAlice::JsonFromString;
using namespace NAlice::NHollywood::NImpl;

Y_UNIT_TEST_SUITE(ExtractMusicType) {
    Y_UNIT_TEST(Empty) {
        UNIT_ASSERT_VALUES_EQUAL(EMPTY, ExtractMusicType({}));
    }

    Y_UNIT_TEST(PlaylistOfTheDay) {
        const auto json = JsonFromString(R"({
            "slots": [
                {
                    "name": "answer",
                    "value": {
                        "type": "foo"
                    }
                },
                {
                    "name": "foo"
                },
                {
                    "name": "special_playlist",
                    "value": "playlist_of_the_day"
                }
            ]
        })");
        UNIT_ASSERT_VALUES_EQUAL(PLAYLIST_OF_THE_DAY, ExtractMusicType(json));
    }

    Y_UNIT_TEST(SpecialPlaylist) {
        const auto json = JsonFromString(R"({
            "slots": [
                {
                    "name": "answer",
                    "value": {
                        "type": "foo"
                    }
                },
                {
                    "name": "foo"
                },
                {
                    "name": "special_playlist",
                    "value": "not_playlist_of_the_day"
                }
            ]
        })");
        UNIT_ASSERT_VALUES_EQUAL(SPECIAL_PLAYLIST, ExtractMusicType(json));
    }

    Y_UNIT_TEST(EntityType) {
        const auto json = JsonFromString(R"({
            "slots": [
                {
                    "name": "answer",
                    "value": {
                        "type": "album"
                    }
                },
                {
                    "name": "foo"
                },
            ]
        })");
        UNIT_ASSERT_VALUES_EQUAL(ENTITY, ExtractMusicType(json));
    }

    Y_UNIT_TEST(UnknownType) {
        const auto json = JsonFromString(R"({
            "slots": [
                {
                    "name": "answer",
                    "value": {
                        "type": "foo"
                    }
                },
                {
                    "name": "foo"
                },
            ]
        })");
        UNIT_ASSERT_VALUES_EQUAL("foo", ExtractMusicType(json));
    }
}
