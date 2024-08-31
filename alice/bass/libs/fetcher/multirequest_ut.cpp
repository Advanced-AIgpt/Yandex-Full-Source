#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/neh_detail.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/ut_helpers/mocks.h>

#include <library/cpp/neh/rpc.h>

#include <util/datetime/base.h>
#include <util/generic/scope.h>
#include <util/generic/string.h>
#include <util/string/builder.h>

using namespace NHttpFetcher;
using namespace NTestingHelpers;
using namespace testing;

namespace {

const TStringBuf ERROR_AND_TIMEOUT_RESPONSE = "error_and_timeout response";
const TStringBuf FAST_RESPONSE = "fast response";
const TStringBuf SLOW_RESPONSE = "slow response";
const TStringBuf RETRY_RESPONSE = "retry response";

void SendReply(const NNeh::IRequestRef& r, TStringBuf data) {
    NNeh::TDataSaver saver;
    saver << data;
    r->SendReply(saver);
}

float GetAttemptTime(TStringBuf log, ui32 requestNo, ui32 attemptNo) {
    TString msgPrefix = TStringBuilder{} << "Request " << requestNo << " attempt " << attemptNo
                                         << " has won, time taken: ";
    size_t pos = log.find(msgPrefix);
    UNIT_ASSERT(pos != TString::npos);

    size_t start = pos + msgPrefix.size();
    TStringBuf value = log.Skip(start).Before('s');

    float result;
    UNIT_ASSERT(TryFromString(value, result));
    return result;
}

ui32 GetMultiHandleReqId(THandle::TRef handle) {
    UNIT_ASSERT(handle);
    const auto& multiHandle = static_cast<TNehMultiHandle&>(*handle);
    return multiHandle.GetInstanceId();
}

class TMultiRequestTest : public NUnitTest::TTestBase {
public:
    void SetUp() override {
        AtomicSet(ReqCnt, 0);
        AtomicSet(WasError, 0);
        BaseUrl = TStringBuilder() << "http://localhost:" << TPortManager().GetPort();

        Loop = NNeh::CreateLoop();
        Loop->Add(FastUrl(), [](const NNeh::IRequestRef& r) {
            Sleep(TDuration::MilliSeconds(FAST_TIME_MS));
            SendReply(r, FAST_RESPONSE);
        });
        Loop->Add(SlowUrl(), [](const NNeh::IRequestRef& r) {
            Sleep(TDuration::MilliSeconds(SLOW_TIME_MS));
            SendReply(r, SLOW_RESPONSE);
        });
        Loop->Add(RetryUrl(), [&cnt = ReqCnt](const NNeh::IRequestRef& r) {
            Sleep(TDuration::MilliSeconds(AtomicGetAndIncrement(cnt) ? FAST_TIME_MS : SLOW_TIME_MS));
            SendReply(r, RETRY_RESPONSE);
        });
        Loop->Add(ErrorAndTimeoutUrl(), [&wasError = WasError](const NNeh::IRequestRef& r) {
            if (!wasError) {
                AtomicSet(wasError, 1);
                r->SendError(NNeh::IRequest::TResponseError::BadGateway);
            } else {
                Sleep(TDuration::MilliSeconds(SLOW_TIME_MS));
                SendReply(r, ERROR_AND_TIMEOUT_RESPONSE);
            }
        });
        Loop->ForkLoop(4);
    }

    void TearDown() override {
        Loop->SyncStopFork();
        Loop.Reset();
    }

    TString FastUrl() const {
        return BaseUrl + "/fast";
    }

    TString SlowUrl() const {
        return BaseUrl + "/slow";
    }

    TString RetryUrl() const {
        return BaseUrl + "/retry";
    }

    TString ErrorAndTimeoutUrl() const {
        return BaseUrl + "/error_and_timeout";
    }

    UNIT_TEST_SUITE(TMultiRequestTest);
    UNIT_TEST(Smoke);
    UNIT_TEST(NetworkResolutionError);
    UNIT_TEST(ActivationFinishTime);
    UNIT_TEST(FirstAttemptTime);
    UNIT_TEST(SecondAttemptTime);
    UNIT_TEST(SaveLastError);
    UNIT_TEST_SUITE_END();

    void Smoke();
    void NetworkResolutionError();
    void ActivationFinishTime();
    void FirstAttemptTime();
    void SecondAttemptTime();
    void SaveLastError();

private:
    NNeh::IServicesRef Loop;
    TString BaseUrl;
    static constexpr size_t SLEEP_TIMEOUT_SEC = 2;
    static constexpr size_t SLOW_TIME_MS = 500;
    static constexpr size_t FAST_TIME_MS = 10;

