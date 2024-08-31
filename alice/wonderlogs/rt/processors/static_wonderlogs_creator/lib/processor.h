#include <alice/wonderlogs/rt/processors/static_wonderlogs_creator/protos/config.pb.h>
#include <alice/wonderlogs/rt/processors/static_wonderlogs_creator/protos/logviewer.pb.h>

#include <ads/bsyeti/big_rt/lib/processing/shard_processor/stateless/processor.h>

namespace NAlice::NWonderlogs {

class TStaticTableProcessor : public NBigRT::TStatelessShardProcessor {
public:
    TStaticTableProcessor(TStatelessShardProcessor::TConstructionArgs spArgs,
                          const TStaticTableCreatorConfig::TProcessorConfig& config);

    void Process(TString dataSource, NBigRT::TMessageBatch dataMessage) override;
    NYT::TFuture<TPrepareForAsyncWriteResult> PrepareForAsyncWrite() override;

private:
    const TStaticTableCreatorConfig::TProcessorConfig& Config;
    TVector<TLogviewer> LogviwerData;
};

} // namespace NAlice::NWonderlogs
