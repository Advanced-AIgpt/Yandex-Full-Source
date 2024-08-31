#pragma once

#include "state.h"

#include <alice/wonderlogs/rt/processors/uniproxy_creator/protos/config.pb.h>

#include <alice/wonderlogs/rt/protos/uniproxy_event.pb.h>

#include <ads/bsyeti/big_rt/lib/processing/shard_processor/stateful/processor.h>

#include <kernel/yt/dynamic/proto_api.h>

namespace NAlice::NWonderlogs {

class TUniproxyCreator : public NBigRT::TStatefulShardProcessor<std::tuple<TString, TString>,
                                                                THolder<TUniproxyPreparedState>, TUniproxyEvent> {
public:
    using TGroupedChunk = typename TStatefulShardProcessor::TGroupedChunk;
    using TPrepareForAsyncWriteResult = typename TStatefulShardProcessor::TPrepareForAsyncWriteResult;
    using TManager = typename TStatefulShardProcessor::TManager;
    using TData = TVector<TUniproxyPreparedWrapper>;

public:
    TUniproxyCreator(TBase::TConstructionArgs spArgs, TUniproxyCreatorConfig::TProcessorConfig config)
        : TStatefulShardProcessor(spArgs)
        , Config(std::move(config)) {
    }

    TGroupedChunk PrepareGroupedChunk(TString dataSource, TManager& stateManager, NBigRT::TMessageBatch data) override;
    void ProcessGroupedChunk(TString dataSource, TGroupedChunk groupedRows) override;
    NYT::TFuture<TPrepareForAsyncWriteResult> PrepareForAsyncWrite() override;

private:
    TUniproxyCreatorConfig::TProcessorConfig Config;
    THashMap<ui64, TVector<TString>> UuidMessageId;
};

} // namespace NAlice::NWonderlogs
