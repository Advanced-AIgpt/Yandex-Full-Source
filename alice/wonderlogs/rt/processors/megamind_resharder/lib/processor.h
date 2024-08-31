#pragma once

#include <alice/wonderlogs/rt/processors/megamind_resharder/protos/config.pb.h>

#include <ads/bsyeti/big_rt/lib/processing/shard_processor/stateless/processor.h>

namespace NAlice::NWonderlogs {

class TMegamindResharder : public NBigRT::TStatelessShardProcessor {
public:
    TMegamindResharder(NBigRT::TStatelessShardProcessor::TConstructionArgs spArgs,
                       const TMegamindResharderConfig::TShardingConfig& config);
    void Process(TString dataSource, NBigRT::TMessageBatch dataMessage) override;
    NYT::TFuture<TPrepareForAsyncWriteResult> PrepareForAsyncWrite() override;

private:
    THashMap<ui64, TVector<TString>> Data;
    const TMegamindResharderConfig::TShardingConfig Config;
};

} // namespace NAlice::NWonderlogs
