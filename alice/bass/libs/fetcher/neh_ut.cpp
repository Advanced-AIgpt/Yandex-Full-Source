#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/request.h>

#include <alice/library/unittest/mock_request_eventlistener.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/neh/rpc.h>
#include <library/cpp/threading/future/async.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/system/mutex.h>
#include <util/thread/pool.h>

using namespace testing;

namespace {

using TVoidPromise = NThreading::TPromise<void>;
using TVoidFuture = NThreading::TFuture<void>;

TVoidPromise NewVoidPromise() {
    return NThreading::NewPromise<void>();
}

void SendStringReply(const NNeh::IRequestRef& r, const TString& str) {
    NNeh::TDataSaver data;
    data << str;
    r->SendReply(data);
}

void AssertNoError(const NHttpFetcher::TResponse::TRef& response) {
    UNIT_ASSERT_C(response, "Empty response");
    UNIT_ASSERT_C(!response->IsError(), response->GetErrorText());
}

constexpr TDuration TIME_QUANT = TDuration::MilliSeconds(10);

struct TTestCommons : private TNonCopyable {
    const TString Addr = TStringBuilder() << "http://localhost:" << TPortManager().GetPort() << "/test_bass_fetcher";
    const TString ResponseText = "testResponse";
    NNeh::IServicesRef Loop = NNeh::CreateLoop();
    TAtomic RequestId;

    const TDuration HardTimeout = TIME_QUANT * 1000;

    TTestCommons() {
        AtomicSet(RequestId, 0);
    }

    ~TTestCommons() {
        Loop->SyncStopFork();
    }
};

} // namespace

Y_UNIT_TEST_SUITE(FetcherTestSuite) {
    Y_UNIT_TEST(TestSingle) {
        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [&tc] (const NNeh::IRequestRef& r) {
            SendStringReply(r, tc.ResponseText);
        });
        tc.Loop->ForkLoop(1);

        NHttpFetcher::TRequestOptions options;

        StrictMock<NAlice::NTestingHelpers::TMockRequestEventListener> listener;
        EXPECT_CALL(listener, OnAttempt(_)).Times(1).WillRepeatedly(DoDefault());

        NHttpFetcher::TResponse::TRef response = NHttpFetcher::Request(tc.Addr, options, &listener)->Fetch()->Wait();
        AssertNoError(response);
        UNIT_ASSERT_VALUES_EQUAL(response->Data, tc.ResponseText);
    }

    Y_UNIT_TEST(TestFirstAttemptFailButSecondOk) {
        TVoidPromise processedPromise = NewVoidPromise();
        TVoidPromise donePromise = NewVoidPromise();

        TVoidFuture processedFuture = processedPromise.GetFuture();
        TVoidFuture doneFuture = donePromise.GetFuture();

        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [&tc, &donePromise, processedFuture] (const NNeh::IRequestRef& r) {
            TAtomicBase requestId = AtomicGetAndIncrement(tc.RequestId);

            switch (requestId) {
            case 0:
                processedFuture.GetValue(tc.HardTimeout);
                break;
            case 1:
                SendStringReply(r, TStringBuilder() << tc.ResponseText << requestId);
                donePromise.SetValue();
                break;

            default:
                UNIT_FAIL("Unexpected attempt #" << requestId);
            }
        });
        tc.Loop->ForkLoop(2);

        NHttpFetcher::TRequestOptions options;
        options.RetryPeriod = TIME_QUANT;
        options.MaxAttempts = 2;

        StrictMock<NAlice::NTestingHelpers::TMockRequestEventListener> listener;
        // OnAttempt() isn't called on cancelled handles.
        EXPECT_CALL(listener, OnAttempt(_)).WillOnce(DoDefault());

        NHttpFetcher::TResponse::TRef response = NHttpFetcher::Request(tc.Addr, options, &listener)->Fetch()->Wait();
        AssertNoError(response);
        UNIT_ASSERT_VALUES_EQUAL(response->Data, TStringBuilder() << tc.ResponseText << 1);

        processedPromise.SetValue();
        doneFuture.GetValue(tc.HardTimeout);
    }

    const TString REPLY_SUCCESS = "replySuccess";
    const TString REPLY_ERROR = "replyError";
    const TString REPLY_FAILURE = "replyFailure";

    class TStatusCallback : public NHttpFetcher::IStatusCallback {
    public:
        NHttpFetcher::TFetchStatus OnResponse(const NHttpFetcher::TResponse& response) override {
            if (response.IsError()) {
                if (IsUserError(response.Code)) {
                    return NHttpFetcher::TFetchStatus::Failure();
                }
                return NHttpFetcher::TFetchStatus::Error();
            }

            if (response.Data == REPLY_SUCCESS) {
                return NHttpFetcher::TFetchStatus::Success();
            }
            if (response.Data == REPLY_ERROR) {
                return NHttpFetcher::TFetchStatus::Error();
            }
            if (response.Data == REPLY_FAILURE) {
                return NHttpFetcher::TFetchStatus::Failure();
            }
            UNIT_FAIL("Unreachable point, got unknown answer " << response.Data);
            return NHttpFetcher::TFetchStatus::Failure();
        }
    };

    Y_UNIT_TEST(TestStatusCallback_ErrorSuccess) {
        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [&tc] (const NNeh::IRequestRef& r) {
            TAtomicBase requestId = AtomicGetAndIncrement(tc.RequestId);

            switch (requestId) {
            case 0:
                SendStringReply(r, REPLY_ERROR);
                break;
            case 1:
                SendStringReply(r, REPLY_SUCCESS);
                break;

            default:
                UNIT_FAIL("Unexpected attempt #" << requestId);
            }
        });
        tc.Loop->ForkLoop(2);

        NHttpFetcher::TRequestOptions options;
        options.RetryPeriod = TIME_QUANT;
        options.MaxAttempts = 2;
        options.StatusCallback = new TStatusCallback();

        StrictMock<NAlice::NTestingHelpers::TMockRequestEventListener> listener;
        // Both attempts are replies so the callback call counter is 2.
        EXPECT_CALL(listener, OnAttempt(_)).Times(2).WillRepeatedly(DoDefault());

        NHttpFetcher::TResponse::TRef response = NHttpFetcher::Request(tc.Addr, options, &listener)->Fetch()->Wait();
        AssertNoError(response);
        UNIT_ASSERT_VALUES_EQUAL(response->Data, REPLY_SUCCESS);
    }

