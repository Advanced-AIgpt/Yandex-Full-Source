#include <alice/uniproxy/mapper/uniproxy_client/lib/async_uniproxy_client.h>
#include <alice/uniproxy/mapper/uniproxy_client/lib/uniproxy_client.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/logger/stream.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/strbuf.h>
#include <util/stream/file.h>

#include <memory>

using namespace NAlice::NUniproxy;
using namespace NJson;
using namespace std;

namespace {
    class TContext {
    private:
        TLog Log;
        unique_ptr<TUniproxyClient> Client_;

    public:
        TContext(const bool async = false)
            : Log(TAutoPtr<TLogBackend>(new TStreamLogBackend(&Cout)))
        {
            SetTimestampFormatter(Log);

            TUniproxyClientParams params;
            params.UniproxyUrl = "wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws";
            params.AuthToken = "6e9e2484-5f4a-45f1-b857-47ea867bfe8a";
            params.Uuid = "ffffffff-ffff-ffff-681f-7da4c37a4540";
            params.AsrChunkSize = 820;
            params.AsrChunkDelayMs = 200;
            params.Logger = &Log;
            params.DisableServerCertificateValidation = true;
            params.Language = "ru-RU";
            params.Timezone = "Europe/Moscow";

            TString application_str = R"({
                "app_id": "simple.client.test",
                "app_version": "0.0.1",
                "os_version": "10.0",
                "platform": "Windows",
                "timezone": "Europe/Moscow",
                "client_time": "20191112T174504",
                "timestamp": "1573580704"
            })";
            TJsonValue application;
            ReadJsonTree(application_str, &application, /* throwOnError= */ true);
            params.Application = std::move(application);
            if (async) {
                Client_.reset(new TAsyncUniproxyClient(params));
            } else {
                Client_.reset(new TUniproxyClient(params));
            }
        }

        TUniproxyClient& Client() {
            return *Client_;
        }
    };

}

Y_UNIT_TEST_SUITE(TUniproxyClientTest) {
    Y_UNIT_TEST(TestTextRequest) {
        TContext context;
        Cout << "Remote addr=" << context.Client().GetRemoteAddress() << ", port=" << context.Client().GetRemotePort() << Endl;
        auto result = context.Client().SendTextRequest("Ответ на главный вопрос жизни");
        UNIT_ASSERT_EQUAL_C(result.size(), 1, "Exactly one result expected");
        UNIT_ASSERT_EQUAL(result[0].Type, EResponseType::Vins);
        UNIT_ASSERT_C(!result[0].StreamId.has_value(), "Text response expected");
        UNIT_ASSERT_C(result[0].Data.size() > 0, "Non-empty response expected");
    }

    Y_UNIT_TEST(TestVoiceRequest) {
        TFileInput input("address.opus");
        TContext context;
        Cout << "Remote addr=" << context.Client().GetRemoteAddress() << ", port=" << context.Client().GetRemotePort() << Endl;

        auto result = context.Client().SendVoiceRequest(TStringBuf("dialog-general"), input);
        UNIT_ASSERT_C(result.size() >= 4, "Expected at least 4 responses");

        size_t index = result.size() - 1;
        UNIT_ASSERT_EQUAL(result[index].Type, EResponseType::TtsStream);
        UNIT_ASSERT_EQUAL(result[--index].Type, EResponseType::TtsText);
        UNIT_ASSERT_EQUAL(result[--index].Type, EResponseType::Vins);
        do {
            UNIT_ASSERT_EQUAL(result[--index].Type, EResponseType::Asr);
        } while (index != 0);

        for (size_t i = 0; i + 1 < result.size(); ++i) {
            UNIT_ASSERT_C(!result[i].StreamId.has_value(), "All responses except the last one should be text responses");
            UNIT_ASSERT_C(result[i].Data.size() > 0, "All text responses should be non-empty");
        }
        UNIT_ASSERT_C(result.back().StreamId.has_value(), "The last response should be a stream");
        UNIT_ASSERT_C(result.back().Data.size() > 0, "The stream response should be non-empty");
    }
}

Y_UNIT_TEST_SUITE(TAsyncUniproxyClientTest) {
    Y_UNIT_TEST(TestTextRequest) {
        TContext context(true);
        Cout << "Remote addr=" << context.Client().GetRemoteAddress() << ", port=" << context.Client().GetRemotePort() << Endl;
        auto result = context.Client().SendTextRequest("Ответ на главный вопрос жизни");
        UNIT_ASSERT_EQUAL_C(result.size(), 1, "Exactly one result expected");
        UNIT_ASSERT_EQUAL(result[0].Type, EResponseType::Vins);
        UNIT_ASSERT_C(!result[0].StreamId.has_value(), "Text response expected");
        UNIT_ASSERT_C(result[0].Data.size() > 0, "Non-empty response expected");
    }

    Y_UNIT_TEST(TestVoiceRequest) {
        TFileInput input("address.opus");
        TContext context(true);
        Cout << "Remote addr=" << context.Client().GetRemoteAddress() << ", port=" << context.Client().GetRemotePort() << Endl;

        auto result = context.Client().SendVoiceRequest(TStringBuf("dialog-general"), input);
        UNIT_ASSERT_C(result.size() >= 4, "Expected at least 4 responses");

        size_t index = result.size() - 1;
        UNIT_ASSERT_EQUAL(result[index].Type, EResponseType::TtsStream);
        UNIT_ASSERT_EQUAL(result[--index].Type, EResponseType::TtsText);
        UNIT_ASSERT_EQUAL(result[--index].Type, EResponseType::Vins);
        do {
            UNIT_ASSERT_EQUAL(result[--index].Type, EResponseType::Asr);
        } while (index != 0);

        for (size_t i = 0; i + 1 < result.size(); ++i) {
            UNIT_ASSERT_C(!result[i].StreamId.has_value(), "All responses except the last one should be text responses");
            UNIT_ASSERT_C(result[i].Data.size() > 0, "All text responses should be non-empty");
        }
        UNIT_ASSERT_C(result.back().StreamId.has_value(), "The last response should be a stream");
        UNIT_ASSERT_C(result.back().Data.size() > 0, "The stream response should be non-empty");
    }
}
