#include <library/cpp/testing/unittest/registar.h>

#include "biometry.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <util/stream/file.h>

using namespace NAlice::NYabio;
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
      "namespace": "Biometry",
      "name": "Classify"
    },
    "payload": {
      "format": "test format",
      "biometry_group": "uniproxy_test",
      "biometry_classify": "test-gender,test-children",
      "hostName": "test hostname",
      "application": "test app",
      "format": "test format",
      "advancedASROptions": {
        "spotter_validation": true
      },
      "TODO_enable_spotter_validation": true
    }
  }
}
)";
}

class TCuttlefishConverterBiometryRequestTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishConverterBiometryRequestTest);
    UNIT_TEST(TestMessageToYabioInitRequestClassifyDefaults);
    UNIT_TEST(TestMessageToYabioInitRequestClassify);
    UNIT_TEST_SUITE_END();

public:
    void FillSessionContext(NAliceProtocol::TSessionContext& sessionContext, const TString& sessionId, const TString& uuid, const TString& device) {
        sessionContext.SetSessionId(sessionId);
        sessionContext.MutableUserInfo()->SetUuid(uuid);
        sessionContext.MutableDeviceInfo()->SetDevice(device);
    }

    void TestMessageToYabioInitRequestClassifyDefaults() {
        TMessage message(TMessage::FromClient, asrRecognize0);
        NAliceProtocol::TSessionContext sessionContext;
        NAliceProtocol::TRequestContext requestContext;
        TString sessionId = "test session-id";
        TString uuid = "test uuid";
        TString device = "test device";
        FillSessionContext(sessionContext, sessionId, uuid, device);
        NProtobuf::TInitRequest initRequest;
        MessageToYabioInitRequestClassify(message, initRequest, sessionContext, requestContext);
        UNIT_ASSERT(initRequest.GethostName().size());
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetsessionId(), sessionId);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.uuid(), uuid);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.mime(), "");
        UNIT_ASSERT_VALUES_EQUAL(int(initRequest.method()), int(YabioProtobuf::Method::Classify));
        UNIT_ASSERT_VALUES_EQUAL(initRequest.group_id(), "");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.model_id(), "");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.user_id(), "");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.requests_ids().size(), 0);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.classification_tags().size(), 0);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.spotter(), false);
        UNIT_ASSERT(initRequest.GetclientHostname().size());
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetMessageId(), "");
    }

    void TestMessageToYabioInitRequestClassify() {
        TMessage message(TMessage::FromClient, asrRecognize1);
        NAliceProtocol::TSessionContext sessionContext;
        NAliceProtocol::TRequestContext requestContext;
        TString sessionId = "test session-id";
        TString uuid = "test uuid";
        TString device = "test device";
        FillSessionContext(sessionContext, sessionId, uuid, device);
        NProtobuf::TInitRequest initRequest;
        MessageToYabioInitRequestClassify(message, initRequest, sessionContext, requestContext);
        UNIT_ASSERT(initRequest.GethostName().size());
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetsessionId(), sessionId);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.uuid(), uuid);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.mime(), "test format");
        UNIT_ASSERT_VALUES_EQUAL(int(initRequest.method()), int(YabioProtobuf::Method::Classify));
        UNIT_ASSERT_VALUES_EQUAL(initRequest.group_id(), "uniproxy_test");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.model_id(), "");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.user_id(), "");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.requests_ids().size(), 0);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.classification_tags().size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(initRequest.classification_tags()[0], "test-gender");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.classification_tags()[1], "test-children");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.spotter(), true);
        UNIT_ASSERT(initRequest.GetclientHostname().size());
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetMessageId(), "test-message-id");
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishConverterBiometryRequestTest)
