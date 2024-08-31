#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/context.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/guest/guest_data.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <library/cpp/testing/gtest_protobuf/matcher.h>
#include "library/cpp/json/json_reader.h"
#include "library/cpp/json/json_writer.h"
#include "library/cpp/protobuf/json/string_transform.h"
#include <library/cpp/testing/gtest/gtest.h>
#include <util/string/builder.h>
#include <variant>

namespace {

    using namespace NAlice::NCuttlefish::NAppHostServices;

    const TString PERS_ID = "PersId-123456";
    const TString OAUTH_TIOKEN = "OAuth-AAVDF****";
    const TString YANDEX_UID = "YandexUid-123";
    const NAlice::TGuestOptions::EGuestOrigin GUEST_ORIGIN = NAlice::TGuestOptions::VoiceBiometry;

    TString Obfuscate(TString source) {
        NJson::TJsonValue value;
        NJson::ReadJsonTree(source, &value, /*throwOnError =*/ true);

        // return value;
        TString obfuscated = NJson::WriteJson(value, false);

        NProtobufJson::TDoubleUnescapeTransform().Transform(obfuscated);

        return obfuscated;
    }

    NAliceProtocol::TMatchVoiceprintResult MakeMatchVoiceprintResult() {
        NAliceProtocol::TMatchVoiceprintResult matchVoiceprintResult;

        matchVoiceprintResult.MutableGuestOptions()->SetPersId(PERS_ID);
        matchVoiceprintResult.MutableGuestOptions()->SetOAuthToken(OAUTH_TIOKEN);
        matchVoiceprintResult.MutableGuestOptions()->SetYandexUID(YANDEX_UID);
        matchVoiceprintResult.MutableGuestOptions()->SetStatus(::NAlice::TGuestOptions::Match);
        matchVoiceprintResult.MutableGuestOptions()->SetGuestOrigin(GUEST_ORIGIN);

        return matchVoiceprintResult;
    }

    NAppHostHttp::THttpResponse MakeBlackboxResponse() {
        NAppHostHttp::THttpResponse response;
        response.SetContent(TStringBuilder() << R"(
        {
           "dbfields" : {
              "userinfo.lastname.uid" : "lastname",
              "userinfo.firstname.uid" : "firstname"
           },
           "uid" : {
              "value" : "uid_123"
           },
           "user_ticket" : "mock_user_ticket",
           "phones" : [
              {
                 "id" : "111",
                 "attributes" : {
                    "108" : "1",
                    "102" : "main_phone",
                    "107" : "default_phone"
                 }
              }
           ],
           "attributes" : {
              "1015" : "1",
              "8" : "1"
           },
           "address-list" : [
              {
                 "address" : "root@yandex.ru"
              }
           ],
           "aliases" : {
              "13" : "root"
           },
           "billing_features" : {
              "basic-kinopoisk" : {
                 "in_trial": true,
                 "region": 225
              },
              "basic-plus" : {
                 "in_trial": true,
                 "region": 225
              },
              "basic-music" : {
                  "in_trial": true,
                  "region_id": 225
              }
           }
        })");

        return response;
    }

