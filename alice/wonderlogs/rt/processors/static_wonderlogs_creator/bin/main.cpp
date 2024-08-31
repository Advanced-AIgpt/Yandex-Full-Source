#include <alice/wonderlogs/rt/processors/static_wonderlogs_creator/lib/processor.h>
#include <alice/wonderlogs/rt/processors/static_wonderlogs_creator/protos/config.pb.h>

#include <alice/wonderlogs/rt/library/common/names.h>

#include <ads/bsyeti/big_rt/lib/consuming_system/consuming_system.h>
#include <ads/bsyeti/big_rt/lib/deprecated/sensors/sensors.h>
#include <ads/bsyeti/big_rt/lib/processing/shard_processor/stateless/processor.h>
#include <ads/bsyeti/big_rt/lib/supplier/supplier.h>
#include <ads/bsyeti/big_rt/lib/utility/logging/logging.h>
#include <ads/bsyeti/libs/profiling/solomon/exporter.h>
#include <ads/bsyeti/libs/ytex/common/await.h>
#include <ads/bsyeti/libs/ytex/http/server.h>
#include <ads/bsyeti/libs/ytex/http/std_handlers.h>
#include <ads/bsyeti/libs/ytex/program/program.h>

namespace {

inline const TString STATIC_WONDERLOGS_CREATOR = "static_wonderlogs_creator";

using namespace NAlice::NWonderlogs;

void RunStaticTableCreator(const TStaticTableCreatorConfig& config,
                           const NYT::TCancelableContextPtr& cancelableContext) {
    auto solomonExporter = NBSYeti::NProfiling::CreateSolomonExporter(config.GetSolomonExporter());
    solomonExporter->Start();

    auto httpServer = NYTEx::NHttp::CreateServer(config.GetHttpServer());
    NYTEx::NHttp::AddStandardHandlers(httpServer, cancelableContext, config,
                                      NBSYeti::NProfiling::GetSensorsGetter(solomonExporter));
    httpServer->Start();

    auto profilerTags = NYT::NProfiling::TTagSet().WithTag({PROCESS, STATIC_WONDERLOGS_CREATOR});

    auto inflightLimiter = NBigRT::CreateProfiledInflightLimiter(profilerTags, config.GetMaxInflightBytes());
    auto ytClientsPool = NYTEx::CreateTransactionKeeper(TDuration::Zero());

    auto consSystem = NBigRT::CreateConsumingSystem({
        .Config = config.GetConsumingSystem(),
        .SuppliersProvider = NBigRT::CreateSupplierFactoriesProvider({.ConfigsRepeated = config.GetSuppliers()}),
        .YtClients = ytClientsPool,
        .ShardsProcessor =
            [=](NBigRT::TConsumingSystem::IConsumer& consumer) {
                TStaticTableProcessor{
                    NBigRT::TStatelessShardProcessor::TConstructionArgs{
                        consumer, config.GetStatelessShardProcessorConfig(), ytClientsPool, inflightLimiter},
                    config.GetProcessorConfig()}
                    .Run();
            },
        .ProfilerTags = profilerTags,
    });

    consSystem->Run();
    NYTEx::WaitFor(cancelableContext);
    INFO_LOG << "Stopping consuming system\n";
    NYT::NConcurrency::WaitUntilSet(consSystem->Stop());
}

class TStaticTableCreatorProgram : public NYTEx::TProgram<TStaticTableCreatorConfig> {
public:
    TStaticTableCreatorProgram()
        : NYTEx::TProgram<TStaticTableCreatorConfig>(STATIC_WONDERLOGS_CREATOR) {
    }

    int DoRun(const NYT::TCancelableContextPtr& context) override {
        RunStaticTableCreator(Config(), context);
        return 0;
    }
};

} // namespace

int main(int argc, const char** argv) {
    return NYTEx::RunProgram<TStaticTableCreatorProgram>(argc, argv);
}
