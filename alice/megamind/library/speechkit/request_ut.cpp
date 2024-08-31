#include "request.h"

#include <alice/library/json/json.h>
#include <alice/library/unittest/fake_fetcher.h>
#include <alice/megamind/library/testing/speechkit.h>

#include <alice/library/restriction_level/protos/content_settings.pb.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/join.h>

using namespace NAlice;
using namespace NAlice::NMegamind;

namespace {

constexpr TStringBuf MEGAREQUEST = TStringBuf(R"(
{
    "application" : {
      "app_id" : "ru.yandex.searchplugin.dev",
      "app_version" : "8.20",
      "client_time" : "20190527T213049",
      "device_id" : "ne-skagu",
      "device_manufacturer" : "Xiaomi",
      "device_model" : "Redmi Note 4",
      "lang" : "ru-RU",
      "os_version" : "7.1.2",
      "platform" : "android",
      "timestamp" : "1558981849",
      "timezone" : "Europe/Moscow",
      "uuid" : "hren-vam!"
   },
   "header" : {
      "request_id" : "5580a634-9b2e-4e71-b14f-70e01f9b7b1f",
      "ref_message_id" : "5580a634-9b2e-4e71-b14f-70e01f9b7b1g",
      "session_id" : "5580a634-9b2e-4e71-b14f-70e01f9b7b1h"
   },
   "request" : {
      "additional_options": {
          "bass_options" : {
              "event" : {
                  "name" : "on_get_greetings",
                  "type" : "server_action"
               }
          },
          "supported_features": [
              "change_alarm_sound"
          ],
          "unsupported_features": [
              "change_alarm_sound_level"
          ],
          "icookie": "2253073970645310284",
          "expboxes": "258950,0,58;258658,0,47;258133,0,74;256153,0,19;239309,0,97;233471,0,22;255325,0,65;257025,0,5;255198,0,1;206399,0,89;253001,0,57"
      },
      "device_state" : {
          "is_default_assistant" : true,
          "multiroom": {
              "master_device_id" : "09108313c8135c28090d",
              "multiroom_token" : "0123456789abcdef",
              "visible_peers": [
                "other_device_id_1",
                "other_device_id_2",
                "other_device_id_3"
              ],
              "room_device_ids": [
                "other_device_id_1",
                "other_device_id_3"
              ]
          }
      },
      "activation_type": "spotter",
      "event" : {
         "name" : "on_get_greetings",
         "type" : "server_action"
      },
      "experiments" : {},
      "reset_session" : false,
      "megamind_cookies": "{\"uaas_tests\":[247071]}",
      "voice_session" : true
   }
})");

class TSpeechKitTestItem {
public:
    TSpeechKitTestItem()
        : Json{NJson::ReadJsonFastTree(MEGAREQUEST)}
        , SKR{TSpeechKitRequestBuilder{MEGAREQUEST}.AddHeader({"h1", "v1"}).AddHeader({"x-super", "value-x-super"}).SetPath("/vins_path/app/pa").Build()}
    {
    }

    void CommonAsserts(const TString& headersStr) {
        TStringStream headersStream{headersStr};
        const THttpHeaders headers{&headersStream};
        const auto& reqHeaders = Request.Headers;
        for (const auto& h : headers) {
            const auto* testHeader = reqHeaders.FindHeader(h.Name());
            UNIT_ASSERT_C(testHeader, TStringBuilder{} << "Header not found: " << h.Name());
            UNIT_ASSERT_VALUES_EQUAL_C(testHeader->Value(), h.Value(), TStringBuilder{} << "Value for header '" << h.Name() << "' is not the same: '" << h.Value() << "' != '" << testHeader->Value() << "'");
        }
        UNIT_ASSERT(Request.Method);
        UNIT_ASSERT_VALUES_EQUAL(*Request.Method, NHttpMethods::POST);
        UNIT_ASSERT(Request.Body);
        UNIT_ASSERT_VALUES_EQUAL(JsonToString(NJson::ReadJsonFastTree(*Request.Body)), JsonToString(Json));
    }