    NAppHostHttp::THttpResponse MakeDatasyncResponse() {
        static const TString address = R"({
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

        static const TString keyValye = R"({
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

        static const TString settings = R"({
            "items": [{
                "do_not_use_user_logs": true
            }]
        })";

        static const TString headers = R"({
            "Access-Control-Allow-Methods": "PUT, POST, GET, DELETE, OPTIONS",
            "Access-Control-Allow-Credentials": "true",
            "Yandex-Cloud-Request-ID": "rest-777b797b5cc6a976422050b75d3ec6fe-api01e",
            "Cache-Control": "no-cache",
            "Access-Control-Allow-Origin": "*",
            "Access-Control-Allow-Headers": "Accept-Language, Accept, X-Uid, X-HTTP-Method, X-Requested-With, Content-Type, Authorization",
            "Content-Type": "application/json; charset=utf-8"
        })";

        static auto makeHttpResponse = [](TString body) {
            NProtobufJson::TCEscapeTransform().Transform(body);
            return TStringBuilder() << R"({"body": ")" << body << R"(", "headers": )" << headers << "}";
        };

        NAppHostHttp::THttpResponse response;
        response.SetStatusCode(200);
        response.SetContent(TStringBuilder()
            << "{\"items\": ["
                << makeHttpResponse(address) << ','
                << makeHttpResponse(keyValye) << ','
                << makeHttpResponse(settings)
            << "]}"
        );

        return response;
    }

    class TSpeakerContextTest : public ::testing::Test {
    public:
        TSpeakerContext Sut;
    };

    TEST_F(TSpeakerContextTest, UpdateFromMatchVoiceprintResult) {
        NAlice::TGuestOptions expectedGuestOptions;
        expectedGuestOptions.SetPersId(PERS_ID);
        expectedGuestOptions.SetOAuthToken(OAUTH_TIOKEN);
        expectedGuestOptions.SetYandexUID(YANDEX_UID);
        expectedGuestOptions.SetStatus(::NAlice::TGuestOptions::Match);
        expectedGuestOptions.SetGuestOrigin(GUEST_ORIGIN);

        EnrichFromMatchResult(Sut, MakeMatchVoiceprintResult());

        EXPECT_THAT(Sut.GuestUserOptions, NGTest::EqualsProto(expectedGuestOptions));
    }

    TEST_F(TSpeakerContextTest, EnrichFromBlackboxResponse) {
        NAlice::TGuestData expectedGuestData;

        expectedGuestData.MutableUserInfo()->SetUid("uid_123");
        expectedGuestData.MutableUserInfo()->SetEmail("root@yandex.ru");
        expectedGuestData.MutableUserInfo()->SetFirstName("firstname");
        expectedGuestData.MutableUserInfo()->SetLastName("lastname");
        expectedGuestData.MutableUserInfo()->SetPhone("main_phone");
        expectedGuestData.MutableUserInfo()->SetHasYandexPlus(true);
        expectedGuestData.MutableUserInfo()->SetIsStaff(true);
        expectedGuestData.MutableUserInfo()->SetIsBetaTester(true);
        expectedGuestData.MutableUserInfo()->SetHasMusicSubscription(true);
        expectedGuestData.MutableUserInfo()->SetMusicSubscriptionRegionId(225);

        EnrichFromBlackboxResponse(Sut, MakeBlackboxResponse());

        EXPECT_THAT(Sut.GuestUserData, NGTest::EqualsProto(expectedGuestData));
    }

    TEST_F(TSpeakerContextTest, EnrichFromDatasyncResponse) {
        const TString rawPersonalData = R"({
            "/v1/personality/profile/alisa/kv/AutomotivePromoCounters": {
                "auto_music_promo_2020": 3
            },
            "/v1/personality/profile/alisa/kv/alice_proactivity": "enabled",
            "/v1/personality/profile/alisa/kv/gender": "male",
            "/v1/personality/profile/alisa/kv/guest_uid": "1234567",
            "/v1/personality/profile/alisa/kv/morning_show": {
                "last_push_timestamp": 1633374073,
                "pushes_sent": 2
            },
            "/v1/personality/profile/alisa/kv/proactivity_history": {},
            "/v1/personality/profile/alisa/kv/user_name": "Swarley",
            "/v1/personality/profile/alisa/kv/video_rater_proactivity_history": {
                "LastShowTime": "1593005794"
            },
            "/v1/personality/profile/alisa/kv/yandexstation_123456_location": {
                "location": "Moscow"
            },
            "/v2/personality/profile/addresses/home": {
                "address_id": "home",
                "address_line": "London, Baker street, 221b",
                "address_line_short": "Baker street, 221b",
                "created": "2016-04-07T23:22:48.010000+00:00",
                "last_used": "2016-04-07T23:22:48.010000+00:00",
                "latitude": 23.456,
                "longitude": 12.345,
                "mined_attributes": [],
                "modified": "2016-04-07T23:22:48.010000+00:00",
                "tags": [],
                "title": "Home"
            },
            "/v2/personality/profile/addresses/work": {
                "address_id": "work",
                "address_line": "Moscow, Tolstogo street, 16",
                "address_line_short": "Tolstogo street, 16",
                "created": "2016-04-07T23:22:48.011000+00:00",
                "last_used": "2016-04-07T23:22:48.011000+00:00",
                "latitude": 45.678,
                "longitude": 34.567,
                "mined_attributes": [],
                "modified": "2016-04-07T23:22:48.011000+00:00",
                "tags": [],
                "title": "Work"
            }
        })";

        NAlice::TGuestData expectedGuestData;
        expectedGuestData.SetRawPersonalData(rawPersonalData);

        EnrichFromDatasyncResponse(Sut, MakeDatasyncResponse());

        EXPECT_THAT(Obfuscate(Sut.GuestUserData.GetRawPersonalData()), Obfuscate(rawPersonalData));
    }
}