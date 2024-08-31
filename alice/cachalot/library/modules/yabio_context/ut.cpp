#include <alice/cachalot/library/cachalot.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/service.h>
#include <alice/cachalot/library/modules/cache/service.h>
#include <alice/cachalot/library/modules/stats/service.h>
#include <alice/cachalot/library/modules/megamind_session/service.h>
#include <alice/cachalot/library/modules/yabio_context/service.h>

#include <alice/cachalot/library/config/application.cfgproto.pb.h>
#include <alice/cuttlefish/library/cachalot_client/cachalot_client.h>
#include <alice/cuttlefish/library/cachalot_client/http_client/cachalot_client.h>
#include <alice/cachalot/events/cachalot.ev.pb.h>

#include <voicetech/library/proto_api/yabio.pb.h>

#include <apphost/api/service/cpp/service.h>
#include <apphost/api/service/cpp/service_loop.h>

#include <library/cpp/getoptpb/getoptpb.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/http2.h>
#include <library/cpp/neh/tcp2.h>

using namespace NAlice;
using namespace NCachalot;
using namespace NCachalot::NApplication;

namespace {
    struct TCachalotLoop: public NAppHost::TLoop {
        TCachalotLoop(const NCachalot::TApplicationSettings& settings) {
            ui16 grpcPort = settings.Server().GrpcPort();
            Cout << "GRPC port=" << grpcPort << Endl;
            EnableGrpc(grpcPort, true, TDuration::Seconds(3));

            Cout << "GRPC threads=" << settings.Server().GrpcThreads() << Endl;
            SetGrpcThreadCount(settings.Server().GrpcThreads());
            SetToolsThreadCount(1);
        }
    };

    class TCachalotExecutor: public TCachalotLoop {
    public:
        TCachalotExecutor(NCachalot::TApplicationSettings& settings)
            : TCachalotLoop(settings)
            , Services(*this, settings.Server().Port(), settings)
        {
        }
        static std::unique_ptr<TCachalotExecutor> Run(NCachalot::TApplicationSettings& settings) {
            NAlice::NCuttlefish::TLogger::TConfig config;
            config.Filename = settings.Log().Filename();
            NAlice::NCuttlefish::GetLogger().Init(config);

            const ui16 basePort = settings.Server().Port();
            const ui16 portRange = 1000;
            ui16 maxPort = 0xFFF0;
            if (basePort < (maxPort - portRange)) {
                maxPort = basePort + portRange;
            }

            for (ui16 port = settings.Server().Port(); port < maxPort; port += 2) {
                settings.Server().SetPort(port);
                settings.Server().SetGrpcPort(port + 1);
                Cout << "Try start cachalot on port=" << port << Endl;
                std::unique_ptr<TCachalotExecutor> cachalot(new TCachalotExecutor(settings));
                try {
                    Cout << "Starting io loop..." << Endl;
                    cachalot->ForkLoop(settings.Server().Threads(), "cachalot");
                    Cout << "Cachalot started on port=" << port << Endl;
                    return std::move(cachalot);
                } catch (...) {
                    Cerr << "Starting cachalot failed: " << CurrentExceptionMessage() << Endl;
                }
            }
            throw yexception() << "Can not bind any port from range=[" << basePort << ".." << maxPort << ")" << Endl;
        }

        TServiceIntegrator<TStatsService, TYabioContextService> Services;
    };

    struct TYabioContextRequestData {
        TYabioContextRequestData(const TString& method, TYabioContextKey key)
            : Method(method)
            , Key(key)
        {
        }

        TString Method;
        TYabioContextKey Key;
        YabioProtobuf::YabioContext RequestContext;
        bool HasResponseContext = false;
        YabioProtobuf::YabioContext ResponseContext;
    };
    using TYabioContextRequestDataPtr = std::shared_ptr<TYabioContextRequestData>;

    class TCachalotRequest: public TCachalotClient::TRequest {
    public:
        TCachalotRequest(TCachalotClient& cachalotClient, NAsio::TIOService& ioService, TAutoEvent& event, TString& error)
            : TCachalotClient::TRequest(cachalotClient, ioService)
            , Event_(event)
            , Error_(error)
        {
        }
        ~TCachalotRequest() override {
            Cout << "~TCachalotRequest" << Endl;
            Event_.Signal();
        }

