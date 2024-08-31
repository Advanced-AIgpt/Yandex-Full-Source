#pragma once

#include "state.h"

#include <alice/wonderlogs/rt/processors/megamind_creator/protos/config.pb.h>
#include <alice/wonderlogs/rt/processors/megamind_creator/protos/megamind_prepared_wrapper.pb.h>

#include <alice/wonderlogs/protos/megamind_prepared.pb.h>

#include <ads/bsyeti/big_rt/lib/processing/shard_processor/stateful/processor.h>

#include <kernel/yt/dynamic/proto_api.h>

namespace NAlice::NWonderlogs {

class TMegamindCreator
    : public NBigRT::TStatefulShardProcessor<std::tuple<TString, TString>, THolder<TMegamindPreparedState>,
                                             TMegamindPrepared::TMegamindRequestResponse> {
public:
    using TGroupedChunk = typename TStatefulShardProcessor::TGroupedChunk;
    using TPrepareForAsyncWriteResult = typename TStatefulShardProcessor::TPrepareForAsyncWriteResult;
    using TManager = typename TStatefulShardProcessor::TManager;
    using TData = TVector<TMegamindPreparedWrapper>;

public:
    TMegamindCreator(TBase::TConstructionArgs spArgs, TMegamindCreatorConfig::TProcessorConfig config)
        : TStatefulShardProcessor(spArgs)
        , Config(std::move(config)) {
    }

    TGroupedChunk PrepareGroupedChunk(TString dataSource, TManager& stateManager, NBigRT::TMessageBatch data) override;
    void ProcessGroupedChunk(TString dataSource, TGroupedChunk groupedRows) override;
    NYT::TFuture<TPrepareForAsyncWriteResult> PrepareForAsyncWrite() override;

private:
    TMegamindCreatorConfig::TProcessorConfig Config;
    THashMap<ui64, TVector<TString>> UuidMessageId;
};

} // namespace NAlice::NWonderlogs
