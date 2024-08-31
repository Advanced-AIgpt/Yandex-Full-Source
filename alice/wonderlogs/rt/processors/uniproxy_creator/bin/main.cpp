#include <alice/wonderlogs/rt/processors/uniproxy_creator/lib/processor.h>

#include <alice/wonderlogs/rt/library/common/names.h>

#include <ads/bsyeti/big_rt/lib/supplier/supplier.h>
#include <ads/bsyeti/big_rt/lib/utility/profiling/safe_stats_over_yt.h>
#include <ads/bsyeti/libs/profiling/solomon/exporter.h>
#include <ads/bsyeti/libs/ytex/http/server.h>
#include <ads/bsyeti/libs/ytex/http/std_handlers.h>
#include <ads/bsyeti/libs/ytex/program/program.h>

#include <quality/user_sessions/rt/lib/common/yt_client.h>
#include <quality/user_sessions/rt/lib/state_managers/proto/factory.h>

namespace {

inline const TString UNIPROXY_CREATOR = "uniproxy_creator";

using namespace NAlice::NWonderlogs;

using NUserSessions::NRT::GetKeyColumns;
using NUserSessions::NRT::TProtoStateManagerFactory;

void RunProcessing(const TUniproxyCreatorConfig& config, const NYT::TCancelableContextPtr& context) {
    auto solomonExporter = NBSYeti::NProfiling::CreateSolomonExporter(config.GetExporter());
    auto httpServer = NYTEx::NHttp::CreateServer(config.GetHttpServer());
    solomonExporter->Start();
    NYTEx::NHttp::AddStandardHandlers(httpServer, context, config, BIND([solomonExporter] {
                                          return NYT::TSharedRef::FromString(
                                              NBSYeti::NProfiling::GetSensors(solomonExporter));
                                      }));
    httpServer->Start();

    auto processingTags = NYT::NProfiling::TTagSet().WithTag({PROCESS, UNIPROXY_CREATOR});
    auto rootCtx = NBigRT::MakeSolomonContext(processingTags);

    auto inflightLimiter = NYT::New<NBigRT::TInflightLimiter>(config.GetMaxInflightBytes());
    auto ytClientsPool = CreateTransactionKeeper(config.GetYtClientConfig());
    auto csConfig = config.GetConsumingSystem();

    NBSYeti::TStopToken stopToken(context);

    auto stateManagerFactory = NYT::New<
        TProtoStateManagerFactory<TUniproxyPreparedWrapper, TUniproxyCreator::TStateId, TUniproxyCreator::TState>>(
        config.GetProtoStateManagerConfig(), GetKeyColumns<TUniproxyPreparedWrapper>(),
        [](auto row) { return MakeHolder<TUniproxyPreparedState>(std::move(row)); }, rootCtx);

    auto consSystem = NBigRT::CreateConsumingSystem({
        .Config = csConfig,
        .SuppliersProvider = NBigRT::CreateSupplierFactoriesProvider({.ConfigsRepeated = config.GetSuppliers()}),
        .YtClients = ytClientsPool,
        .ShardsProcessor =
            [ytClientsPool, inflightLimiter, stateManagerFactory, &config,
             &rootCtx](NBigRT::TConsumingSystem::IConsumer& consumer) {
                TUniproxyCreator{
                    TUniproxyCreator::TConstructionArgs{.Consumer = consumer,
                                                        .Config = config.GetStatefulShardProcessorConfig(),
                                                        .StateManagerFactory = stateManagerFactory,
                                                        .TransactionKeeper = ytClientsPool,
                                                        .InflightLimiter = inflightLimiter,
                                                        .SensorsContext = rootCtx},
                    config.GetProcessorConfig()}
                    .Run();
            },
        .SensorsContext = rootCtx,
    });

    consSystem->Run();

    stopToken.Wait();
    INFO_LOG << "Stopping consuming system\n";

    consSystem->Stop().Get();
}

class TUniproxyCreatorProgram : public NYTEx::TProgram<TUniproxyCreatorConfig> {
public:
    TUniproxyCreatorProgram()
        : NYTEx::TProgram<TUniproxyCreatorConfig>(UNIPROXY_CREATOR) {
    }

    int DoRun(const NYT::TCancelableContextPtr& context) override {
        RunProcessing(Config(), context);
        return 0;
    }
};

} // namespace

int main(int argc, const char** argv) {
    return NYTEx::RunProgram<TUniproxyCreatorProgram>(argc, argv);
}
