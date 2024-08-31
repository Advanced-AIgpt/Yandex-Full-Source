#include "tts_generate.h"

#include <alice/megamind/protos/speechkit/response.pb.h>
#include <voicetech/library/settings_manager/proto/settings.pb.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NTts;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NVoicetech::NUniproxy2;
using namespace NJson;

namespace {
    static const TString ttsGenerateEmpty = R"(
{
  "event": {
    "header": {
    },
    "payload": {
      "text": "",
      "voice": "valtz"
    }
  }
}
)";
    static const TString ttsGenerateAllFields = R"(
{
  "event": {
    "header": {
      "messageId": "test-message-id",
      "namespace": "TTS",
      "name": "Generate"
    },
    "payload": {
      "lang": "en-EN",
      "text": "test text",
      "voice": "shitova.gpu",
      "speed": 0.888,
      "format": "pCm",
      "quality": "UltraHigh",
      "chunked": false,
      "volume": 5.55,
      "effect": "test effect",
      "do_not_log": true,
      "need_timings": true
    }
  }
}
)";

}

class TCuttlefishTtsGenerateTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishTtsGenerateTest);
    UNIT_TEST(TestTtsGenerateMessageToTtsRequestDefaults);
    UNIT_TEST(TestTtsGenerateMessageToTtsRequest);
    UNIT_TEST(TestTtsGenerateMessageToTtsRequestDisableCache);
    UNIT_TEST(TestTtsGenerateMessageToTtsRequestDisableTtsTimings);
    UNIT_TEST(TestTtsGenerateMessageToTtsRequestEnableTtsTimings);
    UNIT_TEST(TestCreateTtsGenerate);
    UNIT_TEST(TestCreateTtsGenerateDoNotLog);
    UNIT_TEST_SUITE_END();

