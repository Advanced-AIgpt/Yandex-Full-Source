#include <alice/cuttlefish/library/cuttlefish/megamind/client/client.h>

#include <alice/cuttlefish/library/rtlog/rtlog.h>
#include <alice/megamind/protos/speechkit/response.pb.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <library/cpp/threading/future/future.h>

#include <util/datetime/base.h>

using namespace NAlice::NCuttlefish::NAppHostServices;

const NAliceProtocol::TSessionContext DEFAULT_SESSION_CONTEXT;

namespace {

NAlice::NCuttlefish::TLogContext CreateFakeLogContext() {
    return NAlice::NCuttlefish::TLogContext(new TSelfFlushLogFrame(nullptr), nullptr);
}

class THttp200OkServer : public THttpServer::ICallBack {
private:
    class TRequest : public TRequestReplier {
    private:
        bool DoReply(const TReplyParams& params) override {
            // imitate delay, 0.1s
            Sleep(TDuration::MilliSeconds(100));

            const TStringBuf content = Server->ResponseContent;

            params.Output << "HTTP/1.1 200 Ok\r\n";
            params.Output << "Connection: close\r\n";
            params.Output << "X-Server: unit test server\r\n";
            params.Output << "Content-Length: " << content.size() << "\r\n";
            params.Output << "\r\n";
            params.Output << content;
            params.Output.Finish();

            return true;
        }

    private:
        THttp200OkServer* Server;

    public:
        TRequest(THttp200OkServer* server)
            : Server{server}
        {}
    };

    TString ResponseContent;

    friend class TRequest;

public:
    THttp200OkServer(TString responseContent = R"({"version": "1.2.3"})")
        : ResponseContent{responseContent}
    {}

    TClientRequest* CreateClient() override {
        return new TRequest(this);
    }
};

NNeh::TMessage MakeMegamindRequest(ui16 port) {
    constexpr TStringBuf content = "SomeCoolContent";
    constexpr TStringBuf headers = "Content-type: application/bullshit\r\nX-Client: unit test client\r\n";
    const TString url = TStringBuilder{} << "http://localhost:" << port << "/handle";

    NNeh::TMessage msg{url, {}};
    NNeh::NHttp::MakeFullRequest(msg, headers, content);
    return msg;
}

} // namespace

