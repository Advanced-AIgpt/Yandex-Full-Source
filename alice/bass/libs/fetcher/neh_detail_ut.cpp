#include "neh_detail.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace NHttpFetcher {

namespace {
const TResponse::TSystemErrorCode CONNECTION_REFUSED = 111;

class TFakeTestRequest : public TNehRequest {
private:
    static NUri::TUri CreateUri(TStringBuf url) {
        NUri::TUri uri;
        uri.Parse(url);
        return uri;
    }
public:
    TFakeTestRequest(TStringBuf url) 
        : TNehRequest{CreateUri(url), {}, /* rtlog */nullptr, /* overrideReqId */false}
    {
    }
    TFakeTestRequest(TStringBuf url, TProxySettingsPtr proxy)
        : TNehRequest{CreateUri(url), proxy, /* rtlog */nullptr, /* overrideReqId */false}
    {
    }

    THandle::TRef Fetch() override {
        return {};
    }

    using TNehRequest::MakeNehAttemptFactory;
};

} // namespace

Y_UNIT_TEST_SUITE(NehDetail) {
    Y_UNIT_TEST(UseProxyNormal) {
        static constexpr TStringBuf requestData1 = "GET http://tmp.ru/get?what=something HTTP/1.1\r\nHost: tmp.ru\r\nX-Pr-Header-1: value1\r\nX-Pr-Header-2: value2\r\nX-Host: http://tmp.ru/get?what=something\r\nX-Fetcher-Attempt-Number: 0\r\n\r\n";
        static constexpr TStringBuf requestData2 = "GET http://tmp.ru/get?what=something HTTP/1.1\r\nHost: tmp.ru\r\nX-Pr-Header-1: value1\r\nX-Pr-Header-2: value2\r\nX-Host: http://tmp.ru/get?what=something\r\nX-Fetcher-Attempt-Number: 1\r\n\r\n";
        THttpHeaders proxyHeaders;
        proxyHeaders.AddHeader({"X-Pr-Header-1", "value1"});
        proxyHeaders.AddHeader({"X-Pr-Header-2", "value2"});
        TProxySettingsPtr proxySettings = new TProxySettings("proxy.ru:1234", std::move(proxyHeaders), TProxySettings::EProxyMode::Normal);

        TFakeTestRequest req{"http://tmp.ru/get?what=something", proxySettings};
        {
            auto reqFactory = req.MakeNehAttemptFactory();
            TString rtLogToken;
            auto msg = reqFactory.CreateMessage(0, rtLogToken);
            UNIT_ASSERT_VALUES_EQUAL(msg.Addr, "full://proxy.ru:1234");
            UNIT_ASSERT_VALUES_EQUAL(msg.Data, requestData1);
        }
        {
            auto reqFactory = req.MakeNehAttemptFactory();
            TString rtLogToken;
            auto msg = reqFactory.CreateMessage(1, rtLogToken);
            UNIT_ASSERT_VALUES_EQUAL(msg.Addr, "full://proxy.ru:1234");
            UNIT_ASSERT_VALUES_EQUAL(msg.Data, requestData2);
        }
    }

    Y_UNIT_TEST(UseProxyBehindBalancer) {
        static constexpr TStringBuf requestData = "GET /?what=something HTTP/1.1\r\nHost: proxy.ru:1234\r\nA: B\r\nX-Pr-Header-1: value1\r\nX-Pr-Header-2: value2\r\nX-Host: http://tmp.ru/get?what=something\r\nX-Fetcher-Attempt-Number: 0\r\n\r\n";

        THttpHeaders proxyHeaders;
        proxyHeaders.AddHeader({"X-Pr-Header-1", "value1"});
        proxyHeaders.AddHeader({"X-Pr-Header-2", "value2"});
        TProxySettingsPtr proxySettings = new TProxySettings("proxy.ru:1234", std::move(proxyHeaders), TProxySettings::EProxyMode::BehindBalancer);

        TFakeTestRequest req{"http://tmp.ru/get?what=something", proxySettings};
        req.AddHeader("A", "B");
        auto reqFactory = req.MakeNehAttemptFactory();
        TString rtLogToken;
        auto msg = reqFactory.CreateMessage(0, rtLogToken);

        UNIT_ASSERT_VALUES_EQUAL(msg.Addr, "full://proxy.ru:1234/");
        UNIT_ASSERT_VALUES_EQUAL(msg.Data, requestData);
    }

    Y_UNIT_TEST(UseProxyAndOtherProxyBehindBalancer) {
        static constexpr TStringBuf requestData = "GET /?what=something HTTP/1.1\r\nHost: proxy.ru:1234\r\nX-Pr-Header-1: value1\r\nX-Pr-Header-2: value2\r\nX-Host: http://qwerty.net/get?what=something\r\nx-yandex-via-proxy: zora-zora-zora.yandex.net:1234\r\nX-Fetcher-Attempt-Number: 0\r\n\r\n";

        THttpHeaders proxyHeaders;
        proxyHeaders.AddHeader({"X-Pr-Header-1", "value1"});
        proxyHeaders.AddHeader({"X-Pr-Header-2", "value2"});
        TProxySettingsPtr proxySettings = new TProxySettings("proxy.ru:1234", std::move(proxyHeaders), TProxySettings::EProxyMode::BehindBalancer);

        TFakeTestRequest req{"http://qwerty.net/get?what=something", proxySettings};
        req.SetProxy("zora-zora-zora.yandex.net:1234");
        auto reqFactory = req.MakeNehAttemptFactory();
        TString rtLogToken;
        auto msg = reqFactory.CreateMessage(0, rtLogToken);

        UNIT_ASSERT_VALUES_EQUAL(msg.Addr, "full://proxy.ru:1234/");
        UNIT_ASSERT_VALUES_EQUAL(msg.Data, requestData);
    }

    Y_UNIT_TEST(UseProxyButNotOverride) {
        static constexpr TStringBuf requestData = "GET http://qwerty.net/get?what=something HTTP/1.1\r\nHost: qwerty.net\r\nX-Host: http://qwerty.net/get?what=something\r\nX-Fetcher-Attempt-Number: 0\r\n\r\n";

        TFakeTestRequest req("http://qwerty.net/get?what=something");
        req.SetProxy("zora-zora-zora.yandex.net:1234");
        auto reqFactory = req.MakeNehAttemptFactory();
        TString rtLogToken;
        auto msg = reqFactory.CreateMessage(0, rtLogToken);

        UNIT_ASSERT_VALUES_EQUAL(msg.Addr, "full://zora-zora-zora.yandex.net:1234");
        UNIT_ASSERT_VALUES_EQUAL(msg.Data, requestData);
    }

    Y_UNIT_TEST(ConnectionRetry) {
        NHttpFetcher::TRequestOptions options;
        options.MaxConnectionAttemptMs = TDuration::Seconds(99999);
        options.MaxConnectionAttempts = 2;

        auto handle = NHttpFetcher::Request("http://localhost:1337/", options)->Fetch();

        const auto response = handle->Wait();
        UNIT_ASSERT_VALUES_EQUAL(CONNECTION_REFUSED, response->GetSystemErrorCode());
        {
            auto* multiHandle = dynamic_cast<TNehMultiHandle*>(handle.Get());
            UNIT_ASSERT(multiHandle);
            UNIT_ASSERT_VALUES_EQUAL(1, multiHandle->ConnectAttempsCount);
            UNIT_ASSERT_VALUES_EQUAL(1, multiHandle->AttemptsCount);
        }
    }

    Y_UNIT_TEST(ConnectionRetrySmallTimeout) {
        NHttpFetcher::TRequestOptions options;
        options.MaxConnectionAttemptMs = TDuration::MicroSeconds(1);
        options.MaxConnectionAttempts = 2;

        auto handle = NHttpFetcher::Request("http://localhost:1337/", options)->Fetch();

        const auto response = handle->Wait();
        UNIT_ASSERT_VALUES_EQUAL(CONNECTION_REFUSED, response->GetSystemErrorCode());
        {
            auto* multiHandle = dynamic_cast<TNehMultiHandle*>(handle.Get());
            UNIT_ASSERT(multiHandle);
            UNIT_ASSERT_VALUES_EQUAL(0, multiHandle->ConnectAttempsCount);
            UNIT_ASSERT_VALUES_EQUAL(1, multiHandle->AttemptsCount);
        }
    }

    Y_UNIT_TEST(WithoutConnectionRetry) {
        NHttpFetcher::TRequestOptions options;
        options.MaxConnectionAttemptMs = TDuration::Seconds(99999);

        auto handle = NHttpFetcher::Request("http://localhost:1337/", options)->Fetch();

        const auto response = handle->Wait();
        UNIT_ASSERT_VALUES_EQUAL(CONNECTION_REFUSED, response->GetSystemErrorCode());
        {
            auto* multiHandle = dynamic_cast<TNehMultiHandle*>(handle.Get());
            UNIT_ASSERT(multiHandle);
            UNIT_ASSERT_VALUES_EQUAL(0, multiHandle->ConnectAttempsCount);
            UNIT_ASSERT_VALUES_EQUAL(1, multiHandle->AttemptsCount);
        }
    }

    Y_UNIT_TEST(ConnectionRetryFastReconnect) {
        NHttpFetcher::TRequestOptions options;
        options.MaxConnectionAttemptMs = TDuration::Seconds(99999);
        options.MaxConnectionAttempts = 2;
        options.EnableFastReconnect = true;
        options.FastReconnectLimit = TDuration::Seconds(999);

        auto handle = NHttpFetcher::Request("http://localhost:1337/", options)->Fetch();

        const auto response = handle->Wait();
        UNIT_ASSERT_VALUES_EQUAL(CONNECTION_REFUSED, response->GetSystemErrorCode());
        {
            auto* multiHandle = dynamic_cast<TNehMultiHandle*>(handle.Get());
            UNIT_ASSERT(multiHandle);
            UNIT_ASSERT_VALUES_EQUAL(0, multiHandle->ConnectAttempsCount);
            UNIT_ASSERT_VALUES_EQUAL(2, multiHandle->AttemptsCount);
        }
    }

    Y_UNIT_TEST(ConnectionRetryNormalRetries) {
        NHttpFetcher::TRequestOptions options;
        options.MaxConnectionAttemptMs = TDuration::Seconds(99999);
        options.MaxConnectionAttempts = 2;
        options.MaxAttempts = 2;

        auto handle = NHttpFetcher::Request("http://localhost:1337/", options)->Fetch();

        const auto response = handle->Wait();
        UNIT_ASSERT_VALUES_EQUAL(CONNECTION_REFUSED, response->GetSystemErrorCode());
        {
            auto* multiHandle = dynamic_cast<TNehMultiHandle*>(handle.Get());
            UNIT_ASSERT(multiHandle);
            UNIT_ASSERT_VALUES_EQUAL(0, multiHandle->ConnectAttempsCount);
            UNIT_ASSERT_VALUES_EQUAL(2, multiHandle->AttemptsCount);
        }
    }
}

} // namespace NHttpFetcher
