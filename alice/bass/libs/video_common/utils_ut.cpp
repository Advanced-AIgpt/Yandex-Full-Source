#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/amediateka_utils.h>
#include <alice/bass/libs/video_common/ivi_utils.h>

#include <alice/library/unittest/ut_helpers.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NVideoCommon;

namespace {
Y_UNIT_TEST_SUITE(Utils) {
    Y_UNIT_TEST(ParseDurationStringSmoke) {
        UNIT_ASSERT_VALUES_EQUAL(*ParseDurationString("0:10:25"), TDuration::Minutes(10) + TDuration::Seconds(25));
        UNIT_ASSERT_VALUES_EQUAL(*ParseDurationString("10:9:08"),
                                 TDuration::Hours(10) + TDuration::Minutes(9) + TDuration::Seconds(8));
        UNIT_ASSERT_VALUES_EQUAL(ParseDurationString("10:25").Defined(), false);
        UNIT_ASSERT_VALUES_EQUAL(ParseDurationString("25").Defined(), false);
        UNIT_ASSERT_VALUES_EQUAL(ParseDurationString("").Defined(), false);
        UNIT_ASSERT_VALUES_EQUAL(ParseDurationString("1:2:3:4").Defined(), false);
    }

    Y_UNIT_TEST(ParseIviItemFromUrl) {
        {
            TVideoItem item;
            UNIT_ASSERT(ParseIviItemFromUrl("https://www.ivi.ru/watch/10-friend-rabbit/161227", item));
            UNIT_ASSERT_VALUES_EQUAL(item->HumanReadableId(), "10-friend-rabbit");
            UNIT_ASSERT_VALUES_EQUAL(item->ProviderItemId(), "161227");
        }

        {
            TVideoItem item;
            UNIT_ASSERT(!ParseIviItemFromUrl("https://www.ivi.ru/watch/10-friend-rabbit/reviews/42584", item));
        }
    }

    Y_UNIT_TEST(ParseAmediatekaItemFromUrl) {
        {
            TVideoItem item;
            UNIT_ASSERT(ParseAmediatekaItemFromUrl("https://www.amediateka.ru/serial/zveri/1/3", item));
            UNIT_ASSERT_VALUES_EQUAL(item->HumanReadableId(), "zveri");
            UNIT_ASSERT_VALUES_EQUAL(item->Type(), ToString(EItemType::TvShow));
        }

        {
            TVideoItem item;
            UNIT_ASSERT(!ParseAmediatekaItemFromUrl("https://www.amediateka.ru/about", item));
        }
    }

    Y_UNIT_TEST(MergeItems) {
        TVideoItem base;
        base->Type() = ToString(EItemType::TvShow);
        base->ProviderName() = PROVIDER_IVI;
        base->ProviderItemId() = "12345";
        AddProviderInfoIfNeeded(base.Scheme(), base.Scheme());

        TVideoItem update = base;
        update->Name() = "Маша и медведь";

        {
            TLightVideoItem kinopoiskInfo;
            kinopoiskInfo->Type() = ToString(EItemType::TvShow);
            kinopoiskInfo->ProviderName() = PROVIDER_KINOPOISK;
            kinopoiskInfo->ProviderItemId() = "67890";
            kinopoiskInfo->MiscIds()->Kinopoisk() = "67890";
            AddProviderInfoIfNeeded(base.Scheme(), kinopoiskInfo.Scheme());
        }

        const auto expected = NSc::TValue::FromJson(R"({
                "name" : "Маша и медведь",
                "provider_info" : [
                    {
                        "provider_item_id" : "12345",
                        "provider_name" : "ivi",
                        "type" : "tv_show"
                    },
                    {
                        "misc_ids" : {
                            "kinopoisk" : "67890"
                        },
                        "provider_item_id" : "67890",
                        "provider_name" : "kinopoisk",
                        "type" : "tv_show"
                    }
                ],
                "provider_item_id" : "12345",
                "provider_name" : "ivi",
                "type" : "tv_show"
            })");

        TVideoItem result = MergeItems(base.Scheme(), update.Scheme());
        UNIT_ASSERT(NTestingHelpers::EqualJson(expected, result.Value()));
    }
}
} // namespace
