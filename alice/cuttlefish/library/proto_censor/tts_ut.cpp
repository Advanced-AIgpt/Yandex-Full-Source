#include "tts.h"

#include <apphost/lib/proto_answers/http.pb.h>
#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NCuttlefish;


class TCuttlefishProtoCensorTtsTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishProtoCensorTtsTest);
    UNIT_TEST(TestCensoreTtsRequest);
    UNIT_TEST(TestCensoreTtsBackendRequest);
    UNIT_TEST(TestCensoreTtsRequestSenderRequest);
    UNIT_TEST(TestCensoreTtsAggregatorRequest);
    UNIT_TEST_SUITE_END();

public:
    void TestCensoreTtsRequest() {
        for (size_t doNotLogTexts = 0; doNotLogTexts < 2; ++doNotLogTexts) {
            NTts::TRequest ttsRequest;
            {
                ttsRequest.SetText("Some text");
                ttsRequest.SetDoNotLogTexts((bool)doNotLogTexts);
            }

            Censore(ttsRequest);

            if (doNotLogTexts) {
                UNIT_ASSERT_VALUES_EQUAL(ttsRequest.GetText(), "<CENSORED>, size = 9");
            } else {
                UNIT_ASSERT_VALUES_EQUAL(ttsRequest.GetText(), "Some text");
            }
        }
    }

    void TestCensoreTtsBackendRequest() {
        for (size_t doNotLogTexts = 0; doNotLogTexts < 2; ++doNotLogTexts) {
            NTts::TBackendRequest ttsBackendRequest;
            {
                ttsBackendRequest.MutableGenerate()->set_text("Some text");
                ttsBackendRequest.SetDoNotLogTexts((bool)doNotLogTexts);
            }

            Censore(ttsBackendRequest);

            if (doNotLogTexts) {
                UNIT_ASSERT_VALUES_EQUAL(ttsBackendRequest.GetGenerate().text(), "<CENSORED>, size = 9");
            } else {
                UNIT_ASSERT_VALUES_EQUAL(ttsBackendRequest.GetGenerate().text(), "Some text");
            }
        }
    }

    void TestCensoreTtsRequestSenderRequest() {
        for (size_t doNotLogTextsRequestSenderRequest = 0; doNotLogTextsRequestSenderRequest < 2; ++doNotLogTextsRequestSenderRequest) {
            for (size_t doNotLogTextsBackendRequest = 0; doNotLogTextsBackendRequest < 2; ++doNotLogTextsBackendRequest) {
                NTts::TRequestSenderRequest ttsRequestSenderRequest;
                {
                    {
                        auto& audioPartGenerateRequest = *ttsRequestSenderRequest.MutableAudioPartGenerateRequests()->Add();
                        auto& httpRequest = *audioPartGenerateRequest.MutableRequest()->MutableHttpRequest();
                        httpRequest.SetPath("/test");
                    }
                    {
                        auto& audioPartGenerateRequest = *ttsRequestSenderRequest.MutableAudioPartGenerateRequests()->Add();
                        auto& ttsBackendRequest = *audioPartGenerateRequest.MutableRequest()->MutableBackendRequest();
                        ttsBackendRequest.MutableGenerate()->set_text("Some text");
                        ttsBackendRequest.SetDoNotLogTexts((bool)doNotLogTextsBackendRequest);
                    }

                    ttsRequestSenderRequest.SetDoNotLogTexts((bool)doNotLogTextsRequestSenderRequest);
                }

                Censore(ttsRequestSenderRequest);

                UNIT_ASSERT_VALUES_EQUAL(ttsRequestSenderRequest.GetAudioPartGenerateRequests().size(), 2);
                UNIT_ASSERT_VALUES_EQUAL(
                    ttsRequestSenderRequest.GetAudioPartGenerateRequests()[0].GetRequest().GetHttpRequest().GetPath(),
                    "/test"
                );

                if (doNotLogTextsRequestSenderRequest || doNotLogTextsBackendRequest) {
                    UNIT_ASSERT_VALUES_EQUAL(
                        ttsRequestSenderRequest.GetAudioPartGenerateRequests()[1].GetRequest().GetBackendRequest().GetGenerate().text(),
                        "<CENSORED>, size = 9"
                    );
                } else {
                    UNIT_ASSERT_VALUES_EQUAL(
                        ttsRequestSenderRequest.GetAudioPartGenerateRequests()[1].GetRequest().GetBackendRequest().GetGenerate().text(),
                        "Some text"
                    );
                }
            }
        }
    }

    void TestCensoreTtsAggregatorRequest() {
        for (size_t doNotLogTexts = 0; doNotLogTexts < 2; ++doNotLogTexts) {
            NTts::TAggregatorRequest ttsAggregatorRequest;
            {
                ttsAggregatorRequest.SetDoNotLogTexts((bool)doNotLogTexts);
            }

            Censore(ttsAggregatorRequest);
            // Do nothing
            // There are no info about texts in aggregator request for now
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishProtoCensorTtsTest)
