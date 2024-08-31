#include "asr_recognize.h"

#include <alice/cuttlefish/library/experiments/flags_json.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>

using namespace NAlice::NAsr;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NVoicetech::NUniproxy2;
using namespace NJson;

namespace {
    static const TString asrRecognize0 = R"(
{
  "event": {
    "header": {
    },
    "payload": {
    }
  }
}
)";
    static const TString asrRecognize1 = R"(
{
  "event": {
    "header": {
      "messageId": "test-message-id",
      "namespace": "ASR",
      "name": "Recognize",
      "streamId": 42
    },
    "payload": {
      "hostName": "test hostname",
      "lang": "en-TR test replace lang",
      "topic": "test topic",
      "application": "test app",
      "format": "test format",
      "punctuation": true,
      "spotter_phrase": "test spotter phrase",
      "experiments": "test exp",
      "advancedASROptions": {
        "manual_punctuation": true,
        "capitalize": true,
        "partial_results": true,
        "degradation_mode": "Enable",
        "allow_multi_utt": false,
        "partial_update_period": 666,
        "embedded_spotter_info": "test embedded_spotter_info"
      },
      "normalizer_options": {
      },
      "disableAntimatNormalizer": true,
      "context": [
        {
          "id": "test context id",
          "trigger": [
            "test trigger"
          ],
          "content": [
            "test content"
          ]
        }
      ],

      "enable_spotter_validation": true,
      "dirty_hacks": {
          "is_client_retry": true
      }
    }
  }
}
)";
}

class TCuttlefishConverterAsrRecognizeTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishConverterAsrRecognizeTest);
    UNIT_TEST(TestMessageToAsrInitRequestDefaults);
    UNIT_TEST(TestMessageToInitRequest);
    // UNIT_TEST(TestTryParseMessageToInitRequest);
    UNIT_TEST_SUITE_END();

