#include "collected_entities.h"

namespace NAlice {

namespace {

void ExtractCustomEntities(const NBg::NProto::TCustomEntitiesResult& customEntitiesProto, TCollectedEntities* out) {
    const auto& occurrenceRanges = customEntitiesProto.GetOccurrences().GetRanges();
    const auto& occurrenceValues = customEntitiesProto.GetValues();

    for (int occurrenceIndex = 0; occurrenceIndex < occurrenceRanges.size(); ++occurrenceIndex) {
        const NBg::NProto::TRange& occurrenceRange = occurrenceRanges[occurrenceIndex];
        const NAlice::NNlu::TCustomEntityValues& values = occurrenceValues[occurrenceIndex];

        TTokenRange range{occurrenceRange.GetBegin(), occurrenceRange.GetEnd()};
        for (const NAlice::NNlu::TCustomEntityValue& value : values.GetCustomEntityValues()) {
            (*out)[value.GetType()].emplace(range, value.GetValue());
        }
    }
}

void ExtractAliceEntitiesCollectorEntities(const NBg::NProto::TAliceEntitiesCollectorResult& entitiesCollectorResult,
                                           TCollectedEntities* out) {
    for (const auto& entity : entitiesCollectorResult.GetEntities()) {
        (*out)[entity.GetType()].emplace(TTokenRange{entity.GetBegin(), entity.GetEnd()}, entity.GetValue());
    }
}

}  // namespace

TCollectedEntities CollectEntities(const NBg::NProto::TCustomEntitiesResult& customEntitiesProto,
                                   const NBg::NProto::TAliceEntitiesCollectorResult& entitiesCollectorResult) {
    TCollectedEntities result;
    ExtractAliceEntitiesCollectorEntities(entitiesCollectorResult, &result);
    ExtractCustomEntities(customEntitiesProto, &result);
    return result;
}

} // namespace NAlice
