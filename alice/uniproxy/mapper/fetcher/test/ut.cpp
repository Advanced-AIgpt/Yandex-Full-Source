#include <alice/uniproxy/mapper/fetcher/lib/request.h>
#include <alice/uniproxy/mapper/fetcher/lib/protos/voice_input.pb.h>
#include <alice/uniproxy/mapper/library/logging/logging.h>

#include <search/scraper_over_yt/mapper/lib/alice_binary_holder.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/logger/stream.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/yson/node/node_io.h>

#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>
#include <util/stream/file.h>

namespace {
    using namespace NAlice::NUniproxy;
    using namespace NAlice::NUniproxy::NFetcher;
    using namespace NJson;

    void CheckTextRequestResult(const TResponses& result) {
        UNIT_ASSERT_EQUAL_C(result.size(), 1, "Exactly one result expected");
        UNIT_ASSERT_EQUAL(result[0].Type, EResponseType::Vins);
        UNIT_ASSERT_C(!result[0].StreamId.has_value(), "Text response expected");
        UNIT_ASSERT_C(result[0].Data.size() > 0, "Non-empty response expected");
    }

    void CheckVoiceRequestResult(const TResponses& result) {
        UNIT_ASSERT_C(result.size() >= 4, "Expected at least 4 responses");

        size_t index = result.size() - 1;
        UNIT_ASSERT_EQUAL(result[index].Type, EResponseType::TtsStream);
        UNIT_ASSERT_EQUAL(result[--index].Type, EResponseType::TtsText);
        UNIT_ASSERT_EQUAL(result[--index].Type, EResponseType::Vins);

        THashSet<EResponseType> asrBioTypes{EResponseType::Asr, EResponseType::Bio};
        do {
            UNIT_ASSERT_C(asrBioTypes.contains(result[--index].Type),
                          "Invalid response type, Asr or Bio expected");
        } while (index != 0);

        for (size_t i = 0; i + 1 < result.size(); ++i) {
            UNIT_ASSERT_C(!result[i].StreamId.has_value(), "All responses except the last one should be text responses");
            UNIT_ASSERT_C(result[i].Data.size() > 0, "All text responses should be non-empty");
        }
        UNIT_ASSERT_C(result.back().StreamId.has_value(), "The last response should be a stream");
        UNIT_ASSERT_C(result.back().Data.size() > 0, "The stream response should be non-empty");
    }

