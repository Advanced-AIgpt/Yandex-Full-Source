#include "datasync_parser.h"

#include <apphost/lib/proto_answers/http.pb.h>
#include <library/cpp/protobuf/json/string_transform.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/strbuf.h>
#include <util/string/builder.h>

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

const TString HEADERS = R"({
    "Access-Control-Allow-Methods": "PUT, POST, GET, DELETE, OPTIONS",
    "Access-Control-Allow-Credentials": "true",
    "Yandex-Cloud-Request-ID": "rest-777b797b5cc6a976422050b75d3ec6fe-api01e",
    "Cache-Control": "no-cache",
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Headers": "Accept-Language, Accept, X-Uid, X-HTTP-Method, X-Requested-With, Content-Type, Authorization",
    "Content-Type": "application/json; charset=utf-8"
})";

static TString HTTP_RESPONSE(TString body) {
    NProtobufJson::TCEscapeTransform().Transform(body);
    return TStringBuilder() << R"({"body": ")" << body << R"(", "headers": )" << HEADERS << "}";
}

const TString ADDRESS_HTTP_RESPONSE = HTTP_RESPONSE(ADDRESS);
const TString KEY_VALUE_HTTP_RESPONSE = HTTP_RESPONSE(KEY_VALUE);
const TString SETTINGS_HTTP_RESPONSE = HTTP_RESPONSE(SETTINGS);

void AssertParsedAddresses(const NJson::TJsonValue& personalData) {
    const auto& homeAddress = personalData.GetMap().FindPtr("/v2/personality/profile/addresses/home")->GetMap();
    UNIT_ASSERT_EQUAL(homeAddress.FindPtr("address_id")->GetString(), "home");
    UNIT_ASSERT_EQUAL(homeAddress.FindPtr("address_line")->GetString(), "London, Baker street, 221b");
    UNIT_ASSERT_EQUAL(homeAddress.FindPtr("address_line_short")->GetString(), "Baker street, 221b");
    UNIT_ASSERT_EQUAL(homeAddress.FindPtr("created")->GetString(), "2016-04-07T23:22:48.010000+00:00");
    UNIT_ASSERT_EQUAL(homeAddress.FindPtr("last_used")->GetString(), "2016-04-07T23:22:48.010000+00:00");
    UNIT_ASSERT_EQUAL(homeAddress.FindPtr("latitude")->GetDouble(), 23.456);
    UNIT_ASSERT_EQUAL(homeAddress.FindPtr("longitude")->GetDouble(), 12.345);
    UNIT_ASSERT_EQUAL(homeAddress.FindPtr("mined_attributes")->GetArray().size(), 0);
    UNIT_ASSERT_EQUAL(homeAddress.FindPtr("modified")->GetString(), "2016-04-07T23:22:48.010000+00:00");
    UNIT_ASSERT_EQUAL(homeAddress.FindPtr("tags")->GetArray().size(), 0);
    UNIT_ASSERT_EQUAL(homeAddress.FindPtr("title")->GetString(), "Home");

    const auto& workAddress = personalData.GetMap().FindPtr("/v2/personality/profile/addresses/work")->GetMap();
    UNIT_ASSERT_EQUAL(workAddress.FindPtr("address_id")->GetString(), "work");
    UNIT_ASSERT_EQUAL(workAddress.FindPtr("address_line")->GetString(), "Moscow, Tolstogo street, 16");
    UNIT_ASSERT_EQUAL(workAddress.FindPtr("address_line_short")->GetString(), "Tolstogo street, 16");
    UNIT_ASSERT_EQUAL(workAddress.FindPtr("created")->GetString(), "2016-04-07T23:22:48.011000+00:00");
    UNIT_ASSERT_EQUAL(workAddress.FindPtr("last_used")->GetString(), "2016-04-07T23:22:48.011000+00:00");
    UNIT_ASSERT_EQUAL(workAddress.FindPtr("latitude")->GetDouble(), 45.678);
    UNIT_ASSERT_EQUAL(workAddress.FindPtr("longitude")->GetDouble(), 34.567);
    UNIT_ASSERT_EQUAL(workAddress.FindPtr("mined_attributes")->GetArray().size(), 0);
    UNIT_ASSERT_EQUAL(workAddress.FindPtr("modified")->GetString(), "2016-04-07T23:22:48.011000+00:00");
    UNIT_ASSERT_EQUAL(workAddress.FindPtr("tags")->GetArray().size(), 0);
    UNIT_ASSERT_EQUAL(workAddress.FindPtr("title")->GetString(), "Work");
}

