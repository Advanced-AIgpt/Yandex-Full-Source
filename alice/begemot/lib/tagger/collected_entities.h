#pragma once

#include <search/begemot/rules/occurrences/custom_entities/rule/proto/custom_entities.pb.h>
#include <search/begemot/rules/alice/entities_collector/proto/alice_entities_collector.pb.h>

#include <alice/library/request/token_range.h>

#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/string.h>

namespace NAlice {

struct TRecognition {
    TRecognition(const TTokenRange& range, const TString& value)
        : Range(range)
        , Value(value)
    {
    }

    bool operator>(const TRecognition& rhs) const {
        return (Range.Size() == rhs.Range.Size()) ? (Range.Start > rhs.Range.Start) : (Range.Size() > rhs.Range.Size());
    }

    TTokenRange Range;
    TString Value;
};

using TRecognitions = TSet<TRecognition, TGreater<TRecognition>>;

using TCollectedEntities = THashMap<TString, TRecognitions>;

TCollectedEntities CollectEntities(const NBg::NProto::TCustomEntitiesResult& customEntitiesProto,
                                   const NBg::NProto::TAliceEntitiesCollectorResult& entitiesCollectorResult);

} // namespace NAlice
