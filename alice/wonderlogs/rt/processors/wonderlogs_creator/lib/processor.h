#pragma once

#include "state.h"

#include <alice/wonderlogs/rt/processors/wonderlogs_creator/protos/config.pb.h>

#include <alice/wonderlogs/rt/processors/megamind_creator/lib/state.h>
#include <alice/wonderlogs/rt/processors/uniproxy_creator/lib/state.h>

#include <alice/wonderlogs/protos/megamind_prepared.pb.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <ads/bsyeti/big_rt/lib/processing/shard_processor/stateful/processor.h>
#include <ads/bsyeti/big_rt/lib/processing/state_manager/read_only/factory.h>
#include <ads/bsyeti/big_rt/lib/processing/state_manager/read_only/manager.h>

#include <kernel/yt/dynamic/proto_api.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <quality/user_sessions/rt/lib/state_managers/proto/factory.h>

namespace NAlice::NWonderlogs {

using TProcessorBase = NBigRT::TCompositeStatefulShardProcessor<TUuidMessageId>;

using TUniproxyPreparedStateDescriptor =
    NBigRT::TStateDescriptor<NBigRT::TReadOnlyStateManagerFactory<NUserSessions::NRT::TProtoStateManagerFactory<
        TUniproxyPreparedWrapper, std::tuple<TString, TString>, THolder<TUniproxyPreparedState>>>>;

using TMegamindPreparedStateDescriptor =
    NBigRT::TStateDescriptor<NBigRT::TReadOnlyStateManagerFactory<NUserSessions::NRT::TProtoStateManagerFactory<
        TMegamindPreparedWrapper, std::tuple<TString, TString>, THolder<TMegamindPreparedState>>>>;

using TUuidMessageIdStateDescriptor = NBigRT::TStateDescriptor<NUserSessions::NRT::TProtoStateManagerFactory<
    TUuidMessageId, std::tuple<TString, TString>, THolder<TUuidMessageIdState>>>;

using TUniproxyPreparedUnderlyingStateFactory = TUniproxyPreparedStateDescriptor::TFactory::TFactory;
using TMegamindPreparedUnderlyingStateFactory = TMegamindPreparedStateDescriptor::TFactory::TFactory;
using TUuidMessageIdUnderlyingStateFactory = TUuidMessageIdStateDescriptor::TFactory;

struct TStateDescriptors {
    TUniproxyPreparedStateDescriptor UniproxyPreparedDescriptor;
    TMegamindPreparedStateDescriptor MegamindPreparedDescriptor;
    TUuidMessageIdStateDescriptor UuidMessageIdDescriptor;
};

class TWonderlogsCreator : public TProcessorBase {
public:
    using TGroupedChunk = typename TProcessorBase::TGroupedChunk;
    using TPrepareForAsyncWriteResult = typename TProcessorBase::TPrepareForAsyncWriteResult;
    using TManager = typename TProcessorBase::TManager;

public:
    TWonderlogsCreator(TBase::TConstructionArgs spArgs, TWonderlogsCreatorConfig::TProcessorConfig config,
                       TStateDescriptors stateDescriptors)
        : TProcessorBase(spArgs)
        , Config(std::move(config))
        , StateDescriptors(std::move(stateDescriptors)) {
    }

    TGroupedChunk PrepareGroupedChunk(TString dataSource, TManager& stateManager, NBigRT::TMessageBatch data) override;
    void ProcessGroupedChunk(TString dataSource, TGroupedChunk groupedRows) override;
    NYT::TFuture<TPrepareForAsyncWriteResult> PrepareForAsyncWrite() override;

private:
    TWonderlogsCreatorConfig::TProcessorConfig Config;
    THashMap<ui64, TVector<TString>> Data;
    TStateDescriptors StateDescriptors;
};

} // namespace NAlice::NWonderlogs
