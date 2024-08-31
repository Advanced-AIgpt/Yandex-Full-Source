#include <alice/wonderlogs/rt/processors/megamind_resharder/lib/processor.h>
#include <alice/wonderlogs/rt/processors/megamind_resharder/protos/config.pb.h>

#include <alice/wonderlogs/rt/library/common/names.h>

#include <ads/bsyeti/big_rt/lib/processing/resharder/lib/resharder.h>
#include <ads/bsyeti/big_rt/lib/utility/profiling/safe_stats_over_yt.h>
#include <ads/bsyeti/libs/profiling/solomon/exporter.h>
#include <ads/bsyeti/libs/ytex/common/await.h>
#include <ads/bsyeti/libs/ytex/http/server.h>
#include <ads/bsyeti/libs/ytex/http/std_handlers.h>
#include <ads/bsyeti/libs/ytex/program/program.h>

#include <logfeller/lib/parsing/config_storage/config_storage.h>

namespace {

inline const TString MEGAMIND_RESHARDER = "megamind_resharder";

using namespace NAlice::NWonderlogs;
using namespace NBigRT;

void RunResharding(const NAlice::NWonderlogs::TMegamindResharderConfig& config,
                   const NYT::TCancelableContextPtr& cancelableContext) {
    auto solomonExporter = NBSYeti::NProfiling::CreateSolomonExporter(config.GetSolomonExporter());
    solomonExporter->Start();

    auto httpServer = NYTEx::NHttp::CreateServer(config.GetHttpServer());
    NYTEx::NHttp::AddStandardHandlers(httpServer, cancelableContext, config,
                                      NBSYeti::NProfiling::GetSensorsGetter(solomonExporter));
    httpServer->Start();

    auto profilerTags = NYT::NProfiling::TTagSet().WithTag({PROCESS, MEGAMIND_RESHARDER});

    NBSYeti::TTvmManagerPtr tvmManager{config.HasTvmConfig() ? NBSYeti::CreateTvmManager(config.GetTvmConfig())
                                                             : NBSYeti::TTvmManagerPtr{}};

    NPersQueue::TPQLibSettings lbPqLibSettings;
    lbPqLibSettings.ThreadsCount = 20;
    lbPqLibSettings.GRpcThreads = 20;
    lbPqLibSettings.ChannelCreationTimeout = TDuration::Seconds(10);
    lbPqLibSettings.DefaultLogger = new TTPQLibGlobalLogBridge();
    NPersQueue::TPQLib lbPqLib(lbPqLibSettings);

    auto inflightLimiter = CreateProfiledInflightLimiter(profilerTags, config.GetMaxInflightBytes());
    auto ytClientsPool = NYTEx::CreateTransactionKeeper(TDuration::Zero());

    auto consSystem = CreateConsumingSystem({
        .Config = config.GetConsumingSystem(),
        .SuppliersProvider = CreateSupplierFactoriesProvider({
            .ConfigsRepeated = config.GetSuppliers(),
            .TvmManager = tvmManager,
            .LbPqLib = &lbPqLib,
        }),
        .YtClients = ytClientsPool,
        .ShardsProcessor =
            [=](TConsumingSystem::IConsumer& consumer) {
                TMegamindResharder{
                    TStatelessShardProcessor::TConstructionArgs{consumer, config.GetStatelessShardProcessorConfig(),
                                                                ytClientsPool, inflightLimiter},
                    config.GetShardingConfig()}
                    .Run();
            },
        .ProfilerTags = profilerTags,
    });

    consSystem->Run();
    NYTEx::WaitFor(cancelableContext);
    INFO_LOG << "Stopping consuming system\n";
    NYT::NConcurrency::WaitUntilSet(consSystem->Stop());
}

class TShardingProgram : public NYTEx::TProgram<TMegamindResharderConfig> {
public:
    TShardingProgram()
        : NYTEx::TProgram<TMegamindResharderConfig>(MEGAMIND_RESHARDER) {
    }

    int DoRun(const NYT::TCancelableContextPtr& context) override {
        RunResharding(Config(), context);
        return 0;
    }
};

} // namespace

int main(int argc, const char** argv) {
    return NYTEx::RunProgram<TShardingProgram>(argc, argv);
}
