#include "utils.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/bass/ut/helpers.h>

#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NVideoCommon;
using namespace NBASS;
using namespace NBASS::NVideo;
using namespace NTestingHelpers;

namespace {
NSc::TValue CONTEXT_LAST_WATCHED_TV_SHOW = NSc::TValue::FromJson(R"(
{
    "form": {
        "name": "personal_assistant.scenarios.video_play",
        "slots": []
    },
    "meta": {
        "epoch": 1526559187,
        "tz": "Europe/Moscow",
        "device_state": {
            "last_watched": {
                "tv_shows": [{
                    "tv_show_item": {
                        "timestamp": 1526492857,
                        "provider_name": "ivi",
                        "provider_item_id": "7312",
                        "progress": {
                            "played": 4,
                            "duration": 413
                        }
                    },
                    "item": {
                        "timestamp": 1526492857,
                        "season": 2,
                        "episode": 3,
                        "provider_name": "ivi",
                        "provider_item_id": "43853",
                        "progress": {
                            "played": 4,
                            "duration": 413
                        }
                    }
                }]
            }
        },
        "experiments": {
            "video_unban_ivi" : 1,
            "video_disable_webview_video_entity_seasons" : 1,
            "video_disable_webview_video_entity" : 1
        }
    }
}
)");

constexpr TStringBuf DESCRIPTION_ANALYTICS_INFO = TStringBuf(R"({
    "Intent":"personal_assistant.scenarios.video_play",
    "Objects": [
        {
            "Id":"1",
            "HumanReadable":"Film or serial description screen",
            "VideoDescriptionScreen": {
                "Item": {
                    "Url": "name2/web/url",
                    "Type": 1,
                    "Description": "name2 description",
                    "Name": "name2",
                    "KinopoiskId": "2345",
                    "Source":7
                }
            },
            "Name":"description"
        }
    ]
})");

constexpr TStringBuf SEARCH_GALLERY_ANALYTICS_INFO = TStringBuf(R"({
    "Intent":"personal_assistant.scenarios.video_play",
    "Objects": [
        {
            "VideoSearchGalleryScreen": {
                "Items": [
                    {
                        "Url": "name1/web/url",
                        "Type": 2,
                        "Name": "name1",
                        "KinopoiskId": "1234",
                        "Source": 7
                    },
                    {
                        "Url": "name2/web/url",
                        "Type": 1,
                        "Name": "name2",
                        "KinopoiskId": "2345",
                        "Source": 7
                    },
                    {
                        "Url": "name3/web/url",
                        "Type": 3,
                        "Name": "name3",
                        "KinopoiskId": "3456",
                        "Source": 7
                    }
                ]
            },
            "Id": "2",
            "HumanReadable": "Film, serial or video search gallery",
            "Name": "gallery"
        }
    ]
})");

constexpr TStringBuf SEASON_GALLERY_ANALYTICS_INFO = TStringBuf(R"({
    "Intent": "personal_assistant.scenarios.video_play",
    "Objects": [
        {
            "Id": "3",
            "HumanReadable": "Gallery with episodes",
            "VideoSeasonGalleryScreen": {
                "Episodes": [
                    "name1",
                    "name2",
                    "name3"
                ],
                "Parent": {
                    "Url": "tv_show_name/web/url",
                    "Type": 2,
                    "Name": "tv_show_name",
                    "KinopoiskId": "",
                    "Source":7
                }
            },
            "Name": "season_gallery"
        }
    ]
})");

void RunTestForAgeRestriction(const TVideoItem& item, EContentRestrictionLevel mode, bool isPornoQuery,
                              bool isFromGallery, bool isPlayerContinue, bool expectedResult) {
    auto ctxPtr = NTestingHelpers::CreateVideoContextWithAgeRestriction(
        mode, [](const NSc::TValue& context) { return MakeContext(context); });
    UNIT_ASSERT(ctxPtr);

    TContext& ctx = *ctxPtr;
    TCurrentVideoState videoState{.IsFromGallery = isFromGallery, .IsPlayerContinue = isPlayerContinue};
    const TAgeCheckerDelegate ageChecker(ctx, videoState, isPornoQuery);
    UNIT_ASSERT_EQUAL(ageChecker.PassesAgeRestriction(item.Scheme()), expectedResult);
}

void TestGalleryAgeFilter(EContentRestrictionLevel mode, const TVideoGallery& gallery, bool isPornoQuery,
                          bool isFromGallery, ui32 expectedItemsCount, bool shouldHaveFilteredAttention,
                          bool shouldHaveEmptyGalleryAttention) {
    auto ctxPtr = CreateVideoContextWithAgeRestriction(mode);
    UNIT_ASSERT(ctxPtr);
    TContext& ctx = *ctxPtr;
    TCurrentVideoState videoState{.IsFromGallery = isFromGallery};
    TAgeCheckerDelegate ageChecker(ctx, videoState, isPornoQuery);
    auto filteredGallery = gallery;
    bool hasAttentions = FilterSearchGalleryOrAddAttentions(filteredGallery, ctx, ageChecker);
    UNIT_ASSERT_EQUAL(filteredGallery->Items().Size(), expectedItemsCount);
    UNIT_ASSERT_EQUAL(IsAttentionInContext(ctx, ATTENTION_ALL_RESULTS_FILTERED), shouldHaveFilteredAttention);
    UNIT_ASSERT_EQUAL(IsAttentionInContext(ctx, ATTENTION_EMPTY_SEARCH_GALLERY), shouldHaveEmptyGalleryAttention);
    UNIT_ASSERT_EQUAL(hasAttentions, shouldHaveEmptyGalleryAttention || shouldHaveFilteredAttention);
}