void AssertParsedKeyValue(const NJson::TJsonValue& personalData) {
    UNIT_ASSERT_EQUAL(
            personalData.GetMap().FindPtr("/v1/personality/profile/alisa/kv/alice_proactivity")->GetString(),
            "enabled");
    UNIT_ASSERT_EQUAL(
            personalData.GetMap().FindPtr("/v1/personality/profile/alisa/kv/gender")->GetString(),
            "male");
    UNIT_ASSERT_EQUAL(
            personalData.GetMap().FindPtr("/v1/personality/profile/alisa/kv/guest_uid")->GetString(),
            "1234567");
    UNIT_ASSERT_EQUAL(
            personalData.GetMap().FindPtr("/v1/personality/profile/alisa/kv/user_name")->GetString(),
            "Swarley");
    UNIT_ASSERT_EQUAL(
            personalData.GetMap()
                    .FindPtr("/v1/personality/profile/alisa/kv/AutomotivePromoCounters")->GetMap()
                    .FindPtr("auto_music_promo_2020")->GetInteger(),
            3);
    UNIT_ASSERT_EQUAL(
            personalData.GetMap()
                    .FindPtr("/v1/personality/profile/alisa/kv/morning_show")->GetMap()
                    .FindPtr("last_push_timestamp")->GetInteger(),
            1633374073);
    UNIT_ASSERT_EQUAL(
            personalData.GetMap()
                    .FindPtr("/v1/personality/profile/alisa/kv/morning_show")->GetMap()
                    .FindPtr("pushes_sent")->GetInteger(),
            2);
    UNIT_ASSERT_EQUAL(
            personalData.GetMap()
                    .FindPtr("/v1/personality/profile/alisa/kv/video_rater_proactivity_history")->GetMap()
                    .FindPtr("LastShowTime")->GetString(),
            "1593005794");
    UNIT_ASSERT_EQUAL(
            personalData.GetMap()
                    .FindPtr("/v1/personality/profile/alisa/kv/yandexstation_123456_location")->GetMap()
                    .FindPtr("location")->GetString(),
            "Moscow");
}

using namespace NAlice::NCuttlefish::NAppHostServices;

namespace {

    TString MakeHttpResponseWithItems(const std::initializer_list<TString>& items) {
        NPrivateStringBuilder::TStringBuilder sb;

        sb << "{\"items\": [";
        for (auto it = items.begin(); it != items.end(); it++) {
            if (it != items.begin()) {
                sb << ", ";
            }
            sb << *it;
        }
        sb << "]}";

        return sb;
    }

    template <typename TParser>
    void ParseDatasyncResponseWithItems(TParser& parser, const std::initializer_list<TString>& items, int statusCode = 200) {
        NAppHostHttp::THttpResponse httpResponse;
        httpResponse.SetContent(MakeHttpResponseWithItems(items));
        httpResponse.SetStatusCode(statusCode);

        parser.ParseDatasyncResponse(httpResponse);
    }
};

namespace TDatasyncResponseParserTests {

