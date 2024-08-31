#pragma once

#include <alice/wonderlogs/rt/processors/uniproxy_resharder/protos/config.pb.h>

#include <ads/bsyeti/big_rt/lib/processing/shard_processor/stateless/processor.h>

namespace NAlice::NWonderlogs {

class TUniproxyResharder : public NBigRT::TStatelessShardProcessor {
public:
    TUniproxyResharder(NBigRT::TStatelessShardProcessor::TConstructionArgs spArgs,
                       const TUniproxyResharderConfig::TShardingConfig& config);
    void Process(TString dataSource, NBigRT::TMessageBatch dataMessage) override;
    NYT::TFuture<TPrepareForAsyncWriteResult> PrepareForAsyncWrite() override;

private:
    THashMap<ui64, TVector<TString>> Data;
    const TUniproxyResharderConfig::TShardingConfig Config;
};

} // namespace NAlice::NWonderlogs