#if 0
    Y_UNIT_TEST(TestStatusCallback_FailureSuccess_NoRetry) {
        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [&tc] (const NNeh::IRequestRef& r) {
            TAtomicBase requestId = AtomicGetAndIncrement(tc.RequestId);

            switch (requestId) {
            case 0:
                SendStringReply(r, REPLY_FAILURE);
                break;
            case 1:
                SendStringReply(r, REPLY_SUCCESS);
                break;

            default:
                UNIT_FAIL("Unexpected attempt #" << requestId);
            }
        });
        tc.Loop->ForkLoop(2);

        NHttpFetcher::TRequestOptions options;
        options.RetryPeriod = TIME_QUANT;
        options.MaxAttempts = 2;
        options.StatusCallback = new TStatusCallback();

        NHttpFetcher::TResponse::TRef response = NHttpFetcher::Request(tc.Addr, options)->Fetch()->Wait();
        UNIT_ASSERT(response->IsError());
        UNIT_ASSERT_VALUES_EQUAL(response->Data, REPLY_FAILURE);
    }
#endif

    Y_UNIT_TEST(TestReceiveFirstAttemptEvenAfterRetry) {
        const TDuration retry = TIME_QUANT;
        const TDuration serverLatency = TIME_QUANT * 3 / 2;

        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [&tc, serverLatency] (const NNeh::IRequestRef& r) {
            TAtomicBase requestId = AtomicGetAndIncrement(tc.RequestId);

            Sleep(serverLatency);
            SendStringReply(r, TStringBuilder() << tc.ResponseText << requestId);
        });
        tc.Loop->ForkLoop(1);

        NHttpFetcher::TRequestOptions options;
        options.RetryPeriod = retry;
        options.MaxAttempts = 2;

        StrictMock<NAlice::NTestingHelpers::TMockRequestEventListener> listener;
        // The second attempt is cancelled.
        EXPECT_CALL(listener, OnAttempt(_)).WillOnce(DoDefault());

        NHttpFetcher::TResponse::TRef response = NHttpFetcher::Request(tc.Addr, options, &listener)->Fetch()->Wait();
        AssertNoError(response);
        UNIT_ASSERT_VALUES_EQUAL(response->Data, TStringBuilder() << tc.ResponseText << 0);
    }

    Y_UNIT_TEST(TestDontWaitAfterSuccess) {
        /**
         * 1. Client sends first and seconds attempt
         * 2. Server responses both attempts after receiving the second one
         * 3. We check that we didn't reach the timeout (which is INF so test will timeout)
         * 4. We check that we got the first response, not the second one
         */
        NThreading::TPromise<const NNeh::IRequestRef*> firstP = NThreading::NewPromise<const NNeh::IRequestRef*>();
        NThreading::TFuture<const NNeh::IRequestRef*> firstF = firstP.GetFuture();
        NThreading::TPromise<void> gotSecondPromise = NThreading::NewPromise<void>();
        NThreading::TPromise<void> receivedResponsePromise = NThreading::NewPromise<void>();

        NThreading::TFuture<void> gotSecondFuture = gotSecondPromise.GetFuture();
        NThreading::TFuture<void> receivedResponseFuture = receivedResponsePromise.GetFuture();

        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [&tc, &firstP, firstF, &gotSecondPromise, gotSecondFuture, receivedResponseFuture] (const NNeh::IRequestRef& r) {
            TAtomicBase requestId = AtomicGetAndIncrement(tc.RequestId);

            switch (requestId) {
            case 0:
                firstP.SetValue(&r);
                gotSecondFuture.Wait(tc.HardTimeout);
                break;

            case 1: {
                const NNeh::IRequestRef* firstRequest = firstF.GetValue(TDuration::Max());

                SendStringReply(*firstRequest, TStringBuilder() << tc.ResponseText << 0);
                gotSecondPromise.SetValue();

                receivedResponseFuture.Wait(tc.HardTimeout);
                SendStringReply(r, TStringBuilder() << tc.ResponseText << 1);
                break;
            }
            default:
                UNIT_FAIL("Unexpected attempt #" << requestId);
            };
        });
        tc.Loop->ForkLoop(2);

        NHttpFetcher::TRequestOptions options;
        options.RetryPeriod = TIME_QUANT;
        options.MaxAttempts = 2;
        options.Timeout = tc.HardTimeout;

        StrictMock<NAlice::NTestingHelpers::TMockRequestEventListener> listener;
        // The second attempt is cancelled.
        EXPECT_CALL(listener, OnAttempt(_)).WillOnce(DoDefault());

        NHttpFetcher::TResponse::TRef response = NHttpFetcher::Request(tc.Addr, options, &listener)->Fetch()->Wait();
        AssertNoError(response);
        UNIT_ASSERT_VALUES_EQUAL(response->Data, TStringBuilder() << tc.ResponseText << 0);

        receivedResponsePromise.SetValue();
    }

    Y_UNIT_TEST(TestReturnAfterFirstSuccess) {
        /**
         * 1. Client sends first and seconds attempt
         * 2. Server responses both attempts after receiving the second one
         * 3. We check that we didn't reach the timeout (which is INF so test will timeout)
         * 4. We check that we got response even if second attempt did not response
         *
         * The set of futures below provides a guarantee that all events are happening in expected order, throwing an error after hardTimeout otherwise
         */
        NThreading::TPromise<const NNeh::IRequestRef*> firstP = NThreading::NewPromise<const NNeh::IRequestRef*>();
        NThreading::TFuture<const NNeh::IRequestRef*> firstF = firstP.GetFuture();

        NThreading::TPromise<void> gotSecondP = NThreading::NewPromise<void>();
        NThreading::TFuture<void> gotSecondF = gotSecondP.GetFuture();

        NThreading::TPromise<void> gotResponseP = NThreading::NewPromise<void>();
        NThreading::TFuture<void> gotResponseF = gotResponseP.GetFuture();

        NThreading::TPromise<void> loopExited = NThreading::NewPromise<void>();

        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [&tc, &firstP, firstF, &gotSecondP, gotSecondF, gotResponseF, &loopExited] (const NNeh::IRequestRef& r) {
            TAtomicBase requestId = AtomicGetAndIncrement(tc.RequestId);

            switch (requestId) {
            case 0:
                firstP.SetValue(&r);
                gotSecondF.Wait();
                break;

            case 1: {
                const NNeh::IRequestRef* firstRequest = firstF.GetValue(tc.HardTimeout);

                SendStringReply(*firstRequest, TStringBuilder() << tc.ResponseText << 0);
                gotSecondP.SetValue();
                gotResponseF.GetValue(tc.HardTimeout);
                loopExited.SetValue();
                break;
            }
            default:
                ythrow yexception() << "Unexpected attempt " << requestId;
            };
        });
        tc.Loop->ForkLoop(2);

        NHttpFetcher::TRequestOptions options;
        options.RetryPeriod = TIME_QUANT;
        options.MaxAttempts = 2;
        options.Timeout = tc.HardTimeout;

        StrictMock<NAlice::NTestingHelpers::TMockRequestEventListener> listener;
        // The first or second attempt can be cancelled but one attempt should win.
        EXPECT_CALL(listener, OnAttempt(_)).Times(AtLeast(1)).WillRepeatedly(DoDefault());

        NHttpFetcher::TResponse::TRef response = NHttpFetcher::Request(tc.Addr, options, &listener)->Fetch()->Wait();
        AssertNoError(response);
        UNIT_ASSERT_VALUES_EQUAL(response->Data, TStringBuilder() << tc.ResponseText << 0);

        gotResponseP.SetValue();
        loopExited.GetFuture().Wait(tc.HardTimeout);
    }

    struct TMetrics : public NHttpFetcher::IMetrics {
        TAtomic Total;
        TAtomic TotalHedged;
        TAtomic Errors;
        TAtomic Failures;
        TAtomic Success;
        TAtomic SuccessHedged;

        void OnStartRequest() override {
            AtomicIncrement(Total);
        }

        void OnFinishRequest(const NHttpFetcher::TResponseStats& stats) override {
            const auto& status = stats.Status;
            if (status.IsSuccess()) {
                AtomicIncrement(Success);
            } else if (status.IsFailure()){
                AtomicIncrement(Failures);
            } else {
                UNIT_FAIL("Status must be Success or Failure");
            }
        }

        void OnStartHedgedRequest(const NHttpFetcher::TRequestStats& /* stats */) override {
            AtomicIncrement(TotalHedged);
        }

        void OnFinishHedgedRequest(const NHttpFetcher::TResponseStats& stats) override {
            const auto& status = stats.Status;
            if (status.IsSuccess()) {
                AtomicIncrement(SuccessHedged);
            } else if (status.IsError()){
                AtomicIncrement(Errors);
            }
        }
    };

    Y_UNIT_TEST(TestFirstOfThreeRequests) {
        /**
         * 1. Client sends hree attempts
         * 2. Server responses the first attempt only after the last one so we check that all retries are made by handle properly
         */
        NThreading::TPromise<const NNeh::IRequestRef*> firstP = NThreading::NewPromise<const NNeh::IRequestRef*>();
        NThreading::TFuture<const NNeh::IRequestRef*> firstF = firstP.GetFuture();

        NThreading::TPromise<void> gotSecondP = NThreading::NewPromise<void>();
        NThreading::TFuture<void> gotSecondF = gotSecondP.GetFuture();

        NThreading::TPromise<void> gotThirdP = NThreading::NewPromise<void>();
        NThreading::TFuture<void> gotThirdF = gotThirdP.GetFuture();

        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [&tc, &firstP, firstF, &gotSecondP, gotSecondF, &gotThirdP, gotThirdF] (const NNeh::IRequestRef& r) {
            TAtomicBase requestId = AtomicGetAndIncrement(tc.RequestId);

            switch (requestId) {
            case 0:
                firstP.SetValue(&r);
                gotSecondF.Wait(tc.HardTimeout);
                gotThirdF.Wait(tc.HardTimeout);
                break;

            case 1:
                firstF.Wait(tc.HardTimeout);
                gotSecondP.SetValue();
                break;

            case 2: {
                firstF.Wait(tc.HardTimeout);
                gotSecondF.Wait(tc.HardTimeout);

                const NNeh::IRequestRef* firstRequest = firstP.GetFuture().GetValue();
                SendStringReply(*firstRequest, TStringBuilder() << tc.ResponseText << 0);

                gotThirdP.SetValue();
                break;
            }
            default:
                UNIT_FAIL("Unexpected attempt #" << requestId);
            };
        });
        tc.Loop->ForkLoop(3);

        TIntrusivePtr<TMetrics> metrics = MakeIntrusive<TMetrics>();

        NHttpFetcher::TRequestOptions options;
        options.RetryPeriod = TIME_QUANT;
        options.MaxAttempts = 3;
        options.Metrics = metrics;

        StrictMock<NAlice::NTestingHelpers::TMockRequestEventListener> listener;
        // Some attempts can be cancelled but one attempt should win and trigger the callback.
        EXPECT_CALL(listener, OnAttempt(_)).Times(AtLeast(1)).WillRepeatedly(DoDefault());

        NHttpFetcher::TResponse::TRef response = NHttpFetcher::Request(tc.Addr, options, &listener)->Fetch()->Wait();
        AssertNoError(response);
        UNIT_ASSERT_VALUES_EQUAL(response->Data, TStringBuilder() << tc.ResponseText << 0);

        firstF.Wait(tc.HardTimeout);
        gotSecondF.Wait(tc.HardTimeout);
        gotThirdF.Wait(tc.HardTimeout);

        UNIT_ASSERT_VALUES_EQUAL(metrics->Success, 1);
        // UNIT_ASSERT_VALUES_EQUAL(metrics->Errors, 2); Currently number of errors is not defined in this case
        UNIT_ASSERT_VALUES_EQUAL(metrics->Failures, 0);
        UNIT_ASSERT_VALUES_EQUAL(metrics->TotalHedged, 3);
        UNIT_ASSERT_VALUES_EQUAL(metrics->SuccessHedged, 1);
    }

    class TBigTest {
    protected:
        static constexpr TDuration Retry = TIME_QUANT;

        static constexpr ui32 ClientThreadsCount = 4;
        static constexpr ui32 ServerThreadsCount = 8;
        static constexpr ui32 RequestsCountPerThread = 1000;
        static constexpr ui32 RequestsCount = RequestsCountPerThread * ClientThreadsCount;

        const TString RequestIdParam = "request_id";
        TTestCommons tc;

    public:
        virtual ~TBigTest() = default;
        virtual void ServeRequest(TAtomicBase requestId, const NNeh::IRequestRef& r) = 0;
        virtual NHttpFetcher::TRequestPtr CreateRequest(TAtomicBase requestId) = 0;
        virtual void ValidateResponse(TAtomicBase /*requestId*/, const NHttpFetcher::TResponse::TRef& /*response*/) {}
        virtual void ValidateAfterRun() {}

        void Run() {
            TVector<TString> texts(RequestsCount);
            TVector<NHttpFetcher::TResponse::TRef> responses(RequestsCount);

            tc.Loop->Add(tc.Addr, [this] (const NNeh::IRequestRef& r) {
                try {
                    TCgiParameters cgis(r->Data());

                    const ui32 requestId = FromString<ui32>(cgis.Get(RequestIdParam));
                    ServeRequest(requestId, r);
                } catch (yexception& e) {
                    Y_FAIL("Server thread failed");
                }
            });
            tc.Loop->ForkLoop(ServerThreadsCount);

            TAdaptiveThreadPool queue;
            queue.Start(1, 1);

            using TVoidFuture = NThreading::TFuture<void>;
            TVector<TVoidFuture> futures;

            for (ui32 i = 0; i < ClientThreadsCount; ++i) {
                futures.push_back(NThreading::Async([this, i, &responses] () {
                    for (ui32 j = 0; j < RequestsCountPerThread; ++j) {
                        ui32 index = i * RequestsCountPerThread + j;

                        NHttpFetcher::TRequestPtr request = CreateRequest(index);
                        request->AddCgiParam(RequestIdParam, ToString(index));
                        responses[index] = request->Fetch()->Wait();
                    }
                }, queue));
            }
            NThreading::WaitExceptionOrAll(futures).GetValue(TDuration::Max());

            for (ui32 i = 0; i < RequestsCount; ++i) {
                ValidateResponse(i, responses[i]);
            }
            ValidateAfterRun();
        }
    };

    class TManyRequestsWithGoodResponseBigTest : public TBigTest {
    private:
        NHttpFetcher::TRequestOptions Options;

        TMutex Mutex;
        THashSet<ui32> OnceFailedRequests;

    public:
        TManyRequestsWithGoodResponseBigTest() {
            Options.RetryPeriod = TIME_QUANT;
            Options.MaxAttempts = 2;
        }

        void ServeRequest(TAtomicBase requestId, const NNeh::IRequestRef& r) override {
            constexpr ui32 requestsFailureSeed = Max<ui32>(TBigTest::RequestsCountPerThread / 5, 10);
            constexpr TDuration serverLatency = Max(TIME_QUANT / 10, TDuration::MilliSeconds(1));

            Sleep(serverLatency);

            if (false && requestId % requestsFailureSeed == requestId / requestsFailureSeed) { // fail some requests from different threads (just for fun)
                auto guard = Guard(Mutex);
                if (!OnceFailedRequests.contains(requestId)) {
                    OnceFailedRequests.insert(requestId);
                    return;
                }
            }
            SendStringReply(r, TString{r->Data()});
        }

        NHttpFetcher::TRequestPtr CreateRequest(TAtomicBase requestId) override {
            NHttpFetcher::TRequestPtr request = NHttpFetcher::Request(tc.Addr, Options);
            request->AddCgiParam(tc.ResponseText, ToString(requestId));
            return request;
        }

        void ValidateResponse(TAtomicBase requestId, const NHttpFetcher::TResponse::TRef& response) override {
            AssertNoError(response);
            TCgiParameters cgi(response->Data);

            UNIT_ASSERT_VALUES_EQUAL(cgi.Get(tc.ResponseText), ToString(requestId));
        }
    };

    Y_UNIT_TEST(TestManyRequestsWithGoodResponse) {
        TManyRequestsWithGoodResponseBigTest test;
        test.Run();
    }

    class TManyErrorsBigTest : public TBigTest {
    private:
        NHttpFetcher::TRequestOptions Options;
        TAtomic RequestsReceived;

        const ui32 GoodRequestIndex;
        const TString ErrorText = "Sorry man, that's just a test";

    public:
        TManyErrorsBigTest()
            : GoodRequestIndex(TBigTest::RequestsCount / 2)
        {
            Options.RetryPeriod = TIME_QUANT * 10;
            Options.MaxAttempts = 3;
            AtomicSet(RequestsReceived, 0);
        }

        void ServeRequest(TAtomicBase requestId, const NNeh::IRequestRef& r) override {
            AtomicIncrement(RequestsReceived);
            if (requestId == GoodRequestIndex) {
                SendStringReply(r, TString{r->Data()});
            } else {
                r->SendError(NNeh::IRequest::BadRequest, ErrorText);
            }
        }

        NHttpFetcher::TRequestPtr CreateRequest(TAtomicBase requestId) override {
            NHttpFetcher::TRequestPtr request = NHttpFetcher::Request(tc.Addr, Options);
            request->AddCgiParam(tc.ResponseText, ToString(requestId));
            return request;
        }

        void ValidateResponse(TAtomicBase requestId, const NHttpFetcher::TResponse::TRef& response) override {
            if (requestId == GoodRequestIndex) {
                AssertNoError(response);
                UNIT_ASSERT_VALUES_EQUAL(TCgiParameters(response->Data).Get(tc.ResponseText), ToString(requestId));
            } else {
                UNIT_ASSERT_VALUES_EQUAL((ui32)response->Result, (ui32)NHttpFetcher::TResponse::EResult::HttpError);
                UNIT_ASSERT(response->GetErrorText().find(ErrorText) != TString::npos);
            }
        }

        void ValidateAfterRun() override {
            ui64 actual = AtomicGet(RequestsReceived);
            ui64 expected = (TBigTest::RequestsCount - 1) * Options.MaxAttempts + 1; // one request is success without retries

            if (actual < expected) {
                UNIT_FAIL("Too few requests: (" << actual << " != " << expected << ')');
            }
            if (actual == expected + 1) {
                UNIT_FAIL("Too many requests: one unexpected retry, probably too sensitive timings, consider options tuning: (" << actual << " != " << expected << ')');
            }
            if (actual == expected + 2) {
                UNIT_FAIL("Too many requests: two unexpected retries, probably too sensitive timings, consider options tuning: (" << actual << " != " << expected << ')');
            }
            if (actual > expected + 2) {
                UNIT_FAIL("Too many requests: many unexpected retries, probably an error! (" << actual << " != " << expected << ')');
            }
            UNIT_ASSERT_VALUES_EQUAL(actual, expected);
        }
    };