    NJson::TJsonValue Json;
    TTestSpeechKitRequest SKR;
    NTestingHelpers::TFakeRequestBuilder Request;
};

Y_UNIT_TEST_SUITE(SpeechKitRequest) {
    Y_UNIT_TEST(SpeechkitRequest) {
        TSpeechKitTestItem testItem;
        UNIT_ASSERT_VALUES_EQUAL("5580a634-9b2e-4e71-b14f-70e01f9b7b1g", testItem.SKR->GetHeader().GetRefMessageId());
        UNIT_ASSERT_VALUES_EQUAL("5580a634-9b2e-4e71-b14f-70e01f9b7b1h", testItem.SKR->GetHeader().GetSessionId());
        UNIT_ASSERT_VALUES_EQUAL("2253073970645310284",
                                 testItem.SKR->GetRequest().GetAdditionalOptions().GetICookie());
        UNIT_ASSERT_VALUES_EQUAL("258950,0,58;258658,0,47;258133,0,74;256153,0,19;239309,0,97;233471,0,22;255325,0,65;"
                                 "257025,0,5;255198,0,1;206399,0,89;253001,0,57",
                                 testItem.SKR->GetRequest().GetAdditionalOptions().GetExpboxes());
        UNIT_ASSERT(testItem.SKR->GetRequest().GetDeviceState().GetIsDefaultAssistant());
        UNIT_ASSERT_VALUES_EQUAL("09108313c8135c28090d", testItem.SKR->GetRequest().GetDeviceState().GetMultiroom().GetMasterDeviceId());
        UNIT_ASSERT_VALUES_EQUAL("0123456789abcdef", testItem.SKR->GetRequest().GetDeviceState().GetMultiroom().GetMultiroomToken());

        const auto& visiblePeers = testItem.SKR->GetRequest().GetDeviceState().GetMultiroom().GetVisiblePeerDeviceIds();
        UNIT_ASSERT_VALUES_EQUAL("other_device_id_1", visiblePeers[0]);
        UNIT_ASSERT_VALUES_EQUAL("other_device_id_2", visiblePeers[1]);
        UNIT_ASSERT_VALUES_EQUAL("other_device_id_3", visiblePeers[2]);

        const auto& roomDeviceIds = testItem.SKR->GetRequest().GetDeviceState().GetMultiroom().GetRoomDeviceIds();
        UNIT_ASSERT_VALUES_EQUAL("other_device_id_1", roomDeviceIds[0]);
        UNIT_ASSERT_VALUES_EQUAL("other_device_id_3", roomDeviceIds[1]);

        UNIT_ASSERT_VALUES_EQUAL("spotter", testItem.SKR->GetRequest().GetActivationType());
        UNIT_ASSERT_VALUES_EQUAL("{\"uaas_tests\":[247071]}", testItem.SKR->GetRequest().GetMegamindCookies());
    }

    Y_UNIT_TEST(InvalidChildContentSettings) {
        auto json = NJson::ReadJsonFastTree(MEGAREQUEST);
        json["request"]["device_state"]["device_config"]["child_content_settings"] = TStringBuf{};

        const auto& request = TSpeechKitRequestBuilder{json}.Build();

        const auto& actual = EContentSettings_Name(
            request.Proto().GetRequest().GetDeviceState().GetDeviceConfig().GetChildContentSettings());
        const auto& expected = EContentSettings_Name(EContentSettings::children);

        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(ValidChildContentSettings) {
        auto json = NJson::ReadJsonFastTree(MEGAREQUEST);
        json["request"]["device_state"]["device_config"]["child_content_settings"] = EContentSettings_Name(EContentSettings::medium);

        const auto& request = TSpeechKitRequestBuilder{json}.Build();

        const auto& actual = EContentSettings_Name(
                request.Proto().GetRequest().GetDeviceState().GetDeviceConfig().GetChildContentSettings());
        const auto& expected = EContentSettings_Name(EContentSettings::medium);

        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(RandomSeedFromHeader) {
        constexpr ui64 randomSeed = 33;

        auto json = NJson::ReadJsonFastTree(MEGAREQUEST);
        UNIT_ASSERT_VALUES_UNEQUAL(TSpeechKitRequestBuilder{json}.Build().GetSeed(), randomSeed);

        json["header"]["random_seed"] = randomSeed;
        UNIT_ASSERT_VALUES_EQUAL(TSpeechKitRequestBuilder{json}.Build().GetSeed(), randomSeed);
    }
}

} // namespace