public:
    void TestTtsGenerateMessageToTtsRequestDefaults() {
        TMessage message(TMessage::FromClient, ttsGenerateEmpty);
        NAliceProtocol::TRequestContext requestContext;
        NAliceProtocol::TSessionContext sessionContext;

        NProtobuf::TRequest ttsRequest;
        TtsGenerateMessageToTtsRequest(ttsRequest, requestContext, sessionContext, message);

        UNIT_ASSERT_VALUES_EQUAL(ttsRequest.GetText(), "");
        UNIT_ASSERT_VALUES_EQUAL(ttsRequest.GetPartialNumber(), 0);
        UNIT_ASSERT_VALUES_EQUAL(ttsRequest.GetRequestId(), "");
        UNIT_ASSERT(!ttsRequest.GetReplaceShitovaWithShitovaGpu());
        UNIT_ASSERT(!ttsRequest.GetNeedTtsBackendTimings());
        UNIT_ASSERT(ttsRequest.GetEnableTtsBackendTimings());
        UNIT_ASSERT(ttsRequest.GetEnableGetFromCache());
        UNIT_ASSERT(ttsRequest.GetEnableCacheWarmUp());
        UNIT_ASSERT(ttsRequest.GetEnableSaveToCache());
        UNIT_ASSERT(!ttsRequest.GetDoNotLogTexts());
    }

    void TestTtsGenerateMessageToTtsRequest() {
        TMessage message(TMessage::FromClient, ttsGenerateAllFields);
        NAliceProtocol::TRequestContext requestContext;
        {
            auto& expFlags = *requestContext.MutableExpFlags();
            expFlags["enable_tts_gpu"] = "1";
            expFlags["enable_tts_timings"] = "1";
        }
        {
            auto& settingsManager = *requestContext.MutableSettingsFromManager();
            settingsManager.SetTtsEnableCache(true);
        }

        NAliceProtocol::TSessionContext sessionContext;
        NProtobuf::TRequest ttsRequest;
        TtsGenerateMessageToTtsRequest(ttsRequest, requestContext, sessionContext, message);

        UNIT_ASSERT_VALUES_EQUAL(ttsRequest.GetText(), "test text");
        UNIT_ASSERT_VALUES_EQUAL(ttsRequest.GetPartialNumber(), 0);
        UNIT_ASSERT_VALUES_EQUAL(ttsRequest.GetRequestId(), "test-message-id");
        UNIT_ASSERT(ttsRequest.GetReplaceShitovaWithShitovaGpu());
        UNIT_ASSERT(ttsRequest.GetNeedTtsBackendTimings());
        UNIT_ASSERT(ttsRequest.GetEnableTtsBackendTimings());
        UNIT_ASSERT(ttsRequest.GetEnableGetFromCache());
        UNIT_ASSERT(ttsRequest.GetEnableCacheWarmUp());
        UNIT_ASSERT(ttsRequest.GetEnableSaveToCache());
        UNIT_ASSERT(ttsRequest.GetDoNotLogTexts());
    }

    void TestTtsGenerateMessageToTtsRequestDisableCache() {
        {
            // Disable by settings manager
            TMessage message(TMessage::FromClient, ttsGenerateAllFields);
            NAliceProtocol::TRequestContext requestContext;
            {
                auto& settingsManager = *requestContext.MutableSettingsFromManager();
                settingsManager.SetTtsEnableCache(false);
            }

            NAliceProtocol::TSessionContext sessionContext;
            NProtobuf::TRequest ttsRequest;
            TtsGenerateMessageToTtsRequest(ttsRequest, requestContext, sessionContext, message);

            UNIT_ASSERT(!ttsRequest.GetEnableGetFromCache());
            UNIT_ASSERT(!ttsRequest.GetEnableCacheWarmUp());
            UNIT_ASSERT(!ttsRequest.GetEnableSaveToCache());
        }
    }

    void TestTtsGenerateMessageToTtsRequestDisableTtsTimings() {
        TMessage message(TMessage::FromClient, ttsGenerateAllFields);
        NAliceProtocol::TRequestContext requestContext;
        {
            auto& expFlags = *requestContext.MutableExpFlags();
            expFlags["enable_tts_timings"] = "1";
            expFlags["disable_tts_timings"] = "1";
        }

        NAliceProtocol::TSessionContext sessionContext;
        NProtobuf::TRequest ttsRequest;
        TtsGenerateMessageToTtsRequest(ttsRequest, requestContext, sessionContext, message);

        UNIT_ASSERT(!ttsRequest.GetNeedTtsBackendTimings());
        UNIT_ASSERT(!ttsRequest.GetEnableTtsBackendTimings());
    }

    void TestTtsGenerateMessageToTtsRequestEnableTtsTimings() {
        TMessage message(TMessage::FromClient, ttsGenerateAllFields);
        NAliceProtocol::TRequestContext requestContext;

        NAliceProtocol::TSessionContext sessionContext;
        {
            auto& deviceInfo = *sessionContext.MutableDeviceInfo();
            deviceInfo.AddSupportedFeatures("enable_tts_timings");
        }
        NProtobuf::TRequest ttsRequest;
        TtsGenerateMessageToTtsRequest(ttsRequest, requestContext, sessionContext, message);

        UNIT_ASSERT(ttsRequest.GetNeedTtsBackendTimings());
        UNIT_ASSERT(ttsRequest.GetEnableTtsBackendTimings());
    }

    void TestCreateTtsGenerate() {
        constexpr double EPS = 1e-9;
        NAliceProtocol::TRequestContext requestContext;
        {
            requestContext.MutableHeader()->SetRefStreamId(228);
            requestContext.MutableAudioOptions()->SetFormat("audio/opus");
            {
                auto& voiceOptions = *requestContext.MutableVoiceOptions();
                voiceOptions.SetLang("ru");
                voiceOptions.SetVoice("oksana");
                voiceOptions.SetQuality(NAliceProtocol::TVoiceOptions::HIGH);
                voiceOptions.SetVolume(1.5);
                voiceOptions.SetSpeed(1.8);
            }
        }
        NAliceProtocol::TMegamindResponse mmResponse;
        {
            mmResponse.MutableProtoResponse()->MutableVoiceResponse()->MutableOutputSpeech()->SetText("some text");
            mmResponse.MutableProtoResponse()->SetContainsSensitiveData(true);
        }

        auto ttsGenerate = CreateTtsGenerate(
            requestContext,
            mmResponse,
            "message_id"
        );

        {
            const auto& header = ttsGenerate.Header;
            UNIT_ASSERT(header);
            UNIT_ASSERT_VALUES_EQUAL(header->Namespace, "TTS");
            UNIT_ASSERT_VALUES_EQUAL(header->Name, "Generate");
            UNIT_ASSERT_VALUES_EQUAL(header->MessageId, "message_id");
            UNIT_ASSERT_VALUES_EQUAL(header->RefStreamId, 228);
        }

        {
            const auto& payload = ttsGenerate.Json.GetValueByPath("event.payload");
            UNIT_ASSERT(payload);
            const auto& payloadMap = payload->GetMap();
            UNIT_ASSERT_VALUES_EQUAL(payloadMap.at("format").GetString(), "audio/opus");
            UNIT_ASSERT_VALUES_EQUAL(payloadMap.at("lang").GetString(), "ru");
            UNIT_ASSERT_VALUES_EQUAL(payloadMap.at("voice").GetString(), "oksana");
            UNIT_ASSERT_VALUES_EQUAL(payloadMap.at("quality").GetString(), "High");
            UNIT_ASSERT_DOUBLES_EQUAL(payloadMap.at("volume").GetDouble(), 1.5, EPS);
            UNIT_ASSERT_DOUBLES_EQUAL(payloadMap.at("speed").GetDouble(), 1.8, EPS);
            UNIT_ASSERT_VALUES_EQUAL(payloadMap.at("text").GetString(), "some text");
            UNIT_ASSERT_VALUES_EQUAL(payloadMap.at("do_not_log").GetBoolean(), true);
        }
    }

    void TestCreateTtsGenerateDoNotLog() {
        NAliceProtocol::TRequestContext requestContext;
        {
            requestContext.MutableHeader()->SetRefStreamId(228);
        }
        NAliceProtocol::TMegamindResponse mmResponse;
        {
            mmResponse.MutableProtoResponse()->MutableVoiceResponse()->MutableOutputSpeech()->SetText("some text");
        }

        for (ui32 containsSensitiveData = 0; containsSensitiveData < 2; ++containsSensitiveData) {
            for (ui32 mmBlackSheepWallExp = 0; mmBlackSheepWallExp < 2; ++mmBlackSheepWallExp) {
                mmResponse.MutableProtoResponse()->SetContainsSensitiveData((bool)containsSensitiveData);
                (*requestContext.MutableExpFlags())["mm_black_sheep_wall"] = ToString((bool)mmBlackSheepWallExp);

                auto ttsGenerate = CreateTtsGenerate(
                    requestContext,
                    mmResponse,
                    "message_id"
                );

                if ((bool)containsSensitiveData && !(bool)mmBlackSheepWallExp) {
                    UNIT_ASSERT_VALUES_EQUAL(ttsGenerate.Json["event"]["payload"]["do_not_log"].GetBoolean(), true);
                } else {
                    UNIT_ASSERT(!ttsGenerate.Json["event"]["payload"].Has("do_not_log"));
                }
            }
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishTtsGenerateTest)