public:
    void FillSessionContext(NAliceProtocol::TSessionContext& sessionContext, const TString& appId, const TString& uuid, const TString& device) {
        sessionContext.SetAppId(appId);
        sessionContext.MutableUserInfo()->SetUuid(uuid);
        sessionContext.MutableDeviceInfo()->SetDevice(device);
    }

    void TestMessageToAsrInitRequestDefaults() {
        TMessage message(TMessage::FromClient, asrRecognize0);
        NAliceProtocol::TSessionContext sessionContext;
        NAliceProtocol::TRequestContext requestContext;
        TString appId = "test app-id";
        TString uuid = "test uuid";
        TString device = "test device";
        FillSessionContext(sessionContext, appId, uuid, device);
        NProtobuf::TInitRequest initRequest;
        MessageToAsrInitRequest(message, initRequest, sessionContext, requestContext);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetAppId(), appId);
        UNIT_ASSERT(initRequest.GetHostName().size());
        UNIT_ASSERT(initRequest.GetClientHostname().size());
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetRequestId(), "");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetUuid(), uuid);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetDevice(), device);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetTopic(), "dialogeneral");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetLang(), "ru-RU");
        UNIT_ASSERT_VALUES_EQUAL(int(initRequest.GetEouMode()), int(NProtobuf::MultiUtterance));
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetMime(), "");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetHasSpotterPart(), false);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.HasExperimentsAB(), false);
        auto& ro = initRequest.GetRecognitionOptions();
        UNIT_ASSERT_VALUES_EQUAL(ro.HasSpotterPhrase(), false);
        UNIT_ASSERT_VALUES_EQUAL(ro.GetContext().size(), 0);
        UNIT_ASSERT_VALUES_EQUAL(ro.HasPunctuation(), false);
        UNIT_ASSERT_VALUES_EQUAL(ro.HasNormalization(), true);  //?!
        UNIT_ASSERT_VALUES_EQUAL(ro.HasCapitalization(), false);
        UNIT_ASSERT_VALUES_EQUAL(ro.HasAntimat(), true);
        UNIT_ASSERT_VALUES_EQUAL(ro.HasManualPunctuation(), false);
        UNIT_ASSERT_VALUES_EQUAL(ro.GetContext().size(), 0);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.HasYabioOptions(), false);
        auto& ao = initRequest.GetAdvancedOptions();
        UNIT_ASSERT_VALUES_EQUAL(ao.HasOverridePartialUpdatePeriod(), false);
        UNIT_ASSERT_VALUES_EQUAL(ao.HasDegradationMode(), false);
        UNIT_ASSERT_VALUES_EQUAL(ao.HasIsUserSessionWithRetry(), false);
    }

    void TestMessageToInitRequest() {
        TMessage message(TMessage::FromClient, asrRecognize1);
        NAliceProtocol::TSessionContext sessionContext;
        NAliceProtocol::TRequestContext requestContext;
        requestContext.MutableExpFlags()->insert({"set_topic=topic_from_uaas", "1"});
        requestContext.MutableExpFlags()->insert({"asr_enable_suggester", "1"});
        TString appId = "test app-id";
        TString uuid = "test uuid";
        TString device = "test device";
        FillSessionContext(sessionContext, appId, uuid, device);
        NVoice::NExperiments::ParseFlagsInfoFromRawResponse(
            sessionContext.MutableExperiments()->MutableFlagsJsonData()->MutableFlagsInfo(),
            R"__({
                    "all": {
                        "CONTEXT": {
                            "MAIN": {
                                "VOICE": {
                                    "flags": ["set_topic=freeform"]
                                },
                                "ASR": {
                                    "flags": ["EouDelay"]
                                }
                            }
                        }
                    }
            })__"
        );
        NProtobuf::TInitRequest initRequest;
        MessageToAsrInitRequest(message, initRequest, sessionContext, requestContext);

        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetAppId(), appId);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetHostName(), "test hostname");
        UNIT_ASSERT(initRequest.GetClientHostname().size());
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetRequestId(), "test-message-id");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetUuid(), uuid);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetDevice(), device);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetTopic(), "topic_from_uaas");  // topic MUST be replaced UAAS experiment set_topic=*
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetLang(), "tr-TR");  // legacy logic hack normalize lang from original here
        UNIT_ASSERT_VALUES_EQUAL(int(initRequest.GetEouMode()), int(NProtobuf::NoEOU));
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetMime(), "test format");
        UNIT_ASSERT(initRequest.GetHasSpotterPart());
        UNIT_ASSERT(initRequest.GetAdvancedOptions().GetEnableSuggester());
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetExperimentsAB(), "{\"ASR\":{\"flags\":[\"EouDelay\"]}}");
        {
            auto& ro = initRequest.GetRecognitionOptions();
            UNIT_ASSERT_VALUES_EQUAL(ro.GetSpotterPhrase(), "test spotter phrase");
            UNIT_ASSERT_VALUES_EQUAL(ro.GetContext().size(), 1);
            {
                auto& context0 = ro.GetContext()[0];
                UNIT_ASSERT_VALUES_EQUAL(context0.GetId(), "test context id");
                UNIT_ASSERT_VALUES_EQUAL(context0.GetTrigger().size(), 1);
                {
                    auto& trigger0 = context0.GetTrigger()[0];
                    UNIT_ASSERT_VALUES_EQUAL(trigger0, "test trigger");
                }
                UNIT_ASSERT_VALUES_EQUAL(context0.GetContent().size(), 1);
                {
                    auto& content0 = context0.GetContent()[0];
                    UNIT_ASSERT_VALUES_EQUAL(content0, "test content");
                }
            }
            UNIT_ASSERT_VALUES_EQUAL(ro.GetPunctuation(), true);
            UNIT_ASSERT_VALUES_EQUAL(ro.GetNormalization(), true);
            UNIT_ASSERT_VALUES_EQUAL(ro.GetCapitalization(), true);
            UNIT_ASSERT_VALUES_EQUAL(ro.GetAntimat(), false);
            UNIT_ASSERT_VALUES_EQUAL(ro.GetManualPunctuation(), true);
            UNIT_ASSERT_VALUES_EQUAL(ro.GetEmbeddedSpotterInfo(), "test embedded_spotter_info");
        }
        //TODO: check YabioOptions
        auto& ao = initRequest.GetAdvancedOptions();
        UNIT_ASSERT_VALUES_EQUAL(ao.GetOverridePartialUpdatePeriod(), 666);
        UNIT_ASSERT_VALUES_EQUAL(int(ao.GetDegradationMode()), int(NProtobuf::DegradationModeEnable));
        UNIT_ASSERT_VALUES_EQUAL(ao.GetIsUserSessionWithRetry(), true);
    }

    void TestTryParseMessageToInitRequest() {
        TFileInput f("asr_init_message.json");
        TString msg = f.ReadAll();
        TMessage message(TMessage::FromClient, msg);
        NAliceProtocol::TSessionContext sessionContext;
        NAliceProtocol::TRequestContext requestContext;
        TString appId = "test app-id";
        TString uuid = "test uuid";
        TString device = "test device";
        FillSessionContext(sessionContext, appId, uuid, device);
        NProtobuf::TInitRequest initRequest;
        MessageToAsrInitRequest(message, initRequest, sessionContext, requestContext);
    }

};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishConverterAsrRecognizeTest)
