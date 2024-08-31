#include "datasync.h"

#include <library/cpp/json/json_writer.h>
#include <library/cpp/protobuf/json/string_transform.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/strbuf.h>

using namespace NAlice::NCuttlefish::NAppHostServices;

namespace {

    const TString ADDRESS = R"({
        "items": [{
            "last_used": "2016-04-07T23:22:48.010000+00:00",
            "address_id": "home",
            "tags": [],
            "title": "Home",
            "modified": "2016-04-07T23:22:48.010000+00:00",
            "longitude": 12.345,
            "created": "2016-04-07T23:22:48.010000+00:00",
            "mined_attributes": [],
            "address_line_short": "Baker street, 221b",
            "latitude": 23.456,
            "address_line": "London, Baker street, 221b"
        },
        {
            "last_used": "2016-04-07T23:22:48.011000+00:00",
            "address_id": "work",
            "tags": [],
            "title": "Work",
            "modified": "2016-04-07T23:22:48.011000+00:00",
            "longitude": 34.567,
            "created": "2016-04-07T23:22:48.011000+00:00",
            "mined_attributes": [],
            "address_line_short": "Tolstogo street, 16",
            "latitude": 45.678,
            "address_line": "Moscow, Tolstogo street, 16"
        }],
        "total": 2,
        "limit": 20,
        "offset": 0
    })";

    const TString KEY_VALUE = R"({
        "items": [{
            "id": "AutomotivePromoCounters",
            "value": {
                "auto_music_promo_2020": 3
           }
        },
        {
            "id": "alice_proactivity",
            "value": "enabled"
        },
        {
            "id": "gender",
            "value": "male"
        },
        {
            "id": "guest_uid",
            "value": "1234567"
        },
        {
            "id": "morning_show",
            "value": {
                "last_push_timestamp": 1633374073,
                "pushes_sent": 2
            }
        },
        {
            "id": "proactivity_history",
            "value": {}
        },
        {
            "id": "user_name",
            "value": "Swarley"
        },
        {
            "id": "video_rater_proactivity_history",
            "value": {
                "LastShowTime": "1593005794"
            }
        },
        {
            "id": "yandexstation_123456_location",
            "value": {
                "location": "Moscow"
            }
        }]
    })";

    const TString SETTINGS = R"({
        "items": [{
            "do_not_use_user_logs": true
        }]
    })";

    TString Unescape(TString source) {
        NProtobufJson::TDoubleUnescapeTransform().Transform(source);
        return source;
    }

    TString MakeHttpResponseWithItems(const TVector <TString> &items) {
        NPrivateStringBuilder::TStringBuilder sb;

        sb << "{\"items\":[";
        for (auto it = items.begin(); it != items.end(); it++) {
            if (it != items.begin()) {
                sb << ",";
            }
            sb << R"({"body":")" << *it << R"("})";
        }
        sb << "]}";

        return sb;
    }

}

namespace TDatasyncClientTests {

    Y_UNIT_TEST_SUITE(TDatasyncClientTests::WhenNoDatasyncItemsProvided) {
        Y_UNIT_TEST(ShouldReturnResponseWithEmptyItems) {
            const auto& content = TDatasyncClient::MakeVinsContextsResponseContent(
                /* Address: */Nothing(),
                /* KeyValue: */Nothing(),
                /* Settings: */Nothing());

            UNIT_ASSERT_STRINGS_EQUAL(Unescape(NJson::WriteJson(content, false)), MakeHttpResponseWithItems({}));
        }
    }

    Y_UNIT_TEST_SUITE(TDatasyncClientTests::WhenOnlyDatasyncAddressResponseProvided) {
        Y_UNIT_TEST(ShouldReturnResponseWithSingleAddressItem) {
            const auto& content = TDatasyncClient::MakeVinsContextsResponseContent(
                ADDRESS,
                /* KeyValue: */Nothing(),
                /* Settings: */Nothing());

            UNIT_ASSERT_STRINGS_EQUAL(Unescape(NJson::WriteJson(content, false)), MakeHttpResponseWithItems({
                ADDRESS
            }));
        }
    }

    Y_UNIT_TEST_SUITE(TDatasyncClientTests::WhenOnlyDatasyncKeyValueResponseProvided) {
        Y_UNIT_TEST(ShouldReturnResponseWithSingleKeyValueItem) {
            const auto &content = TDatasyncClient::MakeVinsContextsResponseContent(
                /* Address: */Nothing(),
                KEY_VALUE,
                /* Settings: */Nothing());

            UNIT_ASSERT_STRINGS_EQUAL(Unescape(NJson::WriteJson(content, false)), MakeHttpResponseWithItems({
                KEY_VALUE
            }));
        }
    }

    Y_UNIT_TEST_SUITE(TDatasyncClientTests::WhenOnlyDatasyncSettingsResponseProvided) {
        Y_UNIT_TEST(ShouldReturnResponseWithSingleSettingsItem) {
            const auto& content = TDatasyncClient::MakeVinsContextsResponseContent(
                /* Address: */Nothing(),
                /* KeyValue: */Nothing(),
                SETTINGS);

            UNIT_ASSERT_STRINGS_EQUAL(Unescape(NJson::WriteJson(content, false)), MakeHttpResponseWithItems({
                SETTINGS
            }));
        }
    }

    Y_UNIT_TEST_SUITE(TDatasyncClientTests::WhenFullDatasyncResponseProvided) {
        Y_UNIT_TEST(ShouldReturnResponseWithAllItems) {
            const auto& content = TDatasyncClient::MakeVinsContextsResponseContent(
                ADDRESS,
                KEY_VALUE,
                SETTINGS);

            UNIT_ASSERT_STRINGS_EQUAL(Unescape(NJson::WriteJson(content, false)), MakeHttpResponseWithItems({
                ADDRESS,
                KEY_VALUE,
                SETTINGS
            }));
        }
    }
}