        void SendYabioContextRequest2(TYabioContextRequestDataPtr requestData) {
            UNIT_ASSERT(requestData);
            Request_ = requestData;
            NCachalotProtocol::TYabioContextRequest request;
            NCachalotProtocol::TYabioContextKey key;
            requestData->Key.SaveTo(key);
            const TString& method = requestData->Method;
            if (method == "put" || method == "save") {
                TString contextStr;
                Y_PROTOBUF_SUPPRESS_NODISCARD requestData->RequestContext.SerializeToString(&contextStr);
                auto& save = *request.MutableSave();
                *save.MutableKey() = key;
                Cout << "Send YabioContextRequest: " << request.ShortUtf8DebugString() << Endl;
                save.SetContext(contextStr);
            } else if (method == "get" || method == "load") {
                *request.MutableLoad()->MutableKey() = key;
                Cout << "Send YabioContextRequest: " << request.ShortUtf8DebugString() << Endl;
            } else if (method == "del" || method == "delete") {
                *request.MutableDelete()->MutableKey() = key;
                Cout << "Send YabioContextRequest: " << request.ShortUtf8DebugString() << Endl;
            } else {
                throw yexception() << "unknown yabio_context method: " << method;
            }
            SendYabioContextRequest(std::move(request), false);
        }

    private:
        void OnYabioContextResponse(NCachalotProtocol::TYabioContextResponse& response) override {
            if (response.HasSuccess()) {
                if (response.GetSuccess().HasContext()) {
                    Request_->HasResponseContext = true;
                    TString s = response.GetSuccess().GetContext();
                    response.MutableSuccess()->ClearContext();
                    Cout << "got YabioContextResponse: " << response.ShortUtf8DebugString() << Endl;
                    try {
                        // parse response.GetSuccess().GetContext(), print HR version
                        Y_PROTOBUF_SUPPRESS_NODISCARD Request_->ResponseContext.ParseFromString(s);
                        Cout << TStringBuilder() << "YabioContext: " << Request_->ResponseContext.ShortUtf8DebugString() << Endl;
                        NJson::TJsonValue json;
                    } catch (...) {
                        OnError(CurrentExceptionMessage());
                    }
                } else {
                    Cout << "got YabioContextResponse: " << response.ShortUtf8DebugString() << Endl;
                }
            } else if (response.HasError()) {
                Cout << "got YabioContextResponse: " << response.ShortUtf8DebugString() << Endl;
                OnError(TStringBuilder() << "got error response: " << response.ShortUtf8DebugString());
            } else {
                Cout << "got YabioContextResponse: " << response.ShortUtf8DebugString() << Endl;
                OnError("got YabioContextResponse with undefined payload (!Error & !Success)");
            }
        }
        bool NextAsyncRead() override {
            if (TCachalotClient::TRequest::NextAsyncRead()) {
                Cout << "NextAsyncRead" << Endl;
                return true;
            }
            return false;
        }
        void OnEndNextResponseProcessing() override {
            TCachalotClient::TRequest::OnEndNextResponseProcessing();
        }
        void OnEndOfResponsesStream() override {
            Cout << "EndOfResponsesStream" << Endl;
        }
        void OnTimeout() override {
            TCachalotClient::TRequest::OnTimeout();
        }
        void OnError(const TString& s) override {
            Cout << "OnError: " << s << Endl;
            if (!Error_) {
                Error_ = s;
            }
            TCachalotClient::TRequest::OnError(s);
        }

    private:
        TAutoEvent& Event_;
        TString& Error_;
        TYabioContextRequestDataPtr Request_;
    };
}

