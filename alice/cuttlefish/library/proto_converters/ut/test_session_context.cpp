#include "common.h"
#include <alice/cuttlefish/library/proto_converters/converters.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <google/protobuf/text_format.h>

using namespace NAlice::NCuttlefish;
using namespace NAliceProtocol;


Y_UNIT_TEST_SUITE(ConvertSessionContext) {

Y_UNIT_TEST(Basic)
{
    NAliceProtocol::TSessionContext ctx;
    ctx.SetSessionId("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee");
    ctx.SetAppToken("11111111-2222-3333-4444-555555555555");
    ctx.SetAppId("some.app.yandex.ru");
    ctx.SetAppType("aliced");
    ctx.MutableDeviceInfo()->SetDeviceId("my-unique-device-id");
    ctx.MutableUserInfo()->SetUuid("1234567890abcdef1234567890abcdef");
    ctx.MutableUserInfo()->SetAuthToken("THE-BEST-AUTH-TOKEN-EVER");
    ctx.MutableUserInfo()->SetAuthTokenType(NAliceProtocol::TUserInfo::OAUTH_TEAM);
    ctx.MutableUserInfo()->SetLaasRegion(R"__({"region_id": 104357, "latitude": 60.166892, "longitude": 24.943592})__");
    ctx.MutableUserInfo()->SetPuid("777");
    ctx.MutableUserInfo()->SetYuid("888");

    const TString expected = AsSortedJsonString(R"__({
        "AppToken": "11111111-2222-3333-4444-555555555555",
        "Application": {
            "Id": "some.app.yandex.ru",
            "Type": "aliced"
        },
        "Device": {
            "Id": "my-unique-device-id"
        },
        "Experiments": {
            "FlagsJson": { }
        },
        "SessionId": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee",
        "User": {
            "AuthToken": "OAuthTeam THE-BEST-AUTH-TOKEN-EVER",
            "Uuid": "1234567890abcdef1234567890abcdef",
            "Puid": "777",
            "Yuid": "888",
            "LaasRegion": "{\"region_id\": 104357, \"latitude\": 60.166892, \"longitude\": 24.943592}"
        },
        "UserOptions": {
            "DoNotUseLogs": false
        }
    })__");

    {
        const TString actual = AsSortedJsonString(ProtobufToJson(SessionContextConverter(), ctx));
        Cerr << "  EXPECTED: " << expected << "\n"
                "  ACTUAL:   " << actual << Endl;
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    } {
        const NJson::TJsonValue jsonValue = ProtobufToJsonValue(SessionContextConverter(), ctx);
        UNIT_ASSERT_VALUES_EQUAL(AsSortedJsonString(jsonValue), expected);
    }
}

Y_UNIT_TEST(Empty)
{
    NAliceProtocol::TSessionContext ctx;

    const TString expected = AsSortedJsonString(R"__({
        "Application": {},
        "Device": {},
        "Experiments": {
            "FlagsJson": {}
        },
        "User": {},
        "UserOptions": {
            "DoNotUseLogs": false
        }
    })__");

    {
        const TString actual = AsSortedJsonString(ProtobufToJson(SessionContextConverter(), ctx));
        Cerr << "  EXPECTED: " << expected << "\n"
                "  ACTUAL:   " << actual << Endl;
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    } {
        const NJson::TJsonValue jsonValue = ProtobufToJsonValue(SessionContextConverter(), ctx);
        UNIT_ASSERT_VALUES_EQUAL(AsSortedJsonString(jsonValue), expected);
    }
}

Y_UNIT_TEST(Real)
{
    const TString sessionContextAsText = R"__(
        AppToken: "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
        UserInfo {
            Uuid: "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            Yuid: "123456789"
            AuthToken: "AgA************************************"
            StaffLogin: "some-staff-login"
            AuthTokenType: OAUTH
            Puid: "123456789"
        }
        Experiments {
            FlagsJsonData {
                Data: "raw flags.json response as json here"
                AppInfo: "another dumped json here"
            }
        }
        AppId: "aliced"
        InitialMessageId: "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb"
    )__";

    NAliceProtocol::TSessionContext ctx;
    google::protobuf::TextFormat::ParseFromString(sessionContextAsText, &ctx);


    const TString expected = AsSortedJsonString(R"__({
        "AppToken":"aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
        "Application": {
            "Id": "aliced"
        },
        "Device":{},
        "Experiments": {
            "FlagsJson": {
                "Data": "raw flags.json response as json here",
                "AppInfo": "another dumped json here"
            }
        },
        "InitialMessageId": "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb",
        "User": {
            "AuthToken": "OAuth AgA************************************",
            "Puid": "123456789",
            "StaffLogin": "some-staff-login",
            "Uuid": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            "Yuid": "123456789"
        },
        "UserOptions": {
            "DoNotUseLogs": false
        }
    })__");

    {
        const TString actual = AsSortedJsonString(ProtobufToJson(SessionContextConverter(), ctx));
        Cerr << "  EXPECTED: " << expected << "\n"
                "  ACTUAL:   " << actual << Endl;
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    } {
        const NJson::TJsonValue jsonValue = ProtobufToJsonValue(SessionContextConverter(), ctx);
        UNIT_ASSERT_VALUES_EQUAL(AsSortedJsonString(jsonValue), expected);
    }
}

}

