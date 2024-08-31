#include <library/cpp/testing/unittest/registar.h>

#include "asr_result.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

using namespace NAlice::NAsr;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NVoicetech::NUniproxy2;
using namespace NJson;

class TCuttlefishConverterAsrResultTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishConverterAsrResultTest);
    UNIT_TEST(TestAsrResponseToJson_InitResponse);
    UNIT_TEST(TestAsrResponseToJson_AddDataResponse);
    UNIT_TEST(TestAsrResponseToJson_AddDataResponse_IsWhisper_false);
    UNIT_TEST(TestAsrResponseToJson_AddDataResponse_IsWhisper_empty);
    UNIT_TEST_SUITE_END();

public:
    void TestAsrResponseToJson_InitResponse() {
        NProtobuf::TResponse response;
        auto& initResponse = *response.MutableInitResponse();
        initResponse.SetIsOk(true);
        initResponse.SetHostname("test hostname");
        initResponse.SetTopic("test topic");
        initResponse.SetTopicVersion("test topic version");
        initResponse.SetServerVersion("test server version");

        NJson::TJsonValue payload;
        AsrResponseToJson(response, payload);
        UNIT_ASSERT_VALUES_EQUAL(payload["responseCode"].GetString(), "OK");
        UNIT_ASSERT_VALUES_EQUAL(payload["serverHostname"].GetString(), "test hostname");
        UNIT_ASSERT_VALUES_EQUAL(payload["topic"].GetString(), "test topic");
        UNIT_ASSERT_VALUES_EQUAL(payload["topicVersion"].GetString(), "test topic version");
        UNIT_ASSERT_VALUES_EQUAL(payload["serverVersion"].GetString(), "test server version");
    }

    void TestAsrResponseToJson_AddDataResponse() {
        NProtobuf::TResponse response;
        {
            auto& addDataResponse = *response.MutableAddDataResponse();
            addDataResponse.SetIsOk(true);
            addDataResponse.SetResponseStatus(NProtobuf::EndOfUtterance);
            addDataResponse.SetValidationInvoked(true);
            addDataResponse.SetDoNotSendToClient(true);
            {
                auto& recognition = *addDataResponse.MutableRecognition();
                {
                    auto& hypo = *recognition.AddHypos();
                    hypo.AddWords("test_word1");
                    hypo.AddWords("test_word2");
                    hypo.SetNormalized("test normalized");
                    hypo.SetTotalScore(0.666);
                    hypo.SetParentModel("test parent model");
                }
                {
                    auto& hypo = *recognition.AddHypos();
                    hypo.AddWords("test2_word1");
                    hypo.AddWords("test2_word2");
                    hypo.SetNormalized("test2 normalized");
                    hypo.SetTotalScore(0.999);
                    hypo.SetParentModel("test2 parent model");
                }
                {
                    auto& contextRef = *recognition.AddContextRef();
                    contextRef.SetIndex(11);
                    contextRef.SetContentIndex(41);
                    contextRef.SetConfidence(0.444);
                }
                recognition.SetThrownPartialsFraction(0.555);
            }
            //TODO:fill BioResult
            addDataResponse.SetCoreDebug("test core debug");
            addDataResponse.SetMessagesCount(42);
            addDataResponse.SetDurationProcessedAudio(4242);
            addDataResponse.SetIsWhisper(true);
        }
        NJson::TJsonValue payload;
        AsrResponseToJson(response, payload);
        //debug output
        Cout << NJson::WriteJson(payload, false, true) << Endl;
        UNIT_ASSERT_VALUES_EQUAL(payload["responseCode"].GetString(), "OK");
        UNIT_ASSERT(payload["recognition"].IsArray());
        UNIT_ASSERT_VALUES_EQUAL(payload["recognition"].GetArray().size(), 2);
        UNIT_ASSERT_DOUBLES_EQUAL(payload["recognition"][0]["confidence"].GetDouble(), 0.666, 0.0001);
        UNIT_ASSERT_VALUES_EQUAL(payload["recognition"][0]["words"][0]["value"].GetString(), "test_word1");
        UNIT_ASSERT_DOUBLES_EQUAL(payload["recognition"][0]["words"][0]["confidence"].GetDouble(), 1., 0.0001);
        UNIT_ASSERT_VALUES_EQUAL(payload["recognition"][0]["words"][1]["value"].GetString(), "test_word2");
        UNIT_ASSERT_DOUBLES_EQUAL(payload["recognition"][0]["words"][1]["confidence"].GetDouble(), 1., 0.0001);
        UNIT_ASSERT_VALUES_EQUAL(payload["recognition"][0]["normalized"].GetString(), "test normalized");
        UNIT_ASSERT_VALUES_EQUAL(payload["recognition"][0]["parentModel"].GetString(), "test parent model");
        UNIT_ASSERT_DOUBLES_EQUAL(payload["recognition"][1]["confidence"].GetDouble(), 0.999, 0.00001);
        UNIT_ASSERT_VALUES_EQUAL(payload["recognition"][1]["words"][0]["value"].GetString(), "test2_word1");
        UNIT_ASSERT_DOUBLES_EQUAL(payload["recognition"][1]["words"][0]["confidence"].GetDouble(), 1., 0.0001);
        UNIT_ASSERT_VALUES_EQUAL(payload["recognition"][1]["words"][1]["value"].GetString(), "test2_word2");
        UNIT_ASSERT_DOUBLES_EQUAL(payload["recognition"][1]["words"][1]["confidence"].GetDouble(), 1., 0.0001);
        UNIT_ASSERT_VALUES_EQUAL(payload["recognition"][1]["normalized"].GetString(), "test2 normalized");
        UNIT_ASSERT_VALUES_EQUAL(payload["recognition"][1]["parentModel"].GetString(), "test2 parent model");
        UNIT_ASSERT_VALUES_EQUAL(payload["contextRef"][0]["index"].GetUInteger(), 11);
        UNIT_ASSERT_VALUES_EQUAL(payload["contextRef"][0]["contentIndex"].GetUInteger(), 41);
        UNIT_ASSERT_DOUBLES_EQUAL(payload["contextRef"][0]["confidence"].GetDouble(), 0.444, 0.001);
        UNIT_ASSERT_VALUES_EQUAL(payload["endOfUtt"].GetBooleanSafe(), true);
        UNIT_ASSERT_VALUES_EQUAL(payload["messagesCount"].GetUInteger(), 42U);
        //TODO UNIT_ASSERT_VALUES_EQUAL(payload["biometryResult"].?, ?);
        //v2 protocol not has this data: UNIT_ASSERT_VALUES_EQUAL(payload["metaInfo"].?, ?);
        //v2 protocol not has this data: UNIT_ASSERT_VALUES_EQUAL(payload["earlyEndOfUtt"].GetBoolean(), false);
        UNIT_ASSERT_VALUES_EQUAL(payload["durationProcessedAudio"].GetString(), "4242");
        //v2 protocol not has this data: UNIT_ASSERT_VALUES_EQUAL(payload["is_trash"].GetBoolean(), false);
        //v2 protocol not has this data: UNIT_ASSERT_DOUBLES_EQUAL(payload["trash_score"].GetDouble(), ?, 0.001);
        UNIT_ASSERT_VALUES_EQUAL(payload["coreDebug"].GetString(), "test core debug");
        UNIT_ASSERT_DOUBLES_EQUAL(payload["thrownPartialsFraction"].GetDouble(), 0.555, 0.001);
        UNIT_ASSERT_VALUES_EQUAL(payload["doNotSendToClient"].GetBoolean(), true);
        UNIT_ASSERT_VALUES_EQUAL(payload["whisperInfo"]["isWhisper"].GetBoolean(), true);
    }

    void TestAsrResponseToJson_AddDataResponse_IsWhisper_false() {
        NProtobuf::TResponse response;
        {
            auto& addDataResponse = *response.MutableAddDataResponse();
            addDataResponse.SetIsOk(true);
            addDataResponse.SetIsWhisper(false);
        }
        NJson::TJsonValue payload;
        AsrResponseToJson(response, payload);
        //debug output
        Cout << NJson::WriteJson(payload, false, true) << Endl;

        UNIT_ASSERT_VALUES_EQUAL(payload["whisperInfo"]["isWhisper"].GetBoolean(), false);
    }

    void TestAsrResponseToJson_AddDataResponse_IsWhisper_empty() {
        NProtobuf::TResponse response;
        {
            auto& addDataResponse = *response.MutableAddDataResponse();
            addDataResponse.SetIsOk(true);
        }
        NJson::TJsonValue payload;
        AsrResponseToJson(response, payload);
        //debug output
        Cout << NJson::WriteJson(payload, false, true) << Endl;

        UNIT_ASSERT_VALUES_EQUAL(payload["whisperInfo"]["isWhisper"].GetBoolean(), false);
    }

};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishConverterAsrResultTest)
