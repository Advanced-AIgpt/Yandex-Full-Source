#include <library/cpp/testing/unittest/registar.h>

#include "tts_generate_response.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

using namespace NAlice::NTts;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NVoicetech::NUniproxy2;
using namespace NJson;

class TCuttlefishTtsGenerateResponseTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishTtsGenerateResponseTest);
    UNIT_TEST(TestTtsGenerateResponseTimingsToJson);
    UNIT_TEST_SUITE_END();

public:
    void TestTtsGenerateResponseTimingsToJson() {
        TTS::GenerateResponse::Timings timings;
        {
            {
                auto& timings0 = *timings.add_timings();
                timings0.set_time(0.111);
                timings0.set_phoneme("test phoneme");
                timings0.set_word("test word");
            }
            {
                auto& utterance = *timings.mutable_utterance();
                {
                    auto& word0 = *utterance.add_words();
                    word0.set_word("test utt word");
                    word0.add_phonemes("test phoneme schwa");
                    word0.add_phonemes("test phoneme mm");
                }
            }
        }

        for (int fromCache = 0; fromCache < 2; ++fromCache) {
            NJson::TJsonValue payload;
            TtsGenerateResponseTimingsToJson(timings, (bool)fromCache, payload);

            UNIT_ASSERT_VALUES_EQUAL(payload["from_cache"].GetBoolean(), (bool)fromCache);

            UNIT_ASSERT_VALUES_EQUAL(payload["timings"].GetArray().size(), 1);
            auto& timings0 = payload["timings"][0];
            UNIT_ASSERT_DOUBLES_EQUAL(timings0["time"].GetDouble(), 0.111, 0.000001);
            UNIT_ASSERT_VALUES_EQUAL(timings0["phoneme"].GetString(), "test phoneme");
            UNIT_ASSERT_VALUES_EQUAL(timings0["word"].GetString(), "test word");
            UNIT_ASSERT_VALUES_EQUAL(payload["utterance"]["words"].GetArray().size(), 1);

            UNIT_ASSERT_VALUES_EQUAL(payload["utterance"]["words"].GetArray().size(), 1);
            auto& word0 = payload["utterance"]["words"][0];
            UNIT_ASSERT_VALUES_EQUAL(word0["word"].GetString(), "test utt word");
            UNIT_ASSERT_VALUES_EQUAL(word0["phonemes"].GetArray().size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(word0["phonemes"][0].GetString(), "test phoneme schwa");
            UNIT_ASSERT_VALUES_EQUAL(word0["phonemes"][1].GetString(), "test phoneme mm");
        }
    }

};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishTtsGenerateResponseTest)
