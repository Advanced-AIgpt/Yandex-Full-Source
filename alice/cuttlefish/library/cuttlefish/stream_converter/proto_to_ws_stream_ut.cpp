#include "proto_to_ws_stream.h"

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NCuttlefish::NAppHostServices;

class TProtobufToWsStreamTest: public TTestBase {
    UNIT_TEST_SUITE(TProtobufToWsStreamTest);
    UNIT_TEST(TestOutputSpeechContainsSpotterWord);
    UNIT_TEST_SUITE_END();

public:
    void TestOutputSpeechContainsSpotterWord() {
        UNIT_ASSERT(TProtobufToWsStream::OutputSpeechContainsSpotterWord("АлИсА"));
        UNIT_ASSERT(TProtobufToWsStream::OutputSpeechContainsSpotterWord("алиса"));
        UNIT_ASSERT(TProtobufToWsStream::OutputSpeechContainsSpotterWord("Яндекс"));
        UNIT_ASSERT(TProtobufToWsStream::OutputSpeechContainsSpotterWord("Алиса Яндекс"));
        UNIT_ASSERT(!TProtobufToWsStream::OutputSpeechContainsSpotterWord("ЯнДеКс НоВоСтИщИ АлИсА"));
        UNIT_ASSERT(TProtobufToWsStream::OutputSpeechContainsSpotterWord("В стране чудес Алиса in wonderland"));
        UNIT_ASSERT(!TProtobufToWsStream::OutputSpeechContainsSpotterWord("В стране чудес in wonderland"));
    }
};

UNIT_TEST_SUITE_REGISTRATION(TProtobufToWsStreamTest);
