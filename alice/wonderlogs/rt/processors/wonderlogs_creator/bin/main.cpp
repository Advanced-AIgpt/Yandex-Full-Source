#include <alice/wonderlogs/rt/processors/wonderlogs_creator/lib/processor.h>

#include <alice/wonderlogs/rt/library/common/names.h>

#include <ads/bsyeti/big_rt/lib/processing/resharder/lib/resharder.h>
#include <ads/bsyeti/big_rt/lib/utility/profiling/safe_stats_over_yt.h>
#include <ads/bsyeti/libs/profiling/solomon/exporter.h>
#include <ads/bsyeti/libs/ytex/http/server.h>
#include <ads/bsyeti/libs/ytex/http/std_handlers.h>
#include <ads/bsyeti/libs/ytex/program/program.h>

#include <quality/user_sessions/rt/lib/common/yt_client.h>
#include <quality/user_sessions/rt/lib/state_managers/proto/factory.h>

namespace {

inline const TString WONDERLOGS_CREATOR = "wonderlogs_creator";

using namespace NAlice::NWonderlogs;

using NUserSessions::NRT::GetKeyColumns;
using NUserSessions::NRT::TProtoStateManagerFactory;

void RunProcessing(const TWonderlogsCreatorConfig& config, const NYT::TCancelableContextPtr& context) {
    auto solomonExporter = NBSYeti::NProfiling::CreateSolomonExporter(config.GetExporter());
    auto httpServer = NYTEx::NHttp::CreateServer(config.GetHttpServer());
    solomonExporter->Start();
    NYTEx::NHttp::AddStandardHandlers(httpServer, context, config, BIND([solomonExporter] {
                                          return NYT::TSharedRef::FromString(
                                              NBSYeti::NProfiling::GetSensors(solomonExporter));
                                      }));
    httpServer->Start();

    auto processingTags = NYT::NProfiling::TTagSet().WithTag({PROCESS, WONDERLOGS_CREATOR});
    auto rootCtx = NBigRT::MakeSolomonContext(processingTags);

    auto inflightLimiter = NYT::New<NBigRT::TInflightLimiter>(config.GetMaxInflightBytes());
    auto ytClientsPool = CreateTransactionKeeper(config.GetYtClientConfig());
    auto csConfig = config.GetConsumingSystem();

    NBSYeti::TStopToken stopToken(context);

    auto uuidMessageIdStateManagerFactory = NYT::New<
        TProtoStateManagerFactory<TUuidMessageId, std::tuple<TString, TString>, THolder<TUuidMessageIdState>>>(
        config.GetStateManagerConfigs().GetUuidMessageId(), GetKeyColumns<TUuidMessageId>(),
        [](auto row) { return MakeHolder<TUuidMessageIdState>(std::move(row)); }, rootCtx);

    auto uniproxyPreparedStateManagerFactory = NYT::New<TUniproxyPreparedStateDescriptor::TFactory>(
        config.GetStateManagerConfigs().GetUniproxyPrepared(), GetKeyColumns<TUniproxyPreparedWrapper>(),
        [](auto row) { return MakeHolder<TUniproxyPreparedState>(std::move(row)); }, rootCtx);

    auto megamindPreparedStateManagerFactory = NYT::New<TMegamindPreparedStateDescriptor::TFactory>(
        config.GetStateManagerConfigs().GetMegamindPrepared(), GetKeyColumns<TMegamindPreparedWrapper>(),
        [](auto row) { return MakeHolder<TMegamindPreparedState>(std::move(row)); }, rootCtx);

    auto compositeStateManagerFactory = NYT::New<NBigRT::TCompositeStateManagerFactory>();

    TStateDescriptors stateDescriptors{
        .UniproxyPreparedDescriptor =
            compositeStateManagerFactory->Add(std::move(uniproxyPreparedStateManagerFactory)),
        .MegamindPreparedDescriptor =
            compositeStateManagerFactory->Add(std::move(megamindPreparedStateManagerFactory)),
        .UuidMessageIdDescriptor = compositeStateManagerFactory->Add(std::move(uuidMessageIdStateManagerFactory))};

    auto consSystem = NBigRT::CreateConsumingSystem({
        .Config = csConfig,
        .SuppliersProvider = NBigRT::CreateSupplierFactoriesProvider({.ConfigsRepeated = config.GetSuppliers()}),
        .YtClients = ytClientsPool,
        .ShardsProcessor =
            [ytClientsPool, inflightLimiter, stateDescriptors, compositeStateManagerFactory, &config,
             &rootCtx](NBigRT::TConsumingSystem::IConsumer& consumer) {
                TWonderlogsCreator{TWonderlogsCreator::TConstructionArgs{
                                       consumer, config.GetStatefulShardProcessorConfig(),
                                       compositeStateManagerFactory, ytClientsPool, inflightLimiter, rootCtx},
                                   config.GetProcessorConfig(), stateDescriptors}
                    .Run();
            },
        .SensorsContext = rootCtx,
    });

    consSystem->Run();

    stopToken.Wait();
    INFO_LOG << "Stopping consuming system\n";

    consSystem->Stop().Get();
}

class TWonderlogsCreatorProgram : public NYTEx::TProgram<TWonderlogsCreatorConfig> {
public:
    TWonderlogsCreatorProgram()
        : NYTEx::TProgram<TWonderlogsCreatorConfig>(WONDERLOGS_CREATOR) {
    }

    int DoRun(const NYT::TCancelableContextPtr& context) override {
        RunProcessing(Config(), context);
        return 0;
    }
};

} // namespace

int main(int argc, const char** argv) {
    return NYTEx::RunProgram<TWonderlogsCreatorProgram>(argc, argv);
}