class TCachalotYabioContextTest: public TTestBase {
    UNIT_TEST_SUITE(TCachalotYabioContextTest);
    UNIT_TEST(TestSaveLoadDelete);
    UNIT_TEST(TestHttpDelete);
    UNIT_TEST_SUITE_END();

public:
    void TestSaveLoadDelete() {
        NCachalot::TMetrics::GetInstance();
        NCachalot::TApplicationSettings appSettings;
        appSettings.YabioContext().SetEnabled(true);
        appSettings.YabioContext().Storage().YdbClient().SetIsFake(true);
        appSettings.Server().SetPort(10000);  // try bind ports starting from this value
        auto cachalot = TCachalotExecutor::Run(appSettings);
        // here we have runned cachalot

        TCachalotClient::TConfig config;
        config.Host = "localhost";
        config.Port = appSettings.Server().GrpcPort();
        config.Path = "yabio_context";
        TCachalotClient cachalotClient(config);
        // here we have ready cachalot client

        NAsio::TExecutorsPool pool(1);  // for timers in request
        NCachalot::TYabioContextKey key;  // tested record key
        key.GroupId = "ut_test_group_id";
        key.DevModel = "ut_test_dev_model";
        key.DevManuf = "ut_test_dev_manuf";
        TString contextGroupId = "ut_test_group_id_777";
        // use cachalot client for save, load, delete yabio_context record
        {
            TString error;
            TAutoEvent eventFinish;
            // save
            {
                TIntrusivePtr<TCachalotRequest> cachalotRequest(new TCachalotRequest(cachalotClient, pool.GetExecutor().GetIOService(), eventFinish, error));
                auto requestData = std::make_shared<TYabioContextRequestData>("save", key);
                requestData->RequestContext.set_group_id(contextGroupId);  // << put some data to save context
                cachalotRequest->SendYabioContextRequest2(requestData);
                cachalotRequest.Reset();
                eventFinish.WaitI();
                UNIT_ASSERT_C(!error, error);
            }
            // load (check saved data)
            {
                TIntrusivePtr<TCachalotRequest> cachalotRequest(new TCachalotRequest(cachalotClient, pool.GetExecutor().GetIOService(), eventFinish, error));
                auto requestData = std::make_shared<TYabioContextRequestData>("load", key);
                cachalotRequest->SendYabioContextRequest2(requestData);
                cachalotRequest.Reset();
                eventFinish.WaitI();
                UNIT_ASSERT_C(!error, error);
                UNIT_ASSERT(requestData->HasResponseContext && requestData->ResponseContext.has_group_id());
                UNIT_ASSERT_VALUES_EQUAL(contextGroupId, requestData->ResponseContext.group_id());
            }
            // delete
            {
                TIntrusivePtr<TCachalotRequest> cachalotRequest(new TCachalotRequest(cachalotClient, pool.GetExecutor().GetIOService(), eventFinish, error));
                auto requestData = std::make_shared<TYabioContextRequestData>("delete", key);
                cachalotRequest->SendYabioContextRequest2(requestData);
                cachalotRequest.Reset();
                eventFinish.WaitI();
                UNIT_ASSERT_C(!error, error);
            }
            // load (check empty result after delete)
            {
                TIntrusivePtr<TCachalotRequest> cachalotRequest(new TCachalotRequest(cachalotClient, pool.GetExecutor().GetIOService(), eventFinish, error));
                auto requestData = std::make_shared<TYabioContextRequestData>("load", key);
                cachalotRequest->SendYabioContextRequest2(requestData);
                cachalotRequest.Reset();
                eventFinish.WaitI();
                UNIT_ASSERT_C(!error, error);
                UNIT_ASSERT_EQUAL(requestData->HasResponseContext, false);
            }
        }
    }

    void TestHttpDelete() {
        NCachalot::TMetrics::GetInstance();
        NCachalot::TApplicationSettings appSettings;
        appSettings.YabioContext().SetEnabled(true);
        appSettings.YabioContext().Storage().YdbClient().SetIsFake(true);
        appSettings.Server().SetPort(10000);  // try bind ports starting from this value
        auto cachalot = TCachalotExecutor::Run(appSettings);
        // here we have runned cachalot

        TCachalotClient::TConfig config;
        config.Host = "localhost";
        config.Port = appSettings.Server().GrpcPort();
        config.Path = "yabio_context";
        TCachalotClient cachalotClient(config);
        // here we have ready cachalot client

        NAsio::TExecutorsPool pool(1);  // for timers in request
        NCachalot::TYabioContextKey key;  // tested record key
        key.GroupId = "ut_test_group_id";
        key.DevModel = "";
        key.DevManuf = "";
        TString contextGroupId = "ut_test_group_id_777";
        // use cachalot client for save, load, delete yabio_context record
        {
            TString error;
            TAutoEvent eventFinish;
            // delete
            {
                THttpCachalotClient cachalot(TStringBuilder() << "http://localhost:" << appSettings.Server().Port() << "/yabio_context/v2");
                auto answer = cachalot.SendYabioContextDeleteRequest(key.GroupId);
                Cerr << "ERROR: " << answer.ErrorText << Endl;
                UNIT_ASSERT_EQUAL(answer.Ok, true);
                UNIT_ASSERT_EQUAL(answer.ErrorText, "");
                UNIT_ASSERT_C(!error, error);
            }
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCachalotYabioContextTest)