Y_UNIT_TEST_SUITE(MegaClient) {
    Y_UNIT_TEST(SingleRequest) {
        // init server
        TPortManager portManager;
        const ui16 port = portManager.GetPort();
        Cerr << "Init server using port " << port << Endl;

        THttp200OkServer serverImpl;
        THttpServer::TOptions options(port);

        THttpServer server(&serverImpl, options);
        UNIT_ASSERT(server.Start());

        // init client
        Cerr << "Init client" << Endl;
        NAliceCuttlefishConfig::TConfig config = NAlice::NCuttlefish::GetDefaultCuttlefishConfig();
        config.megamind().set_timeout(TDuration::Seconds(100));
        TMegamindClientPtr client = TMegamindClient::Create(ERequestPhase::RUN, config, DEFAULT_SESSION_CONTEXT, CreateFakeLogContext());

        // send request and wait
        NNeh::TMessage dummy = MakeMegamindRequest(port);
        Cerr << "Sending request through client" << Endl;

        TInstant start = TInstant::Now();
        auto future = client->SendRequest(std::move(dummy), 0, MakeAtomicShared<NAlice::NCuttlefish::TRTLogActivation>());
        Cerr << "Request sent, waiting for response" << Endl;

        future.Subscribe([](const NThreading::TFuture<NAliceProtocol::TMegamindResponse>& fut) {
            UNIT_ASSERT(fut.HasValue());
            UNIT_ASSERT(fut.GetValueSync().GetProtoResponse().GetVersion() == "1.2.3");
        });
        future.Wait();
        TInstant end = TInstant::Now();

        TDuration duration = end - start;
        Cerr << "Got response, duration " << duration << " (should be a bit longer than 0.1s)" << Endl;
        UNIT_ASSERT(duration >= TDuration::MilliSeconds(100));
    }

    Y_UNIT_TEST(ManyRequests) {
        // init server
        TPortManager portManager;
        const ui16 port = portManager.GetPort();
        Cerr << "Init server using port " << port << Endl;

        THttp200OkServer serverImpl;
        THttpServer::TOptions options(port);

        THttpServer server(&serverImpl, options);
        UNIT_ASSERT(server.Start());

        // init client
        Cerr << "Init client" << Endl;
        NAliceCuttlefishConfig::TConfig config = NAlice::NCuttlefish::GetDefaultCuttlefishConfig();
        config.megamind().set_timeout(TDuration::Seconds(100));
        TMegamindClientPtr client = TMegamindClient::Create(ERequestPhase::RUN, config, DEFAULT_SESSION_CONTEXT, CreateFakeLogContext());

        // send 100 requests and wait
        constexpr int REQUEST_COUNT = 100;

        TInstant startTime = TInstant::Now();

        TVector<NThreading::TFuture<NAliceProtocol::TMegamindResponse>> futures;
        NNeh::TMessage dummy = MakeMegamindRequest(port);
        for (int i = 0; i < REQUEST_COUNT; ++i) {
            Cerr << "Sending request #" << i << " through client" << Endl;
            futures.push_back(client->SendRequest(std::move(dummy), 0, MakeAtomicShared<NAlice::NCuttlefish::TRTLogActivation>()));
        }
        Cerr << REQUEST_COUNT << " requests sent, waiting for response" << Endl;

        for (auto& future : futures) {
            future.Subscribe([](const NThreading::TFuture<NAliceProtocol::TMegamindResponse>& fut) {
                UNIT_ASSERT(fut.HasValue());
                UNIT_ASSERT(fut.GetValueSync().GetProtoResponse().GetVersion() == "1.2.3");
            });
            future.Wait();
        }
        TInstant endTime = TInstant::Now();

        TDuration elapsed = endTime - startTime;
        Cerr << "Got all responses, duration " << elapsed << " (should be a bit longer than 0.1s)" << Endl;
        UNIT_ASSERT(elapsed >= TDuration::MilliSeconds(100));
        UNIT_ASSERT(elapsed <= TDuration::MilliSeconds(1000)); // 1s is more than enough!
    }

    Y_UNIT_TEST(TimeoutedRequests) {
        // init server
        TPortManager portManager;
        const ui16 port = portManager.GetPort();
        Cerr << "Init server using port " << port << Endl;

        THttp200OkServer serverImpl;
        THttpServer::TOptions options(port);

        THttpServer server(&serverImpl, options);
        UNIT_ASSERT(server.Start());

        // init client
        Cerr << "Init client" << Endl;
        NAliceCuttlefishConfig::TConfig config = NAlice::NCuttlefish::GetDefaultCuttlefishConfig();
        config.megamind().set_timeout(TDuration::MilliSeconds(10)); // smaller than delay time
        TMegamindClientPtr client = TMegamindClient::Create(ERequestPhase::RUN, config, DEFAULT_SESSION_CONTEXT, CreateFakeLogContext());

        // send 3 requests and wait
        constexpr int REQUEST_COUNT = 3;

        TInstant startTime = TInstant::Now();

        TVector<NThreading::TFuture<NAliceProtocol::TMegamindResponse>> futures;
        NNeh::TMessage dummy = MakeMegamindRequest(port);
        for (int i = 0; i < REQUEST_COUNT; ++i) {
            Cerr << "Sending request #" << i << " through client" << Endl;
            futures.push_back(client->SendRequest(std::move(dummy), 0, MakeAtomicShared<NAlice::NCuttlefish::TRTLogActivation>()));
        }
        Cerr << REQUEST_COUNT << " requests sent, waiting for response" << Endl;

        for (auto& future : futures) {
            future.Subscribe([](const NThreading::TFuture<NAliceProtocol::TMegamindResponse>& fut) {
                UNIT_ASSERT(fut.HasException());
                UNIT_ASSERT_EXCEPTION_CONTAINS(
                    fut.TryRethrow(),
                    NAlice::NCuttlefish::NAppHostServices::TErrorWithCode,
                    "(NAlice::NCuttlefish::NAppHostServices::TErrorWithCode) code=timeout: Timeout"
                );
            });
            future.Wait();
        }
        TInstant endTime = TInstant::Now();

        TDuration elapsed = endTime - startTime;
        Cerr << "Got all responses, duration " << elapsed << " (should be a bit smaller than 0.1s)" << Endl;
        UNIT_ASSERT(elapsed <= TDuration::MilliSeconds(100));
    }

    Y_UNIT_TEST(StupidMegamindAnswer) {
        // init server
        TPortManager portManager;
        const ui16 port = portManager.GetPort();
        Cerr << "Init server using port " << port << Endl;

        THttp200OkServer serverImpl("stupid non-json content");
        THttpServer::TOptions options(port);

        THttpServer server(&serverImpl, options);
        UNIT_ASSERT(server.Start());

        // init client
        Cerr << "Init client" << Endl;
        NAliceCuttlefishConfig::TConfig config = NAlice::NCuttlefish::GetDefaultCuttlefishConfig();
        config.megamind().set_timeout(TDuration::Seconds(100));
        TMegamindClientPtr client = TMegamindClient::Create(ERequestPhase::RUN, config, DEFAULT_SESSION_CONTEXT, CreateFakeLogContext());

        // send a request and wait
        TInstant startTime = TInstant::Now();

        NNeh::TMessage dummy = MakeMegamindRequest(port);
        Cerr << "Sending request through client" << Endl;
        auto fut = client->SendRequest(std::move(dummy), 0, MakeAtomicShared<NAlice::NCuttlefish::TRTLogActivation>());
        Cerr << "Request sent, waiting for response" << Endl;

        fut.Subscribe([](const NThreading::TFuture<NAliceProtocol::TMegamindResponse>& fut) {
            UNIT_ASSERT(fut.HasException());
            try {
                fut.TryRethrow();
            } catch (...) {
                UNIT_ASSERT(CurrentExceptionMessage().Contains("JSON error at offset 7 (invalid syntax at token: 'non-json')"));
            }
        });
        fut.Wait();

        TInstant endTime = TInstant::Now();

        TDuration elapsed = endTime - startTime;
        Cerr << "Got all responses, duration " << elapsed << " (should be a bit longer than 0.1s)" << Endl;
        UNIT_ASSERT(elapsed >= TDuration::MilliSeconds(100));
        UNIT_ASSERT(elapsed <= TDuration::MilliSeconds(1000)); // 1s is more than enough!
    }
}