    TVoiceInput CreateVoiceInput() {
        TVoiceInput voiceInput;
        voiceInput.SetRequestId("ffffffff-ffff-40c5-bae3-59a3ee1daf5b");
        voiceInput.SetUuid("ffffffff-ffff-40c5-bae3-59a3ee1daf5b");
        voiceInput.SetTopic("dialog-general");
        voiceInput.SetTimezone("Europe/Moscow");
        voiceInput.SetLang("ru-RU");
        voiceInput.SetFormat("audio/opus");
        voiceInput.SetPayload(R"({
            "header": {"request_id": "98556ba9-8c40-40c5-bae3-59a3ee1daf5b"},
            "advancedASROptions": {
                "manual_punctuation": false,
                "partial_results": true
            },
            "punctuation": true,
            "application": {
                "app_id": "ru.yandex.searchplugin",
                "app_version": "20.81",
                "device_manufacturer": "samsung",
                "device_model": "SM-G965F",
                "os_version": "9",
                "platform": "android",
                "device_id": "feedface-e22e-4abb-86f3-5105e892a8b9",
                "uuid": "deadbeef-f77a-44e3-9982-7028be490bb7",
                "lang": "ru-RU",
                "client_time": "20200327T170058",
                "timestamp": "1585328458",
                "timezone": "Europe/Moscow"
            },
            "request": {
                "event": {},
                "location": {"lon": 37.587937, "lat": 55.733771},
                "device_state": {},
                "experiments": {},
                "reset_session": false,
                "voice_session": true
            },
            "disableAntimatNormalizer": true
        })");
        voiceInput.SetAuthToken("6e9e2484-5f4a-45f1-b857-47ea867bfe8a");
        voiceInput.SetApplication(R"({
            "app_id": "ru.yandex.searchplugin",
            "app_version": "20.81",
            "device_manufacturer": "samsung",
            "device_model": "SM-G965F",
            "os_version": "9",
            "platform": "android",
            "device_id": "feedface-e22e-4abb-86f3-5105e892a8b9",
            "uuid": "deadbeef-f77a-44e3-9982-7028be490bb7",
            "lang": "ru-RU",
            "client_time": "20200327T170058",
            "timestamp": "1585328458",
            "timezone": "Europe/Moscow"
        })");
        return voiceInput;
    }

    class TRequestSessionContext {
    private:
        TLog Log;

    public:
        THolder<TUniproxyRequestPerformer> Performer;

    public:
        TRequestSessionContext(const TVoiceInput& voiceInput)
            : Log(TAutoPtr<TLogBackend>(new TStreamLogBackend(&Cout)))
        {
            SetTimestampFormatter(Log);
            Performer = MakeHolder<TUniproxyRequestPerformer>(
                voiceInput,
                "wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws", /* uniproxyUrl */
                "6e9e2484-5f4a-45f1-b857-47ea867bfe8a",                               /* authToken */
                "http://vins.hamster.alice.yandex.net/speechkit/app/pa/",             /* vinsUrl */
                "2020-03-17T150000",                                                  /* clientTime */
                -1,                                                                   /* asrChunkSize */
                0,                                                                    /* asrChunkDelayMs */
                &Log);                                                                /* log */
        }
    };

    Y_UNIT_TEST_SUITE(TUniproxyBinaryHolder) {
        Y_UNIT_TEST(TestBinaryHolder) {
            TString path = BinaryPath(
                "alice/uniproxy/mapper/test_bin/bin/bin");
            TJsonValue params;
            params["fetcher_mode"] = "text";
            TUniproxyBinaryHolder binHolder(path, params);
            Cerr << binHolder.StderrStatus() << Endl;

            NYT::TNode startCmd;
            startCmd["command"] = "start";
            NYT::TNode payload;
            payload["test_name"] = "hello_and_repeat";
            startCmd["payload"] = payload;
            auto binRequest = NYT::NodeToYsonString(startCmd, NYson::EYsonFormat::Text);
            auto binResponse = binHolder.Process(binRequest);
            Cerr << binHolder.StderrStatus() << Endl;
            UNIT_ASSERT(binResponse);
            Cerr << *binResponse << Endl;
            auto binResponseNode = NYT::NodeFromYsonString(TStringBuf(*binResponse), ::NYson::EYsonType::Node);
            UNIT_ASSERT_EQUAL(binResponseNode["type"], "next_request");
            auto nextRequest = binResponseNode["payload"];

            UNIT_ASSERT_EQUAL(nextRequest["FetcherMode"], "text");
            UNIT_ASSERT_EQUAL(nextRequest["Text"], "привет алисонька как дела");
        }

        Y_UNIT_TEST(DownloaderBinaryHolder) {
            TString path = BinaryPath("alice/acceptance/modules/request_generator/scrapper/bin/downloader_inner");
            TJsonValue params;
            params["fetcher_mode"] = "text";
            TUniproxyBinaryHolder binHolder(path, params);
            Cerr << binHolder.StderrStatus() << Endl;

            auto expected_text = "привет алисонька как дела";
            NYT::TNode request;
            request["text"] = expected_text;
            request["request_id"] = "1234";
            NYT::TNode payload;
            payload["session_requests"] = NYT::TNode::CreateList();
            payload["session_requests"].Add(std::move(request));

            NYT::TNode startCmd;
            startCmd["command"] = "start";
            startCmd["payload"] = payload;
            auto binRequest = NYT::NodeToYsonString(startCmd, NYson::EYsonFormat::Text);
            Cerr << binRequest << Endl;
            auto binResponse = binHolder.Process(binRequest);
            Cerr << binHolder.StderrStatus() << Endl;
            UNIT_ASSERT(binResponse);
            Cerr << *binResponse << Endl;
            auto binResponseNode = NYT::NodeFromYsonString(TStringBuf(*binResponse), ::NYson::EYsonType::Node);
            UNIT_ASSERT_EQUAL(binResponseNode["type"], "next_request");
            auto nextRequest = binResponseNode["payload"];

            UNIT_ASSERT_EQUAL(nextRequest["FetcherMode"], "text");
            UNIT_ASSERT_EQUAL(nextRequest["Text"], expected_text);
            UNIT_ASSERT_EQUAL(nextRequest["RequestId"], "1234");

        }
    }

    Y_UNIT_TEST_SUITE(TUniproxyRequestPerformer) {
        Y_UNIT_TEST(TestTextRequest) {
            auto voiceInput = CreateVoiceInput();
            voiceInput.SetTextData("привет алисонька что делаешь");
            voiceInput.SetFetcherMode("text");

            TRequestSessionContext context(voiceInput);
            auto result = PerformUniproxyRequestInsideSession(*context.Performer, voiceInput);
            CheckTextRequestResult(result);
        }

        Y_UNIT_TEST(TestVoiceRequest) {
            TFileInput input("address.opus");
            auto opus = input.ReadAll();
            UNIT_ASSERT_C(opus.size() > 0, "Empty opus file");
            auto voiceInput = CreateVoiceInput();
            voiceInput.SetVoiceData(opus);
            voiceInput.SetFetcherMode("voice");

            TRequestSessionContext context(voiceInput);
            auto result = PerformUniproxyRequestInsideSession(*context.Performer, voiceInput);
            CheckVoiceRequestResult(result);
        }
    }

    Y_UNIT_TEST_SUITE(SimpleUniproxyRequest) {
        Y_UNIT_TEST(TestTextRequest) {
            auto voiceInput = CreateVoiceInput();
            voiceInput.SetTextData("привет алисонька что делаешь");
            voiceInput.SetFetcherMode("text");
            auto result = PerformUniproxyRequest(
                voiceInput,
                "wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws", /* uniproxyUrl */
                "6e9e2484-5f4a-45f1-b857-47ea867bfe8a",                               /* authToken */
                "http://vins.hamster.alice.yandex.net/speechkit/app/pa/",             /* vinsUrl */
                "2020-03-17T150000",                                                  /* clientTime */
                -1,                                                                   /* asrChunkSize */
                0);                                                                   /* asrChunkDelayMs */
            CheckTextRequestResult(result);
        }

        Y_UNIT_TEST(TestVoiceRequest) {
            TFileInput input("address.opus");
            auto opus = input.ReadAll();
            UNIT_ASSERT_C(opus.size() > 0, "Empty opus file");

            auto voiceInput = CreateVoiceInput();
            voiceInput.SetVoiceData(opus);
            voiceInput.SetFetcherMode("voice");
            auto result = PerformUniproxyRequest(
                voiceInput,
                "wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws", /* uniproxyUrl */
                "6e9e2484-5f4a-45f1-b857-47ea867bfe8a",                               /* authToken */
                "http://vins.hamster.alice.yandex.net/speechkit/app/pa/",             /* vinsUrl */
                "2020-03-17T150000",                                                  /* clientTime */
                -1,                                                                   /* asrChunkSize */
                0);                                                                   /* asrChunkDelayMs */
            CheckVoiceRequestResult(result);
        }
    }
}