    Y_UNIT_TEST_SUITE(TDatasyncResponseParserTests::WhenNoDatasyncItemsProvided) {
        Y_UNIT_TEST(ShouldThrowException) {
            TDatasyncResponseParser parser;
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                    ParseDatasyncResponseWithItems(parser, {}),
                    yexception,
                    "got unexpected number of response items");
        }
    };

    Y_UNIT_TEST_SUITE(TDatasyncResponseParserTests::WhenOnlyDatasyncAddressResponseProvided) {
        Y_UNIT_TEST(ShouldThrowException) {
            TDatasyncResponseParser parser;
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                    ParseDatasyncResponseWithItems(parser, {ADDRESS_HTTP_RESPONSE}),
                    yexception,
                    "got unexpected number of response items");
        }
    };

    Y_UNIT_TEST_SUITE(TDatasyncResponseParserTests::WhenOnlyDatasyncKeyValueResponseProvided) {
        Y_UNIT_TEST(ShouldThrowException) {
            TDatasyncResponseParser parser;
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                    ParseDatasyncResponseWithItems(parser, {KEY_VALUE_HTTP_RESPONSE}),
                    yexception,
                    "got unexpected number of response items");
        }
    };

    Y_UNIT_TEST_SUITE(TDatasyncResponseParserTests::WhenOnlyDatasyncSettingsResponseProvided) {
        Y_UNIT_TEST(ShouldThrowException) {
            TDatasyncResponseParser parser;
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                    ParseDatasyncResponseWithItems(parser, {SETTINGS_HTTP_RESPONSE}),
                    yexception,
                    "got unexpected number of response items");
        }
    };

    Y_UNIT_TEST_SUITE(TDatasyncResponseParserTests::WhenOnlyDatasyncAddressAndKeyValueResponseProvided) {
        Y_UNIT_TEST(ShouldParseDatasyncAddressAndKeyValue) {
            TDatasyncResponseParser parser;
            UNIT_ASSERT_NO_EXCEPTION(ParseDatasyncResponseWithItems(parser, {
                    ADDRESS_HTTP_RESPONSE,
                    KEY_VALUE_HTTP_RESPONSE}));

            AssertParsedAddresses(parser.PersonalData);
            AssertParsedKeyValue(parser.PersonalData);
            UNIT_ASSERT(!parser.DoNotUseUserLogs);
        }
    };

    Y_UNIT_TEST_SUITE(TDatasyncResponseParserTests::WhenFullDatasyncResponseProvided) {
        Y_UNIT_TEST(ShouldParseAllItems) {
            TDatasyncResponseParser parser;
            UNIT_ASSERT_NO_EXCEPTION(ParseDatasyncResponseWithItems(parser, {
                    ADDRESS_HTTP_RESPONSE,
                    KEY_VALUE_HTTP_RESPONSE,
                    SETTINGS_HTTP_RESPONSE
            }));

            AssertParsedAddresses(parser.PersonalData);
            AssertParsedKeyValue(parser.PersonalData);
            UNIT_ASSERT(parser.DoNotUseUserLogs);
        }
    };

    Y_UNIT_TEST_SUITE(TDatasyncResponseParserTests::WhenDatasyncResponseHasErrorStatusCode) {
        Y_UNIT_TEST(ShouldNotThrowAndNotParseEmptyResponse) {
            TDatasyncResponseParser parser;
            UNIT_ASSERT_NO_EXCEPTION(ParseDatasyncResponseWithItems(parser, {}, /*status_code: */ 500));

            UNIT_ASSERT(!parser.DoNotUseUserLogs);
            UNIT_ASSERT(!parser.PersonalData.Has("/v2/personality/profile/addresses/home"));
            UNIT_ASSERT(!parser.PersonalData.Has("/v2/personality/profile/addresses/work"));
            UNIT_ASSERT(!parser.PersonalData.Has("/v1/personality/profile/alisa/kv/guest_uid"));
        }

        Y_UNIT_TEST(ShouldNotThrowAndNotParseFullResponse) {
            TDatasyncResponseParser parser;
            UNIT_ASSERT_NO_EXCEPTION(ParseDatasyncResponseWithItems(parser, {
                    KEY_VALUE_HTTP_RESPONSE,
                    ADDRESS_HTTP_RESPONSE,
                    SETTINGS_HTTP_RESPONSE
            }, /*status_code: */ 500));

            UNIT_ASSERT(!parser.DoNotUseUserLogs);
            UNIT_ASSERT(!parser.PersonalData.Has("/v2/personality/profile/addresses/home"));
            UNIT_ASSERT(!parser.PersonalData.Has("/v2/personality/profile/addresses/work"));
            UNIT_ASSERT(!parser.PersonalData.Has("/v1/personality/profile/alisa/kv/guest_uid"));
        }
    };
};

namespace TShallowDatasyncResponseParserTests {