Y_UNIT_TEST_SUITE(BassVideoUtilsTestSuite) {
    Y_UNIT_TEST(ResolveTvShowSmoke) {
        UNIT_ASSERT(!CONTEXT_LAST_WATCHED_TV_SHOW.IsNull());
        const auto ctx = NTestingHelpers::MakeContext(CONTEXT_LAST_WATCHED_TV_SHOW);
        UNIT_ASSERT(ctx);

        NSc::TValue item;
        item["provider_name"] = "ivi";
        item["provider_item_id"] = "7312";

        const auto index =
            ResolveEpisodeIndex(TVideoItemConstScheme(&item), Nothing() /* season */, Nothing() /* episode */, *ctx);
        UNIT_ASSERT_VALUES_EQUAL(index.Season, TSerialIndex(static_cast<ui32>(1)));
        UNIT_ASSERT_VALUES_EQUAL(index.Episode, TSerialIndex(static_cast<ui32>(2)));
    }

    Y_UNIT_TEST(MergeDuplicatesAndFillProvidersInfoSmoke) {
        const auto zveropolisIvi = NSc::TValue::FromJson(R"({
            "type": "movie",
            "provider_name": "ivi",
            "provider_item_id": "12345",
            "name": "Зверополис",
            "misc_ids": {
                "kinopoisk": "1111"
            }
        })");

        const auto ironSkyIvi = NSc::TValue::FromJson(R"({
            "type": "movie",
            "provider_name": "ivi",
            "provider_item_id": "54321",
            "name": "Железное небо",
            "misc_ids": {
                "kinopoisk": "2222"
            }
        })");

        const auto zveropolisIviAux = NSc::TValue::FromJson(R"({
            "type": "movie",
            "provider_name": "ivi",
            "provider_item_id": "56789",
            "name": "Зверополис + дополнительные материалы",
            "misc_ids": {
                "kinopoisk": "1111"
            }
        })");

        const auto zveropolisAmediateka = NSc::TValue::FromJson(R"({
            "type": "movie",
            "provider_name": "amediateka",
            "provider_item_id": "zveropolis",
            "name": "Зверополис",
            "misc_ids": {
                "kinopoisk": "1111"
            }
        })");

        const auto ironSkyAmediteka = NSc::TValue::FromJson(R"({
            "type": "movie",
            "provider_name": "amediateka",
            "provider_item_id": "ironsky",
            "name": "Железное небо",
            "misc_ids": {
                "kinopoisk": "2222"
            }
        })");

        const auto marsAttacks = NSc::TValue::FromJson(R"({
            "type": "movie",
            "provider_name": "ivi",
            "provider_item_id": "1357",
            "name": "Марс атакует!"
        })");

        TVideoGallery gallery;
        for (const auto& item :
             {zveropolisIvi, ironSkyAmediteka, zveropolisIviAux, zveropolisAmediateka, ironSkyIvi, marsAttacks}) {
            gallery->Items().Add() = TVideoItemConstScheme(&item);
        }

        const auto ctx = NTestingHelpers::MakeContext(CONTEXT_LAST_WATCHED_TV_SHOW);
        UNIT_ASSERT(ctx);

        MergeDuplicatesAndFillProvidersInfo(gallery, *ctx);

        const auto expectedGallery = NSc::TValue::FromJson(R"({
            "items" : [
                {
                    "available" : 1,
                    "misc_ids" : {
                        "kinopoisk" : "1111"
                    },
                    "name" : "Зверополис",
                    "provider_info" : [
                        {
                            "misc_ids": {
                                "kinopoisk": "1111"
                            },
                            "provider_item_id" : "12345",
                            "provider_name" : "ivi",
                            "type" : "movie"
                        },
                        {
                            "misc_ids": {
                                "kinopoisk": "1111"
                            },
                            "provider_item_id" : "zveropolis",
                            "provider_name" : "amediateka",
                            "type" : "movie"
                        }
                    ],
                    "provider_item_id" : "12345",
                    "provider_name" : "ivi",
                    "type" : "movie"
                },
                {
                    "available" : 1,
                    "misc_ids" : {
                        "kinopoisk" : "2222"
                    },
                    "name" : "Железное небо",
                    "provider_info" : [
                        {
                            "misc_ids" : {
                                "kinopoisk" : "2222"
                            },
                            "provider_item_id" : "ironsky",
                            "provider_name" : "amediateka",
                            "type" : "movie"
                        },
                        {
                            "misc_ids" : {
                                "kinopoisk" : "2222"
                            },
                            "provider_item_id" : "54321",
                            "provider_name" : "ivi",
                            "type" : "movie"
                        }
                    ],
                    "provider_item_id" : "ironsky",
                    "provider_name" : "amediateka",
                    "type" : "movie"
                },
                {
                    "available" : 1,
                    "misc_ids" : {
                        "kinopoisk" : "1111"
                    },
                    "name" : "Зверополис + дополнительные материалы",
                    "provider_info" : [
                        {
                            "misc_ids" : {
                                "kinopoisk" : "1111"
                            },
                            "provider_item_id" : "56789",
                            "provider_name" : "ivi",
                            "type" : "movie"
                        }
                    ],
                    "provider_item_id" : "56789",
                    "provider_name" : "ivi",
                    "type" : "movie"
                },
                {
                    "available" : 1,
                    "name" : "Марс атакует!",
                    "provider_info" : [
                        {
                            "provider_item_id" : "1357",
                            "provider_name" : "ivi",
                            "type" : "movie"
                        }
                    ],
                    "provider_item_id" : "1357",
                    "provider_name" : "ivi",
                    "type" : "movie"
                }
            ]
        })");

        UNIT_ASSERT(NTestingHelpers::EqualJson(expectedGallery, gallery.Value()));
    }

    Y_UNIT_TEST(MergeDuplicatesAndFillProvidersInfoNonEmptyProvidersInfo) {
        const auto zveropolisIvi = NSc::TValue::FromJson(R"({
            "type": "movie",
            "provider_name": "ivi",
            "provider_item_id": "12345",
            "name": "Зверополис",
            "misc_ids": {
                "kinopoisk": "1111"
            },
            "provider_info": [{
                "misc_ids": {
                    "kinopoisk": "1111"
                },
                "provider_name": "ivi",
                "provider_item_id": "12345",
            }]
        })");

        const auto ironSkyIvi = NSc::TValue::FromJson(R"({
            "type": "movie",
            "provider_name": "ivi",
            "provider_item_id": "54321",
            "name": "Железное небо",
            "misc_ids": {
                "kinopoisk": "2222"
            }
        })");

        TVideoGallery gallery;
        for (const auto& item : {zveropolisIvi, ironSkyIvi})
            gallery->Items().Add() = TVideoItemConstScheme(&item);

        const auto ctx = NTestingHelpers::MakeContext(CONTEXT_LAST_WATCHED_TV_SHOW);
        UNIT_ASSERT(ctx);

        MergeDuplicatesAndFillProvidersInfo(gallery, *ctx);

        const auto expectedGallery = NSc::TValue::FromJson(R"({
            "items" : [
                {
                    "available" : 1,
                    "misc_ids" : {
                        "kinopoisk" : "1111"
                    },
                    "name" : "Зверополис",
                    "provider_info" : [
                        {
                            "misc_ids": {
                                "kinopoisk": "1111"
                            },
                            "provider_item_id" : "12345",
                            "provider_name" : "ivi"
                        }
                    ],
                    "provider_item_id" : "12345",
                    "provider_name" : "ivi",
                    "type" : "movie"
                },
                {
                    "available" : 1,
                    "misc_ids" : {
                        "kinopoisk" : "2222"
                    },
                    "name" : "Железное небо",
                    "provider_info" : [
                        {
                            "misc_ids" : {
                                "kinopoisk" : "2222"
                            },
                            "provider_item_id" : "54321",
                            "provider_name" : "ivi",
                            "type" : "movie"
                        }
                    ],
                    "provider_item_id" : "54321",
                    "provider_name" : "ivi",
                    "type" : "movie"
                }
            ]
        })");

        UNIT_ASSERT(NTestingHelpers::EqualJson(expectedGallery, gallery.Value()));
    }

    Y_UNIT_TEST(MergeDuplicatesAndFillProvidersInfoKeepAsIs) {
        const auto zveropolisIvi = NSc::TValue::FromJson(R"({
            "type": "movie",
            "provider_name": "ivi",
            "provider_item_id": "12345",
            "name": "Зверополис",
            "misc_ids": {
                "kinopoisk": "1111"
            },
            "provider_info": [{
                "provider_name": "ivi",
                "provider_item_id": "12345",
            }, {
                "provider_name": "amediateka",
                "provider_item_id": "zveropolis",
            }]
        })");

        const auto zveropolisAmediteka = NSc::TValue::FromJson(R"({
            "type": "movie",
            "provider_name": "amediateka",
            "provider_item_id": "zveropolis",
            "name": "Зверополис",
            "misc_ids": {
                "kinopoisk": "1111"
            }
        })");

        TVideoGallery gallery;
        for (const auto& item : {zveropolisIvi, zveropolisAmediteka})
            gallery->Items().Add() = TVideoItemConstScheme(&item);

        const auto ctx = NTestingHelpers::MakeContext(CONTEXT_LAST_WATCHED_TV_SHOW);
        UNIT_ASSERT(ctx);

        MergeDuplicatesAndFillProvidersInfo(gallery, *ctx);

        const auto expectedGallery = NSc::TValue::FromJson(R"({
            "items" : [
                {
                    "available" : 1,
                    "misc_ids" : {
                        "kinopoisk" : "1111"
                    },
                    "name" : "Зверополис",
                    "provider_info" : [
                        {
                            "provider_item_id" : "12345",
                            "provider_name" : "ivi"
                        },
                        {
                            "provider_item_id" : "zveropolis",
                            "provider_name" : "amediateka"
                        }
                    ],
                    "provider_item_id" : "12345",
                    "provider_name" : "ivi",
                    "type" : "movie"
                },
                {
                    "available" : 1,
                    "misc_ids" : {
                        "kinopoisk" : "1111"
                    },
                    "name" : "Зверополис",
                    "provider_info" : [
                        {
                            "misc_ids" : {
                                "kinopoisk" : "1111"
                            },
                            "provider_item_id" : "zveropolis",
                            "provider_name" : "amediateka",
                            "type" : "movie"
                        }
                    ],
                    "provider_item_id" : "zveropolis",
                    "provider_name" : "amediateka",
                    "type" : "movie"
                }
            ]
        })");

        UNIT_ASSERT(NTestingHelpers::EqualJson(expectedGallery, gallery.Value()));
    }

    Y_UNIT_TEST(MergeDifferentTypes) {
        const auto vinnieThePoohMovie = NSc::TValue::FromJson(R"({
            "provider_item_id": "4f59ccc89de60c68ab6c97e8b3a80dc6",
            "provider_name": "kinopoisk",
            "type": "movie",
            "misc_ids": {
                "kinopoisk": "45779",
                "kinopoisk_uuid": "4f59ccc89de60c68ab6c97e8b3a80dc6"
            }
        })");

        const auto vinnieThePoohTvShow = NSc::TValue::FromJson(R"({
            "human_readable_id": "vinni-puh_i_vse_vse_vse",
            "provider_item_id": "6871",
            "provider_name": "ivi",
            "type": "tv_show",
            "misc_ids": {
                "kinopoisk": "45779",
                "kinopoisk_uuid": "4f59ccc89de60c68ab6c97e8b3a80dc6"
            }
        })");

        TVideoGallery gallery;
        for (const auto& item : {vinnieThePoohMovie, vinnieThePoohTvShow})
            gallery->Items().Add() = TVideoItemConstScheme(&item);

        const auto ctx = NTestingHelpers::MakeContext(CONTEXT_LAST_WATCHED_TV_SHOW);
        UNIT_ASSERT(ctx);

        MergeDuplicatesAndFillProvidersInfo(gallery, *ctx);

        const auto expectedGallery = NSc::TValue::FromJson(R"({
            "items" : [
                {
                    "available" : 1,
                    "misc_ids" : {
                        "kinopoisk" : "45779",
                        "kinopoisk_uuid" : "4f59ccc89de60c68ab6c97e8b3a80dc6"
                    },
                    "provider_info" : [
                        {
                            "misc_ids" : {
                                "kinopoisk" : "45779",
                                "kinopoisk_uuid" : "4f59ccc89de60c68ab6c97e8b3a80dc6"
                            },
                            "provider_item_id" : "4f59ccc89de60c68ab6c97e8b3a80dc6",
                            "provider_name" : "kinopoisk",
                            "type" : "movie"
                        }
                    ],
                    "provider_item_id" : "4f59ccc89de60c68ab6c97e8b3a80dc6",
                    "provider_name" : "kinopoisk",
                    "type" : "movie"
                },
                {
                    "available" : 1,
                    "human_readable_id" : "vinni-puh_i_vse_vse_vse",
                    "misc_ids" : {
                        "kinopoisk" : "45779",
                        "kinopoisk_uuid" : "4f59ccc89de60c68ab6c97e8b3a80dc6"
                    },
                    "provider_info" : [
                        {
                            "misc_ids" : {
                                "kinopoisk" : "45779",
                                "kinopoisk_uuid" : "4f59ccc89de60c68ab6c97e8b3a80dc6"
                            },
                            "human_readable_id" : "vinni-puh_i_vse_vse_vse",
                            "provider_item_id" : "6871",
                            "provider_name" : "ivi",
                            "type" : "tv_show"
                        }
                    ],
                    "provider_item_id" : "6871",
                    "provider_name" : "ivi",
                    "type" : "tv_show"
                }
            ]
        })");

        UNIT_ASSERT(NTestingHelpers::EqualJson(expectedGallery, *gallery->GetRawValue()));
    }

    Y_UNIT_TEST(IsPornoQuery) {
        {
            auto ctxPtr = NTestingHelpers::CreateVideoContextWithAgeRestriction(
                EContentRestrictionLevel::Without, [](const NSc::TValue& context) {
                    auto newCtx = context;
                    newCtx["meta"]["is_porn_query"] = true;
                    return MakeContext(newCtx);
                });
            UNIT_ASSERT(ctxPtr);
            UNIT_ASSERT(IsPornoQuery(*ctxPtr));
        }

        {
            auto ctxPtr = NTestingHelpers::CreateVideoContextWithAgeRestriction(
                EContentRestrictionLevel::Without, [](const NSc::TValue& context) {
                    auto newCtx = context;
                    newCtx["meta"]["is_porn_query"] = false;
                    return MakeContext(newCtx);
                });
            UNIT_ASSERT(ctxPtr);
            UNIT_ASSERT(!IsPornoQuery(*ctxPtr));
        }
    }

    Y_UNIT_TEST(MarkVideoItemUnauthorizedSmoke) {
        const auto init = NSc::TValue::FromJson(R"({
        })");

        const auto expected = NSc::TValue::FromJson(R"({
            "unauthorized": 1
        })");

        TVideoItem item(init);
        MarkVideoItemUnauthorized(item);
        UNIT_ASSERT_VALUES_EQUAL(item.Value(), expected);
    }

    Y_UNIT_TEST(MarkVideoItemUnauthorizedSimple) {
        const auto init = NSc::TValue::FromJson(R"({
            "provider_info": [
               {"provider_name": "ivi"},
               {"provider_name": "kinopoisk"},
               {"provider_name": "amediateka"}
            ],
            "unauthorized": 0
        })");

        const auto expected = NSc::TValue::FromJson(R"({
            "provider_info": [
               {"provider_name": "ivi"},
               {"provider_name": "kinopoisk"},
               {"provider_name": "amediateka"}
            ],
            "unauthorized": 1
        })");

        TVideoItem item(init);
        MarkVideoItemUnauthorized(item);
        UNIT_ASSERT_VALUES_EQUAL(item.Value(), expected);
    }

    Y_UNIT_TEST(MarkShowPayScreenCommandDataUnauthorizedSmoke) {
        const auto init = NSc::TValue::FromJson(R"({
            "tv_show_item": {
                "provider_info": [
                   {"provider_name": "ivi"},
                   {"provider_name": "kinopoisk"},
                   {"provider_name": "amediateka"}
                ]
            }
        })");

        TShowPayScreenCommandData command(init);
        MarkShowPayScreenCommandDataUnauthorized(command);

        // No change in value because command data does not have
        // "item".
        UNIT_ASSERT_VALUES_EQUAL(command.Value(), init);
    }

    Y_UNIT_TEST(MarkShowPayScreenCommandDataUnauthorizedSimple) {
        const auto init = NSc::TValue::FromJson(R"({
            "item": {
                "provider_info": [
                   {"provider_name": "ivi"},
                   {"provider_name": "amediateka"}
                ]
            },
            "tv_show_item": {
                "provider_info": [
                   {"provider_name": "ivi"},
                   {"provider_name": "kinopoisk"},
                   {"provider_name": "amediateka"}
                ]
            }
        })");

        const auto expected = NSc::TValue::FromJson(R"({
            "item": {
                "provider_info": [
                   {"provider_name": "ivi"},
                   {"provider_name": "amediateka"}
                ],
                "unauthorized": 1
            },
            "tv_show_item": {
                "provider_info": [
                   {"provider_name": "ivi"},
                   {"provider_name": "kinopoisk"},
                   {"provider_name": "amediateka"}
                ]
            }
        })");

        TShowPayScreenCommandData command(init);
        MarkShowPayScreenCommandDataUnauthorized(command);
        UNIT_ASSERT_VALUES_EQUAL(command.Value(), expected);
    }

    Y_UNIT_TEST(KinopoiskAgeRestriction) {
        TVideoItem sixteenPlus;
        sixteenPlus->MinAge() = 16;
        sixteenPlus->AgeLimit() = "16";

        TVideoItem forteenPlus;
        forteenPlus->MinAge() = 14;
        forteenPlus->AgeLimit() = "14";

        TVideoItem sixPlus;
        sixPlus->MinAge() = 6;
        sixPlus->AgeLimit() = "6";

        TVideoItem porn;
        porn->MinAge() = 14;
        porn->AgeLimit() = "14";
        porn->Genre() = "porn";

        { // Check that 14+ content isn't available for little children.
            RunTestForAgeRestriction(forteenPlus, EContentRestrictionLevel::Safe, false /* isPornoQuery */,
                                     false /* isFromGallery */, false /* isPlayerContinue */,
                                     false /* expectedResult */);
        }
        { // Check that 16+ content isn't available for children.
            RunTestForAgeRestriction(sixteenPlus, EContentRestrictionLevel::Children, false /* isPornoQuery */,
                                     false /* isFromGallery */, false /* isPlayerContinue */,
                                     false /* expectedResult */);
        }
        { // Check that 16+ content is available for medium settings.
            RunTestForAgeRestriction(sixteenPlus, EContentRestrictionLevel::Medium, false /* isPornoQuery */,
                                     false /* isFromGallery */, false /* isPlayerContinue */,
                                     true /* expectedResult */);
        }
        { // Check that 14+ content is available for children.
            RunTestForAgeRestriction(forteenPlus, EContentRestrictionLevel::Children, false /* isPornoQuery */,
                                     false /* isFromGallery */, false /* isPlayerContinue */,
                                     true /* expectedResult */);
        }
        { // Check that 6+ content is available for little children.
            RunTestForAgeRestriction(sixPlus, EContentRestrictionLevel::Safe, false /* isPornoQuery */,
                                     false /* isFromGallery */, false /* isPlayerContinue */,
                                     true /* expectedResult */);
        }
        { // Check that porn is not available for children, independently of item's min_age.
            RunTestForAgeRestriction(porn, EContentRestrictionLevel::Children, false /* isPornoQuery */,
                                     false /* isFromGallery */, false /* isPlayerContinue */,
                                     false /* expectedResult */);
        }
        { // Check that porn is not available for children, independently of item's min_age, even by a direct request.
            RunTestForAgeRestriction(porn, EContentRestrictionLevel::Children, true /* isPornoQuery */,
                                     false /* isFromGallery */, false /* isPlayerContinue */,
                                     false /* expectedResult */);
        }
        { // Check that porn is available in family mode by direct request.
            RunTestForAgeRestriction(porn, EContentRestrictionLevel::Medium, true /* isPornoQuery */,
                                     false /* isFromGallery */, false /* isPlayerContinue */,
                                     true /* expectedResult */);
        }
        { // Check that porn is available in family mode if we continue watching.
            RunTestForAgeRestriction(porn, EContentRestrictionLevel::Medium, false /* isPornoQuery */,
                                     false /* isFromGallery */, true /* isPlayerContinue */,
                                     true /* expectedResult */);
        }
    }

    Y_UNIT_TEST(EmptyGalleryResponseAttentions) {
        const auto safeMode = EContentRestrictionLevel::Safe;
        const auto childMode = EContentRestrictionLevel::Children;
        const auto familyMode = EContentRestrictionLevel::Medium;
        const auto fullMode = EContentRestrictionLevel::Without;

        TVideoGallery emptyGallery, childGallery, adultGallery, mixedGallery, videoGallery;
        {
            TVideoItem childItem;
            childItem->MinAge() = 6;
            childItem->AgeLimit() = "6";

            TVideoItem adultItem;
            adultItem->MinAge() = 18;
            adultItem->AgeLimit() = "18";

            TVideoItem unrestrictedItem;

            TVideoItem videoItem;
            videoItem->MinAge() = 18;
            videoItem->AgeLimit() = "18";
            videoItem->Type() = "video";

            TVideoItem pornItem;
            pornItem->Genre() = "porn";

            TVideoItem pornItemRu;
            pornItemRu->Genre() = "порн";

            childGallery->Items().Add() = childItem.Scheme();
            childGallery->Items().Add() = unrestrictedItem.Scheme();

            adultGallery->Items().Add() = adultItem.Scheme();
            adultGallery->Items().Add() = pornItem.Scheme();
            adultGallery->Items().Add() = pornItemRu.Scheme();

            mixedGallery->Items().Add() = childItem.Scheme();
            mixedGallery->Items().Add() = unrestrictedItem.Scheme();
            mixedGallery->Items().Add() = adultItem.Scheme();
            mixedGallery->Items().Add() = pornItem.Scheme();
            mixedGallery->Items().Add() = pornItemRu.Scheme();

            videoGallery->Items().Add() = videoItem.Scheme();
        }
        { // Keep all children items if in children context.
            TestGalleryAgeFilter(safeMode, childGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 1u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
            TestGalleryAgeFilter(childMode, childGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 1u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
        }
        { // Remove all adult items in children context and show "filtered" attention.
            TestGalleryAgeFilter(safeMode, adultGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 0u /* expectedItemsCount */, true /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
            TestGalleryAgeFilter(childMode, adultGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 0u /* expectedItemsCount */, true /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
        }
        { // Porn query should not affect child mode.
            TestGalleryAgeFilter(safeMode, adultGallery, true /* isPornoQuery */, false /* isFromGallery */,
                                 0u /* expectedItemsCount */, true /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
            TestGalleryAgeFilter(childMode, adultGallery, true /* isPornoQuery */, false /* isFromGallery */,
                                 0u /* expectedItemsCount */, true /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
        }
        { // Filter some items from mixed gallery in child mode.
            TestGalleryAgeFilter(safeMode, mixedGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 1u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
            TestGalleryAgeFilter(childMode, mixedGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 1u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
        }
        { // Empty search gallery should add empty gallery attention both for porn and non-porn queries.
            TestGalleryAgeFilter(safeMode, emptyGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 0u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 true /* shouldHaveEmptyGalleryAttention */);
            TestGalleryAgeFilter(childMode, emptyGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 0u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 true /* shouldHaveEmptyGalleryAttention */);
            TestGalleryAgeFilter(childMode, emptyGallery, true /* isPornoQuery */, false /* isFromGallery */,
                                 0u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 true /* shouldHaveEmptyGalleryAttention */);
        }

        { // Porn filtering in family mode.
            TestGalleryAgeFilter(familyMode, adultGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 1u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
            TestGalleryAgeFilter(familyMode, mixedGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 3u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
        }
        { // Porn filtering in family mode is disabled by direct request.
            TestGalleryAgeFilter(familyMode, adultGallery, true /* isPornoQuery */, false /* isFromGallery */,
                                 3u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
            TestGalleryAgeFilter(familyMode, mixedGallery, true /* isPornoQuery */, false /* isFromGallery */,
                                 5u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
        }

        { // Porn filtering is disabled in full mode.
            TestGalleryAgeFilter(fullMode, mixedGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 5u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
        }

        { // Video search filtering is disabled in family mode.
            TestGalleryAgeFilter(safeMode, videoGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 0u /* expectedItemsCount */, true /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
            TestGalleryAgeFilter(childMode, videoGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 1u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
            TestGalleryAgeFilter(familyMode, videoGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 1u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
            TestGalleryAgeFilter(fullMode, videoGallery, false /* isPornoQuery */, false /* isFromGallery */,
                                 1u /* expectedItemsCount */, false /* shouldHaveFilteredAttention */,
                                 false /* shouldHaveEmptyGalleryAttention */);
        }
    }

    Y_UNIT_TEST(FindLastWatchedItem) {
        {
            const auto request = NSc::TValue::FromJson(R"({
                "meta": {
                  "epoch": 1526559187,
                  "tz": "Europe/Moscow",
                  "device_state": {
                    "last_watched": {
                      "movies": [],
                      "tv_shows": [],
                      "videos": [
                        {
                          "play_uri": "youtube://RZq9E1NwbMA",
                          "progress": {
                            "duration": 676,
                            "played": 666
                          },
                          "provider_item_id": "RZq9E1NwbMA",
                          "provider_name": "youtube",
                          "timestamp": 1548331766
                        }
                      ]
                    },
                    "video": {
                      "currently_playing": {
                        "item": {
                          "human_readable_id": "",
                          "name": "LastWatchedVideo",
                          "provider_info": [
                            {
                              "available": 1,
                              "provider_item_id": "RZq9E1NwbMA",
                              "provider_name": "youtube",
                              "type": "video"
                            }
                          ],
                          "provider_item_id": "RZq9E1NwbMA",
                          "provider_name": "youtube",
                          "source_host": "www.youtube.com",
                          "thumbnail_url_16x9": "https://picture",
                          "thumbnail_url_16x9_small": "https://picture_small",
                          "type": "video",
                          "view_count": 100500
                        }
                      }
                    }
                  }
                }
               }
            )");
            auto ctxPtr = NTestingHelpers::MakeContext(request);
            UNIT_ASSERT(ctxPtr);
            NSc::TValue lastVideo = FindLastWatchedItem(*ctxPtr);
        }
        {
            const auto request = NSc::TValue::FromJson(R"({
                "meta": {
                  "epoch": 1526559187,
                  "tz": "Europe/Moscow",
                  "device_state": {
                    "last_watched": {
                      "movies": [],
                      "tv_shows": [],
                      "videos": [
                        {
                          "play_uri": "youtube://RZq9E1NwbMA",
                          "progress": {
                            "duration": 676,
                            "played": 666
                          },
                          "provider_item_id": "RZq9E1NwbMA",
                          "provider_name": "youtube",
                          "timestamp": 1548331766
                        }
                      ]
                    }
                  }
                }
               }
            )");
            auto ctxPtr = NTestingHelpers::MakeContext(request);
            UNIT_ASSERT(ctxPtr);
            NSc::TValue lastVideo = FindLastWatchedItem(*ctxPtr);
            UNIT_ASSERT(lastVideo["thumbnail_url_16x9"].IsNull());
            UNIT_ASSERT_VALUES_EQUAL(lastVideo["provider_name"].GetString(), TStringBuf("youtube"));
        }
    }

    Y_UNIT_TEST(FillFromProviderInfo) {
        TLightVideoItem info;
        info->Type() = ToString(EItemType::TvShow);
        info->ProviderItemId() = "0001";

        TVideoItem item;
        item->Type() = ToString(EItemType::TvShow);
        item->HumanReadableId() = "masha-i-medved";
        item->TvShowItemId() = "masha-i-medved-tv-show";
        item->Description() = "Веселый отечественный мультсериал про непоседливую девочку Машу и добродушного медведя";

        FillFromProviderInfo(info, item);

        const auto expected = NSc::TValue::FromJson(R"({
            "type": "tv_show",
            "provider_item_id": "0001",
            "description": "Веселый отечественный мультсериал про непоседливую девочку Машу и добродушного медведя"
        })");

        UNIT_ASSERT(NTestingHelpers::EqualJson(expected, item.Value()));
    }

    Y_UNIT_TEST(TestYaVideoAgeFilterParam) {
        auto checkParams = [](EContentRestrictionLevel restriction,
                              bool forceUnfiltered, const TCgiParameters& cgisTarget) {
            auto ctxPtr = CreateVideoContextWithAgeRestriction(restriction);
            UNIT_ASSERT(ctxPtr);
            TCgiParameters cgisResult;
            AddYaVideoAgeFilterParam(*ctxPtr, cgisResult, forceUnfiltered);
            UNIT_ASSERT(cgisResult == cgisTarget);
        };

        const TStringBuf filtrationRelevParam = "relev";
        const TStringBuf filtrationFamilyParam = "family";
        const TStringBuf filtrationAgeRestrictionLevelParam = "age_restriction_level";
        {
            TCgiParameters cgisTarget;
            cgisTarget.InsertUnescaped(filtrationFamilyParam, TStringBuf("moderate"));
            checkParams(EContentRestrictionLevel::Without, false /* forceUnfiltered */, cgisTarget);
        }
        {
            TCgiParameters cgisTarget;
            cgisTarget.InsertUnescaped(filtrationRelevParam, TStringBuf("pf=off"));
            checkParams(EContentRestrictionLevel::Without, true /* forceUnfiltered */, cgisTarget);
        }
        {
            TCgiParameters cgisTarget;
            cgisTarget.InsertUnescaped(filtrationFamilyParam, TStringBuf("moderate"));
            checkParams(EContentRestrictionLevel::Medium, false /* forceUnfiltered */, cgisTarget);
        }
        {
            TCgiParameters cgisTarget;
            cgisTarget.InsertUnescaped(filtrationRelevParam, TStringBuf("pf=off"));
            checkParams(EContentRestrictionLevel::Medium, true /* forceUnfiltered */, cgisTarget);
        }
        {
            TCgiParameters cgisTarget;
            cgisTarget.InsertUnescaped(filtrationRelevParam, TStringBuf("pf=strict"));
            cgisTarget.InsertUnescaped(filtrationAgeRestrictionLevelParam, TStringBuf("family"));
            checkParams(EContentRestrictionLevel::Children, false /* forceUnfiltered */, cgisTarget);
        }
        {
            TCgiParameters cgisTarget;
            cgisTarget.InsertUnescaped(filtrationRelevParam, TStringBuf("pf=off"));
            checkParams(EContentRestrictionLevel::Children, true /* forceUnfiltered */, cgisTarget);
        }
        {
            TCgiParameters cgisTarget;
            cgisTarget.InsertUnescaped(filtrationRelevParam, TStringBuf("pf=strict"));
            cgisTarget.InsertUnescaped(filtrationAgeRestrictionLevelParam, TStringBuf("kids"));
            checkParams(EContentRestrictionLevel::Safe, false /* forceUnfiltered */, cgisTarget);
        }
        {
            TCgiParameters cgisTarget;
            cgisTarget.InsertUnescaped(filtrationRelevParam, TStringBuf("pf=off"));
            checkParams(EContentRestrictionLevel::Safe, true /* forceUnfiltered */, cgisTarget);
        }
    }

    Y_UNIT_TEST(TestAddVideoCommands) {
        auto makeItem = [](const TString& name, const TString& type, const TString& providerId = "") {
            TVideoItem item;
            item->Name() = name;
            item->DebugInfo().WebPageUrl() = name + "/web/url";
            item->Description() = name + " description";
            item->Source() = VIDEO_SOURCE_CAROUSEL;
            item->Type() = type;
            item->ProviderName() = PROVIDER_KINOPOISK;
            item->ProviderItemId() = providerId;
            return item;
        };

        // Test description.
        {
            NJson::TJsonValue expected;
            NJson::ReadJsonTree(DESCRIPTION_ANALYTICS_INFO, &expected, true);
            const auto ctx = NTestingHelpers::MakeContext(CONTEXT_LAST_WATCHED_TV_SHOW);
            UNIT_ASSERT(ctx);

            ShowDescription(makeItem("name2", "movie", "2345").Scheme(), *ctx);
            const auto analyticsInfo = ctx->GetAnalyticsInfoBuilder().Build();
            NJson::TJsonValue result;
            NProtobufJson::Proto2Json(analyticsInfo, result);
            UNIT_ASSERT_VALUES_EQUAL(result, expected);
        }

        // Test gallery.
        {
            NJson::TJsonValue expected;
            NJson::ReadJsonTree(SEARCH_GALLERY_ANALYTICS_INFO, &expected, true);
            const auto ctx = NTestingHelpers::MakeContext(CONTEXT_LAST_WATCHED_TV_SHOW);
            UNIT_ASSERT(ctx);
            auto slots = TVideoSlots::TryGetFromContext(*ctx);

            NVideo::TVideoGallery gallery;
            gallery->Items().Add() = makeItem("name1", "tv_show", "1234").Scheme();
            gallery->Items().Add() = makeItem("name2", "movie", "2345").Scheme();
            gallery->Items().Add() = makeItem("name3", "video", "3456").Scheme();
            AddShowSearchGalleryResponse(gallery, *ctx, *slots);
            const auto analyticsInfo = ctx->GetAnalyticsInfoBuilder().Build();
            NJson::TJsonValue result;
            NProtobufJson::Proto2Json(analyticsInfo, result);
            UNIT_ASSERT_VALUES_EQUAL(result, expected);
        }

        // Test season.
        {
            NJson::TJsonValue expected;
            NJson::ReadJsonTree(SEASON_GALLERY_ANALYTICS_INFO, &expected, true);
            const auto ctx = NTestingHelpers::MakeContext(CONTEXT_LAST_WATCHED_TV_SHOW);
            UNIT_ASSERT(ctx);

            NVideo::TVideoGallery gallery;
            gallery->Items().Add() = makeItem("name1", "tv_show_item").Scheme();
            gallery->Items().Add() = makeItem("name2", "tv_show_item").Scheme();
            gallery->Items().Add() = makeItem("name3", "tv_show_item").Scheme();
            gallery->TvShowItem() = makeItem("tv_show_name", "tv_show").Scheme();
            AddShowSeasonGalleryResponse(gallery, *ctx, {} /* serialDescr */, Nothing() /* attention */);
            const auto analyticsInfo = ctx->GetAnalyticsInfoBuilder().Build();
            NJson::TJsonValue result;
            NProtobufJson::Proto2Json(analyticsInfo, result);
            UNIT_ASSERT_VALUES_EQUAL(result, expected);
        }
    }
}
} // namespace
