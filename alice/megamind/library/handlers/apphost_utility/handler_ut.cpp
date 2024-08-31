#include "handler.h"

#include <alice/megamind/library/testing/mock_global_context.h>

#include <alice/library/logger/logger.h>
#include <alice/library/unittest/fake_fetcher.h>

#include <library/cpp/logger/log.h>
#include <library/cpp/neh/rpc.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/string/join.h>


using namespace testing;

namespace NAlice::NMegamind {

namespace {

class TFakeAppHostRequest : public NNeh::IRequest {
public:
    TFakeAppHostRequest(TStringBuilder& responseText)
        : IRequest()
        , Response_(responseText)
    {
    }
    TStringBuf Scheme() const override {
        return {};
    };
    TString RemoteHost() const override {
        return {};
    }
    TStringBuf Service() const override {
        return {};
    };
    TStringBuf Data() const override {
        return {};
    }
    TStringBuf RequestId() const override {
        return {};
    }
    bool Canceled() const override {
        return false;
    }
    void SendReply(NNeh::TData& data) override {
        Response_ << TString{data.begin(), data.end()};
    }

    void SendError(NNeh::IRequest::TResponseError /* err */, const TString& details = TString()) override {
        Response_ << "Error: " << details;
    }

    TString GetResponse() const {
        return Response_;
    }

private:
    TStringBuilder& Response_;
};

TRTLogClient GetRtLogClient() {
    TRTLog rtLog{};
    rtLog.SetAsync(true);
    rtLog.SetFlushPeriodSecs(1);
    rtLog.SetServiceName("megamind");
    rtLog.SetFilename("/dev/null");
    rtLog.SetFileStatCheckPeriodSecs(1);
    return TRTLogClient{std::move(rtLog)};
}

} // anonymous namespace

Y_UNIT_TEST_SUITE(TestHttpHandlers) {
    Y_UNIT_TEST(TestLogRolate) {
        TConfig config{};
        TMockGlobalContext globalCtx{};
        TMaybe<NInfra::TLogger> udpLogger;
        TMaybe<NUdpClickMetrics::TSelfBalancingClient> udpClient;

        TLog fakeLog{};
        TRTLogClient rtLogClient = GetRtLogClient();

        EXPECT_CALL(globalCtx, Config()).WillRepeatedly(ReturnRef(config));
        EXPECT_CALL(globalCtx, BaseLogger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(globalCtx, MegamindAnalyticsLog()).WillRepeatedly(ReturnRef(fakeLog));
        EXPECT_CALL(globalCtx, MegamindProactivityLog()).WillRepeatedly(ReturnRef(fakeLog));
        EXPECT_CALL(globalCtx, RTLogClient()).WillRepeatedly(ReturnRef(rtLogClient));

        TStringBuilder responseText;
        TAutoPtr<NNeh::IRequest> requestRef = new TFakeAppHostRequest{responseText};
        NImpl::UtilityHandlerHttp("/reload_logs", globalCtx, udpLogger, udpClient, requestRef);
        UNIT_ASSERT_C(responseText == "OK", TStringBuilder{} << "Failed to reload logs: " << responseText);
        requestRef.Destroy();
    }
}

} // namespace NAlice::NMegamind