    TAtomic ReqCnt;
    TAtomic WasError;
};
UNIT_TEST_SUITE_REGISTRATION(TMultiRequestTest);

void TMultiRequestTest::Smoke() {
    auto multiRequest = NHttpFetcher::WeakMultiRequest();
    UNIT_ASSERT(multiRequest);

    auto slowRequest = multiRequest->AddRequest(SlowUrl(), TRequestOptions{});
    UNIT_ASSERT(slowRequest);
    {
        auto fastRequest = multiRequest->AddRequest(FastUrl(), TRequestOptions{});
        UNIT_ASSERT(fastRequest);
        const auto response = fastRequest->Fetch()->Wait();
        UNIT_ASSERT(response);
        UNIT_ASSERT_C(!response->IsError(), response->GetErrorText());
        UNIT_ASSERT_VALUES_EQUAL(response->Data, FAST_RESPONSE);
    }

    {
        const auto response = slowRequest->Fetch()->Wait();
        UNIT_ASSERT(response);
        UNIT_ASSERT_C(!response->IsError(), response->GetErrorText());
        UNIT_ASSERT_VALUES_EQUAL(response->Data, SLOW_RESPONSE);
    }
}

void TMultiRequestTest::NetworkResolutionError() {
    auto multiRequest = NHttpFetcher::WeakMultiRequest();
    UNIT_ASSERT(multiRequest);

    auto slowRequest = multiRequest->AddRequest(SlowUrl(), TRequestOptions{});
    {
        auto badRequest = multiRequest->AddRequest("http://trololo", TRequestOptions{});
        UNIT_ASSERT(badRequest);

        const auto handle = badRequest->Fetch();
        UNIT_ASSERT(handle);

        const auto response = handle->Wait();
        UNIT_ASSERT(response);

        UNIT_ASSERT_C(response->IsError(), response->Data);
    }

    multiRequest->WaitAll(TInstant::Now() + TDuration::Seconds(1));

    {
        const auto response = slowRequest->Fetch()->Wait();
        UNIT_ASSERT(!response->IsError());
        UNIT_ASSERT_VALUES_EQUAL(response->Data, SLOW_RESPONSE);
    }
}

void TMultiRequestTest::ActivationFinishTime() {
    auto fakeLogger = MakeIntrusive<StrictMock<TMockEventLogger>>();
    TStringBuilder debugOut;
    EXPECT_CALL(*fakeLogger, Debug(_)).WillRepeatedly(Invoke([&debugOut](TStringBuf data) {
        debugOut << data << Endl;
    }));

    TRequestAPI requestAPI{fakeLogger};
    auto multiRequest = requestAPI.MultiRequest();
    UNIT_ASSERT(multiRequest);

    auto slowRequest = multiRequest->AddRequest(SlowUrl(), TRequestOptions{})->Fetch();
    auto fastRequest = multiRequest->AddRequest(FastUrl(), TRequestOptions{})->Fetch();
    UNIT_ASSERT(slowRequest && fastRequest);

    constexpr int SLEEP_TIMEOUT_SEC = 1;
    Sleep(TDuration::Seconds(SLEEP_TIMEOUT_SEC));
    multiRequest->WaitAll(TInstant::Now() + TDuration::Seconds(1));

    UNIT_ASSERT_LT(GetAttemptTime(debugOut, GetMultiHandleReqId(slowRequest), 1 /* attempt */), SLEEP_TIMEOUT_SEC);
    UNIT_ASSERT_LT(GetAttemptTime(debugOut, GetMultiHandleReqId(fastRequest), 1 /* attempt */), SLEEP_TIMEOUT_SEC);
}

void TMultiRequestTest::FirstAttemptTime() {
    auto fakeLogger = MakeIntrusive<StrictMock<TMockEventLogger>>();
    TStringBuilder debugOut;
    EXPECT_CALL(*fakeLogger, Debug(_)).WillRepeatedly(Invoke([&debugOut](TStringBuf data) {
        debugOut << data << Endl;
    }));

    TRequestAPI requestAPI{fakeLogger};
    auto multiRequest = requestAPI.MultiRequest();
    UNIT_ASSERT(multiRequest);

    TRequestOptions options;
    options.RetryPeriod = TDuration::MilliSeconds(50);
    options.MaxAttempts = 3;
    auto slowRequest = multiRequest->AddRequest(SlowUrl(), options)->Fetch();
    UNIT_ASSERT(slowRequest);

    multiRequest->WaitAll(TInstant::Now() + TDuration::Seconds(1));
    UNIT_ASSERT_LT(GetAttemptTime(debugOut, GetMultiHandleReqId(slowRequest), 1 /* attempt */), SLEEP_TIMEOUT_SEC);
}

void TMultiRequestTest::SecondAttemptTime() {
    auto fakeLogger = MakeIntrusive<StrictMock<TMockEventLogger>>();
    TStringBuilder debugOut;
    EXPECT_CALL(*fakeLogger, Debug(_)).WillRepeatedly(Invoke([&debugOut](TStringBuf data) {
        debugOut << data << Endl;
    }));

    TRequestAPI requestAPI{fakeLogger};
    auto multiRequest = requestAPI.MultiRequest();
    UNIT_ASSERT(multiRequest);

    TRequestOptions options;
    options.RetryPeriod = TDuration::MilliSeconds(50);
    options.MaxAttempts = 3;

    auto retryRequest = multiRequest->AddRequest(RetryUrl(), options)->Fetch();
    UNIT_ASSERT(retryRequest);

    multiRequest->WaitAll(TInstant::Now() + TDuration::Seconds(1));
    UNIT_ASSERT_GE(GetAttemptTime(debugOut, GetMultiHandleReqId(retryRequest), 2 /* attempt */),
                   (options.RetryPeriod.MilliSeconds() + FAST_TIME_MS) / 1000.);
}

void TMultiRequestTest::SaveLastError() {
    auto fakeLogger = MakeIntrusive<StrictMock<TMockEventLogger>>();
    EXPECT_CALL(*fakeLogger, Debug(_)).WillRepeatedly({});

    TRequestAPI requestAPI{fakeLogger};
    auto multiRequest = requestAPI.MultiRequest();
    UNIT_ASSERT(multiRequest);

    TRequestOptions options;
    options.RetryPeriod = TDuration::MilliSeconds(50);
    options.MaxAttempts = 2;
    options.Timeout = TDuration::MilliSeconds(150);

    auto errorAndTimeoutRequest = multiRequest->AddRequest(ErrorAndTimeoutUrl(), options)->Fetch();
    UNIT_ASSERT(errorAndTimeoutRequest);

    multiRequest->WaitAll(TInstant::Now() + TDuration::MilliSeconds(200));
    auto resp = errorAndTimeoutRequest->Wait();

    UNIT_ASSERT(resp->Code == 502);
}

} // namespace
