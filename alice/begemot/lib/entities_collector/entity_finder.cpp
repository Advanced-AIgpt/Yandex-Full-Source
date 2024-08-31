#include "entity_finder.h"

#include <alice/nlu/granet/lib/sample/entity_utils.h>

#include <util/generic/algorithm.h>
#include <util/string/split.h>

namespace NBg::NAliceEntityCollector {

namespace {

constexpr size_t ENTITY_FINDER_BEGIN_FIELD_INDEX = 1;
constexpr size_t ENTITY_FINDER_END_FIELD_INDEX = 2;
constexpr size_t ENTITY_FINDER_VALUE_FIELD_INDEX = 3;
constexpr size_t ENTITY_FINDER_QUALITY_FIELD_INDEX = 4;
constexpr size_t ENTITY_FINDER_TYPE_FIELD_INDEX = 5;

NGranet::TEntity ParseEntity(const TStringBuf match) {
    NGranet::TEntity entity;

    TVector<TString> fields;
    StringSplitter(match).Split('\t').Collect(&fields);

    entity.Interval.Begin = FromString(fields[ENTITY_FINDER_BEGIN_FIELD_INDEX]);
    entity.Interval.End = FromString(fields[ENTITY_FINDER_END_FIELD_INDEX]);

    entity.Value = fields[ENTITY_FINDER_VALUE_FIELD_INDEX];
    entity.Quality = FromString(fields[ENTITY_FINDER_QUALITY_FIELD_INDEX]);
    entity.LogProbability = NGranet::NEntityLogProbs::ENTITY_SEARCH;

    const TString ontoCategory = fields[ENTITY_FINDER_TYPE_FIELD_INDEX];
    entity.Type = NGranet::NEntityTypePrefixes::ENTITY_SEARCH + ontoCategory;

    return entity;
}

void FilterWorseEntitiesWithSameInterval(TVector<NGranet::TEntity>& entities) {
    Sort(entities, [](const NGranet::TEntity& lhs, const NGranet::TEntity& rhs) {
        return std::tie(lhs.Interval, lhs.Quality) > std::tie(rhs.Interval, rhs.Quality);
    });

    NNlu::TInterval previousEntityInterval = {NPOS, NPOS};

    size_t oldEntitiesPosition = 0;
    size_t newEntitiesPosition = 0;

    while (oldEntitiesPosition < entities.size()) {
        const auto& entityInterval = entities[oldEntitiesPosition].Interval;

        if (previousEntityInterval != entityInterval) {
            Y_ENSURE(newEntitiesPosition <= oldEntitiesPosition);

            if (oldEntitiesPosition != newEntitiesPosition) {
                entities[newEntitiesPosition] = entities[oldEntitiesPosition];
            }

            previousEntityInterval = entityInterval;
            newEntitiesPosition += 1;
        }

        oldEntitiesPosition += 1;
    }

    entities.resize(newEntitiesPosition);
}

} // namespace

TVector<NGranet::TEntity> CollectEntityFinder(const NProto::TEntityFinderResult& entityFinderResult) {
    const auto& matches = entityFinderResult.GetWinner().empty()
        ? entityFinderResult.GetMatch() : entityFinderResult.GetWinner();

    TVector<NGranet::TEntity> entities;
    for (const TString& match : matches) {
        entities.push_back(ParseEntity(match));
    }

    FilterWorseEntitiesWithSameInterval(entities);

    return entities;
}

} // namespace NBg::NAliceEntityCollector