#if 0 //TODO: investigate flacky test
    Y_UNIT_TEST(TestManyErrors) {
        TManyErrorsBigTest test;
        test.Run();
    }
#endif

    Y_UNIT_TEST(TestMultiRequestSimple) {
        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [] (const NNeh::IRequestRef& r) {
            NNeh::TDataSaver data;
            data << r->Data();
            r->SendReply(data);
        });
        tc.Loop->ForkLoop(1);

        NHttpFetcher::TRequestOptions options;
        options.RetryPeriod = TIME_QUANT;
        options.MaxAttempts = 1;

        NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
        NHttpFetcher::THandle::TRef handle1 = multiRequest->AddRequest(tc.Addr, options)->AddCgiParam("id", "1").Fetch();
        NHttpFetcher::THandle::TRef handle2 = multiRequest->AddRequest(tc.Addr, options)->AddCgiParam("id", "2").Fetch();

        multiRequest->WaitAll(tc.HardTimeout.ToDeadLine());

        NHttpFetcher::TResponse::TRef response1 = handle1->Wait();
        NHttpFetcher::TResponse::TRef response2 = handle2->Wait();
        AssertNoError(response1);
        AssertNoError(response2);
        UNIT_ASSERT_VALUES_EQUAL(response1->Data, "id=1");
        UNIT_ASSERT_VALUES_EQUAL(response2->Data, "id=2");
    }

    Y_UNIT_TEST(TestMultiRequestWithRetry) {
        const ui32 requestsNum = 10;
        const ui32 maxAttempts = 3;
        TVoidPromise shutdownP = NThreading::NewPromise<void>();
        TVoidFuture shutdownF = shutdownP.GetFuture();

        TVoidPromise exitedP = NThreading::NewPromise<void>();
        TVoidFuture exitedF = exitedP.GetFuture();

        TAtomic count;
        AtomicSet(count, 0);

        THashMap<ui32, TAtomic> reqid2attempt;
        for (ui32 i = 0; i < requestsNum; ++i) {
            AtomicSet(reqid2attempt[i], 0);
        }

        TAtomic inflight;
        AtomicSet(inflight, 0);

        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [&tc, &reqid2attempt, &count, shutdownF, &inflight] (const NNeh::IRequestRef& r) {
            AtomicIncrement(inflight);
            AtomicIncrement(count);

            const TCgiParameters cgis(r->Data());
            const ui32 id = FromString<ui32>(cgis.Get("id"));

            if (AtomicGetAndIncrement(reqid2attempt[id]) < maxAttempts - 1) {
                shutdownF.Wait(tc.HardTimeout);
            } else {
                NNeh::TDataSaver data;
                data << r->Data();
                r->SendReply(data);
            }
            AtomicDecrement(inflight);
        });
        tc.Loop->ForkLoop(requestsNum * maxAttempts);

        NHttpFetcher::TRequestOptions options;
        options.RetryPeriod = TIME_QUANT;
        options.MaxAttempts = maxAttempts;

        NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
        TVector<NHttpFetcher::THandle::TRef> handles;
        for (ui32 i = 0; i < requestsNum; ++i) {
            handles.push_back(multiRequest->AddRequest(tc.Addr, options)->AddCgiParam("id", ToString(i)).Fetch());
        }
        multiRequest->WaitAll(tc.HardTimeout.ToDeadLine());

        for (ui32 i = 0; i < requestsNum; ++i) {
            NHttpFetcher::TResponse::TRef response = handles[i]->Wait();
            AssertNoError(response);
            UNIT_ASSERT_VALUES_EQUAL(response->Data, TStringBuilder() << "id=" << i);
        }
        UNIT_ASSERT_VALUES_EQUAL(AtomicGet(count), requestsNum * maxAttempts);
        shutdownP.SetValue();

        while (AtomicGet(inflight) > 0) {
            Sleep(TDuration::MilliSeconds(10));
        }
    }

    class TFailOnErrorCallback : public NHttpFetcher::IStatusCallback {
    public:
        NHttpFetcher::TFetchStatus OnResponse(const NHttpFetcher::TResponse& response) override {
            if (response.IsError()) {
                return NHttpFetcher::TFetchStatus::Failure();
            }
            return NHttpFetcher::TFetchStatus::Success();
        }
    };

    Y_UNIT_TEST(TestMultiRequestWithFailure) {
        TAtomic req2;
        AtomicSet(req2, 0);

        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [&req2] (const NNeh::IRequestRef& r) {
            const TCgiParameters cgis(r->Data());
            const ui32 id = FromString<ui32>(cgis.Get("id"));

            if (id == 1) {
                r->SendError(NNeh::IRequest::TResponseError::BadRequest); // Will cause whole multiRequest failure
                return;
            }

            UNIT_ASSERT_VALUES_EQUAL(id, 2);
            if (AtomicGetAndIncrement(req2) == 0) {
                r->SendError(NNeh::IRequest::TResponseError::ServiceUnavailable); // Will schedule a retry (that should not happen due to failure)
                return;
            }
        });
        tc.Loop->ForkLoop(4);

        NHttpFetcher::TRequestOptions options1;
        options1.RetryPeriod = TIME_QUANT * 100;
        options1.MaxAttempts = 2;
        options1.StatusCallback = new TFailOnErrorCallback();
        options1.IsRequired = true;
        NHttpFetcher::TRequestOptions options2;
        options2.RetryPeriod = TIME_QUANT * 100;
        options2.MaxAttempts = 2;

        NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
        NHttpFetcher::THandle::TRef handle1 = multiRequest->AddRequest(tc.Addr, options1)->AddCgiParam("id", "1").Fetch();
        NHttpFetcher::THandle::TRef handle2 = multiRequest->AddRequest(tc.Addr, options2)->AddCgiParam("id", "2").Fetch();

        TAdaptiveThreadPool queue;
        queue.Start(0);
        NThreading::TFuture<void> future = NThreading::Async([&multiRequest] () {
            multiRequest->WaitAll();
        }, queue);
        future.Wait(tc.HardTimeout);
        UNIT_ASSERT_C(future.HasValue(), "WaitAll() timed out");
        future.GetValue();

        NHttpFetcher::TResponse::TRef response1 = handle1->Wait();
        NHttpFetcher::TResponse::TRef response2 = handle2->Wait();
        UNIT_ASSERT(response1->IsError());
        UNIT_ASSERT(response2->IsError());
    }

    Y_UNIT_TEST(TestMultiRequestDoubleWait) {
        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [] (const NNeh::IRequestRef& r) {
            NNeh::TDataSaver data;
            data << r->Data();
            r->SendReply(data);
        });
        tc.Loop->ForkLoop(1);

        NHttpFetcher::TRequestOptions options;
        options.RetryPeriod = TIME_QUANT;
        options.MaxAttempts = 2;

        NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
        NHttpFetcher::THandle::TRef handle1 = multiRequest->AddRequest(tc.Addr, options)->AddCgiParam("id", "1").Fetch();
        NHttpFetcher::THandle::TRef handle2 = multiRequest->AddRequest(tc.Addr, options)->AddCgiParam("id", "2").Fetch();

        TAdaptiveThreadPool queue;
        queue.Start(0);
        NThreading::TFuture<void> future = NThreading::Async([&multiRequest] () {
            multiRequest->WaitAll();
            multiRequest->WaitAll();
        }, queue);
        future.Wait(tc.HardTimeout);
        UNIT_ASSERT_C(future.HasValue(), "Double WaitAll() timed out");
        future.GetValueSync();

        NHttpFetcher::TResponse::TRef response1 = handle1->Wait();
        NHttpFetcher::TResponse::TRef response2 = handle2->Wait();
        AssertNoError(response1);
        AssertNoError(response2);
        UNIT_ASSERT_VALUES_EQUAL(response1->Data, "id=1");
        UNIT_ASSERT_VALUES_EQUAL(response2->Data, "id=2");
    }

    Y_UNIT_TEST(TestMultiRequestWithTimeout) {
        TVoidPromise donePromise = NThreading::NewPromise<void>();
        TVoidFuture doneFuture = donePromise.GetFuture();

        TAtomic inflight;
        AtomicSet(inflight, 0);

        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [doneFuture, &inflight] (const NNeh::IRequestRef& r) {
            AtomicIncrement(inflight);
            doneFuture.GetValueSync();
            NNeh::TDataSaver data;
            data << "reply";
            r->SendReply(data);
            AtomicDecrement(inflight);
        });
        tc.Loop->ForkLoop(30);

        NHttpFetcher::TRequestOptions options;
        options.RetryPeriod = TIME_QUANT;
        options.Timeout = TIME_QUANT * 5;
        options.MaxAttempts = 3;

        NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
        NHttpFetcher::THandle::TRef handle1 = multiRequest->AddRequest(tc.Addr, options)->AddCgiParam("id", "1").Fetch();
        NHttpFetcher::THandle::TRef handle2 = multiRequest->AddRequest(tc.Addr, options)->AddCgiParam("id", "2").Fetch();

        NHttpFetcher::TResponse::TRef r1;
        NHttpFetcher::TResponse::TRef r2;

        TAdaptiveThreadPool queue;
        queue.Start(0);
        NThreading::TFuture<void> future = NThreading::Async([&multiRequest, &r1, &r2, &handle1, &handle2] () {
            multiRequest->WaitAll();

            r1 = handle1->Wait();
            r2 = handle2->Wait();
        }, queue);
        future.Wait(tc.HardTimeout);
        UNIT_ASSERT_C(future.HasValue(), "Double WaitAll() timed out");
        future.GetValueSync();

        UNIT_ASSERT_C(r1->IsTimeout(), "Timeout expected!");
        UNIT_ASSERT_C(r2->IsTimeout(), "Timeout expected!");
        donePromise.SetValue();

        while (AtomicGet(inflight) > 0) {
            Sleep(TDuration::MilliSeconds(10));
        }
    }

    Y_UNIT_TEST(TestMultiRequestCancelOnTimeout) {
        NThreading::TPromise<void> done = NThreading::NewPromise<void>();
        NThreading::TFuture<void> doneF = done.GetFuture();

        TAtomic inflight;
        AtomicSet(inflight, 0);

        TTestCommons tc;
        tc.Loop->Add(tc.Addr, [doneF, &inflight] (const NNeh::IRequestRef& r) {
            AtomicIncrement(inflight);
            NNeh::TDataSaver data;
            data << r->Data();
            doneF.GetValueSync();
            r->SendReply(data);
            AtomicDecrement(inflight);
        });
        tc.Loop->ForkLoop(1);

        NHttpFetcher::TRequestOptions options;
        options.Timeout = TIME_QUANT;
        options.MaxAttempts = 1;

        NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
        NHttpFetcher::THandle::TRef handle1 = multiRequest->AddRequest(tc.Addr, options)->AddCgiParam("id", "1").Fetch();
        NHttpFetcher::THandle::TRef handle2 = multiRequest->AddRequest(tc.Addr, options)->AddCgiParam("id", "2").Fetch();

        multiRequest->WaitAll(tc.HardTimeout.ToDeadLine());

        NHttpFetcher::TResponse::TRef response1 = handle1->Wait();
        NHttpFetcher::TResponse::TRef response2 = handle2->Wait();

        done.SetValue();

        UNIT_ASSERT(response1->IsTimeout());
        UNIT_ASSERT(response2->IsTimeout());

        while (AtomicGet(inflight) > 0) {
            Sleep(TDuration::MilliSeconds(10));
        }
    }
};
