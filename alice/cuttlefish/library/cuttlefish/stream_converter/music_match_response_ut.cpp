#include "music_match_response.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NMusicMatch;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NVoicetech::NUniproxy2;
using namespace NJson;

class TCuttlefishMusicMatchResponseTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishMusicMatchResponseTest);
    UNIT_TEST(TestMusicMatchStreamResponseToJsonOk);
    UNIT_TEST(TestMusicMatchStreamResponseToJsonNotOk);
    UNIT_TEST_SUITE_END();

public:
    void TestMusicMatchStreamResponseToJsonOk() {
        TJsonValue expectedAnswer = TJsonMap();
        {
            expectedAnswer["result"] = "music";
        }

        NProtobuf::TStreamResponse streamResponse;
        {
            auto* musicResult = streamResponse.MutableMusicResult();
            musicResult->SetIsOk(true);
            musicResult->SetRawMusicResultJson(expectedAnswer.GetStringRobust());
        }

        NJson::TJsonValue payload;
        MusicMatchStreamResponseToJson(streamResponse, payload);
        UNIT_ASSERT_VALUES_EQUAL_C(payload, expectedAnswer, payload.GetStringRobust());
    }

    void TestMusicMatchStreamResponseToJsonNotOk() {
        {
            // Not ok answer
            NProtobuf::TStreamResponse streamResponse;
            auto* musicResult = streamResponse.MutableMusicResult();
            musicResult->SetIsOk(false);

            NJson::TJsonValue payload;
            UNIT_ASSERT_EXCEPTION_CONTAINS(MusicMatchStreamResponseToJson(streamResponse, payload), yexception, "bad music match stream response");
        }

        {
            // Bad json
            NProtobuf::TStreamResponse streamResponse;
            auto* musicResult = streamResponse.MutableMusicResult();
            musicResult->SetIsOk(true);
            musicResult->SetRawMusicResultJson("9oikj230dsf234234sdfsdgs;;}|23234");

            NJson::TJsonValue payload;
            UNIT_ASSERT_EXCEPTION_CONTAINS(MusicMatchStreamResponseToJson(streamResponse, payload), yexception, "The document root must not be followed by other values");
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishMusicMatchResponseTest)
