#include "music_match_request.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NMusicMatch;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NVoicetech::NUniproxy2;
using namespace NJson;

namespace {
    static const TString musicMatchViaAsr = R"(
{
  "event": {
    "header": {
      "messageId": "test-message-id",
      "namespace": "ASR",
      "name": "Recognize",
      "streamId": 42
    },
    "payload": {
      "music_request2": {
        "headers": {
          "Content-Type": "audio/opus"
        }
      },
      "format": "audio/opus"
    }
  }
}
)";
    static const TString musicMatchViaAsrMusicOnly = R"(
{
  "event": {
    "header": {
      "messageId": "test-message-id",
      "namespace": "ASR",
      "name": "Recognize",
      "streamId": 42
    },
    "payload": {
      "music_request2": {
        "headers": {
          "Content-Type": "audio/opus"
        }
      },
      "format": "audio/opus",
      "recognize_music_only": true
    }
  }
}
)";

    static const TString musicMatchViaVins = R"(
{
  "event": {
    "header": {
      "messageId": "test-message-id",
      "namespace": "Vins",
      "name": "MusicInput",
      "streamId": 42
    },
    "payload": {
      "format": "audio/opus"
    }
  }
}
)";
}

class TCuttlefishConverterMusicMatchRequestTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishConverterMusicMatchRequestTest);
    UNIT_TEST(TestIsMusicMatchRequestViaAsr);
    UNIT_TEST(TestIsRecognizeMusicOnly);
    UNIT_TEST(TestVinsMessageToMusicMatchInitRequest);
    UNIT_TEST(TestAsrMessageToMusicMatchInitRequest)
    UNIT_TEST_SUITE_END();

public:
    void TestIsMusicMatchRequestViaAsr() {
        {
            // Test return true
            TMessage message(TMessage::FromClient, musicMatchViaAsr);
            UNIT_ASSERT(IsMusicMatchRequestViaAsr(message));
        }

        {
            // Test return false
            TMessage message(TMessage::FromClient, musicMatchViaVins);
            UNIT_ASSERT(!IsMusicMatchRequestViaAsr(message));
        }
    }

    void TestIsRecognizeMusicOnly() {
        {
            // Test return true
            TMessage message(TMessage::FromClient, musicMatchViaAsrMusicOnly);
            UNIT_ASSERT(IsRecognizeMusicOnly(message));
        }

        {
            // Test return false
            TMessage message(TMessage::FromClient, musicMatchViaAsr);
            UNIT_ASSERT(!IsRecognizeMusicOnly(message));
        }
    }

    void TestVinsMessageToMusicMatchInitRequest() {
        TMessage message(TMessage::FromClient, musicMatchViaVins);
        NProtobuf::TInitRequest initRequest;
        VinsMessageToMusicMatchInitRequest(message, initRequest);

        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetRequestId(), "test-message-id");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetHeaders(), "");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetAudioFormat(), "audio/opus");
    }

    void TestAsrMessageToMusicMatchInitRequest() {
        TMessage message(TMessage::FromClient, musicMatchViaAsr);
        NProtobuf::TInitRequest initRequest;
        AsrMessageToMusicMatchInitRequest(message, initRequest);

        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetRequestId(), "test-message-id");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetHeaders(), "Content-Type: audio/opus\r\n");
        UNIT_ASSERT_VALUES_EQUAL(initRequest.GetAudioFormat(), "audio/opus");
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishConverterMusicMatchRequestTest)
