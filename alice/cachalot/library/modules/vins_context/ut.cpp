#include <alice/cachalot/library/cachalot.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/service.h>
#include <alice/cachalot/library/modules/cache/service.h>
#include <alice/cachalot/library/modules/stats/service.h>
#include <alice/cachalot/library/modules/megamind_session/service.h>
#include <alice/cachalot/library/modules/vins_context/service.h>

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
        TCachalotExecutor(NCachalot::TApplicationSettings settings)
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

            for (ui16 port = settings.Server().Port(); port < maxPort; port += 3) {
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

        TServiceIntegrator<TStatsService, TVinsContextService> Services;
    };
}

class TCachalotVinsContextTest: public TTestBase {
    UNIT_TEST_SUITE(TCachalotVinsContextTest);
    UNIT_TEST(TestHttpDelete);
    UNIT_TEST_SUITE_END();

public:
    void TestHttpDelete() {
        NCachalot::TMetrics::GetInstance();
        NCachalot::TApplicationSettings appSettings;
        appSettings.VinsContext().SetEnabled(true);
        appSettings.VinsContext().Ydb().SetIsFake(true);
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
        NCachalot::TVinsContextKey key;  // tested record key
        key.Puid = "ut_test_puid";
        // use cachalot client for save, load, delete yabio_context record
        {
            TString error;
            TAutoEvent eventFinish;
            // delete
            {
                THttpCachalotClient cachalot(TStringBuilder() << "http://localhost:" << appSettings.Server().Port() << "/vins_context/v1");
                auto answer = cachalot.SendVinsContextDeleteRequest(key.Puid);
                Cerr << "ERROR: " << answer.ErrorText << Endl;
                UNIT_ASSERT_EQUAL(answer.Ok, true);
                UNIT_ASSERT_EQUAL(answer.ErrorText, "");
                UNIT_ASSERT_C(!error, error);
            }
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCachalotVinsContextTest)