    Y_UNIT_TEST_SUITE(TShallowDatasyncResponseParserTests::WhenNoDatasyncItemsProvided) {
        Y_UNIT_TEST(ShouldThrowException) {
            TShallowDatasyncResponseParser parser;
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                    ParseDatasyncResponseWithItems(parser, {}),
                    yexception,
                    "got unexpected number of response items");
        }
    };

    Y_UNIT_TEST_SUITE(TShallowDatasyncResponseParserTests::WhenOnlyDatasyncAddressResponseProvided) {
        Y_UNIT_TEST(ShouldThrowException) {
            TShallowDatasyncResponseParser parser;
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                    ParseDatasyncResponseWithItems(parser, {ADDRESS_HTTP_RESPONSE}),
                    yexception,
                    "got unexpected number of response items");
        }
    };

    Y_UNIT_TEST_SUITE(TShallowDatasyncResponseParserTests::WhenOnlyDatasyncKeyValueResponseProvided) {
        Y_UNIT_TEST(ShouldThrowException) {
            TShallowDatasyncResponseParser parser;
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                    ParseDatasyncResponseWithItems(parser, {KEY_VALUE_HTTP_RESPONSE}),
                    yexception,
                    "got unexpected number of response items");
        }
    };

    Y_UNIT_TEST_SUITE(TShallowDatasyncResponseParserTests::WhenOnlyDatasyncSettingsResponseProvided) {
        Y_UNIT_TEST(ShouldThrowException) {
            TShallowDatasyncResponseParser parser;
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                    ParseDatasyncResponseWithItems(parser, {SETTINGS_HTTP_RESPONSE}),
                    yexception,
                    "got unexpected number of response items");
        }
    };

    Y_UNIT_TEST_SUITE(TShallowDatasyncResponseParserTests::WhenOnlyDatasyncAddressAndKeyValueResponseProvided) {
        Y_UNIT_TEST(ShouldParseDatasyncAddressAndKeyValue) {
            TShallowDatasyncResponseParser parser;
            UNIT_ASSERT_NO_EXCEPTION(ParseDatasyncResponseWithItems(parser, {
                    ADDRESS_HTTP_RESPONSE,
                    KEY_VALUE_HTTP_RESPONSE}));

            UNIT_ASSERT_STRINGS_EQUAL(parser.ParseResponse.AddressesResponse->data(), ADDRESS);
            UNIT_ASSERT_STRINGS_EQUAL(parser.ParseResponse.KeyValueResponse->data(), KEY_VALUE);
            UNIT_ASSERT(!parser.ParseResponse.SettingsResponse.Defined());
        }
    };

    Y_UNIT_TEST_SUITE(TShallowDatasyncResponseParserTests::WhenFullDatasyncResponseProvided) {
        Y_UNIT_TEST(ShouldParseAllItems) {
            TShallowDatasyncResponseParser parser;
            UNIT_ASSERT_NO_EXCEPTION(ParseDatasyncResponseWithItems(parser, {
                    ADDRESS_HTTP_RESPONSE,
                    KEY_VALUE_HTTP_RESPONSE,
                    SETTINGS_HTTP_RESPONSE
            }));

            UNIT_ASSERT_STRINGS_EQUAL(parser.ParseResponse.AddressesResponse->data(), ADDRESS);
            UNIT_ASSERT_STRINGS_EQUAL(parser.ParseResponse.KeyValueResponse->data(), KEY_VALUE);
            UNIT_ASSERT_STRINGS_EQUAL(parser.ParseResponse.SettingsResponse->data(), SETTINGS);
        }
    };

    Y_UNIT_TEST_SUITE(TShallowDatasyncResponseParserTests::WhenDatasyncResponseHasErrorStatusCode) {
        Y_UNIT_TEST(ShouldNotThrowAndNotParseEmptyResponse) {
            TShallowDatasyncResponseParser parser;
            UNIT_ASSERT_NO_EXCEPTION(ParseDatasyncResponseWithItems(parser, {}, /*status_code: */ 500));

            UNIT_ASSERT(!parser.ParseResponse.AddressesResponse.Defined());
            UNIT_ASSERT(!parser.ParseResponse.KeyValueResponse.Defined());
            UNIT_ASSERT(!parser.ParseResponse.SettingsResponse.Defined());
        }

        Y_UNIT_TEST(ShouldNotThrowAndNotParseFullResponse) {
            TShallowDatasyncResponseParser parser;
            UNIT_ASSERT_NO_EXCEPTION(ParseDatasyncResponseWithItems(parser, {
                    ADDRESS_HTTP_RESPONSE,
                    KEY_VALUE_HTTP_RESPONSE,
                    SETTINGS_HTTP_RESPONSE
            }, /*status_code: */ 500));

            UNIT_ASSERT(!parser.ParseResponse.AddressesResponse.Defined());
            UNIT_ASSERT(!parser.ParseResponse.KeyValueResponse.Defined());
            UNIT_ASSERT(!parser.ParseResponse.SettingsResponse.Defined());
        }
    };
};
