#include "entities_collector.h"
#include "entity_to_proto.h"
#include "date.h"
#include "entity_finder.h"
#include "fio.h"
#include "geo.h"
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>
#include <library/cpp/scheme/scheme.h>
#include <util/generic/vector.h>
#include <util/string/split.h>

namespace NBg::NAliceEntityCollector {

// ~~~~ TEntitiesCollector ~~~~

TEntitiesCollector::TEntitiesCollector(const TVector<TString>& tokens)
    : AlignedEntities(tokens)
{
}

void TEntitiesCollector::SetIsPASkills(bool value) {
    IsPASkills = value;
}

void TEntitiesCollector::CollectNonsense(const NProto::TAliceNonsenseTaggerResult* srcResult) {
    if (!srcResult) {
        return;
    }
    TVector<NGranet::TEntity> entities;
    for (const NProto::TAliceNonsenseEntityHypothesis& srcEntity : srcResult->GetEntityHypotheses()) {
        if (srcEntity.GetProb() < srcResult->GetEntityThresholdOption()) {
            continue;
        }
        const NNlu::TInterval interval = {srcEntity.GetBegin(), srcEntity.GetEnd()};
        Y_ENSURE(interval.Length() > 0);

        NGranet::TEntity entity;
        entity.Interval = {srcEntity.GetBegin(), srcEntity.GetEnd()};
        Y_ENSURE(entity.Interval.Length() > 0);
        entity.Quality = srcEntity.GetProb();
        entity.LogProbability = entity.Interval.Length() * NGranet::NEntityLogProbs::NONSENSE_WORD;
        entity.Type = NGranet::NEntityTypes::NONSENSE;

        entities.emplace_back(entity);
        if (IsPASkills) {
            entity.Type = NGranet::NEntityTypes::PA_SKILLS_NONSENSE;
            entities.emplace_back(entity);
        }
    }
    if (entities.empty()) {
        return;
    }

    // Align entities
    TVector<TString> tokens;
    for (const NProto::TAliceNonsenseTaggerToken& token : srcResult->GetToken()) {
        tokens.push_back(token.GetText());
    }
    AlignedEntities.AddEntities(tokens, std::move(entities));
}

void TEntitiesCollector::CollectCustomEntities(const NProto::TCustomEntitiesResult* srcResult) {
    if (!srcResult) {
        return;
    }

    const auto& srcRanges = srcResult->GetOccurrences().GetRanges();
    const auto& srcMaps = srcResult->GetValues();
    Y_ENSURE(srcMaps.size() == srcRanges.size());
    if (srcRanges.empty()) {
        return;
    }

    TVector<NGranet::TEntity> entities(Reserve(srcRanges.size()));
    for (int r = 0; r < srcRanges.size(); ++r) {
        const NProto::TRange& srcRange = srcRanges[r];
        const NAlice::NNlu::TCustomEntityValues& srcMap = srcMaps[r];

        NGranet::TEntity entity;
        entity.Interval = {srcRange.GetBegin(), srcRange.GetEnd()};
        entity.LogProbability = NGranet::NEntityLogProbs::CUSTOM;

        for (const NAlice::NNlu::TCustomEntityValue& srcKeyValue : srcMap.GetCustomEntityValues()) {
            NGranet::TEntity child = entity;
            child.Type = NGranet::NEntityTypePrefixes::CUSTOM + srcKeyValue.GetType();
            child.Value = srcKeyValue.GetValue();
            entities.emplace_back(child);
        }
    }

    // Align entities
    TVector<TString> tokens;
    for (const TString& token : srcResult->GetOccurrences().GetTokens()) {
        tokens.push_back(token);
    }
    AlignedEntities.AddEntities(tokens, std::move(entities));
}

void TEntitiesCollector::CollectAliceTypeParserResult(const NProto::TAliceTypeParserTimeResult* srcResult) {
    if (!srcResult) {
        return;
    }

    TVector<NGranet::TEntity> entities;
    for (const auto& [type, entitiesProto] : srcResult->GetResult().GetParsedEntitiesByType()) {
        for (const auto& entityProto : entitiesProto.GetParsedEntities()) {
            NGranet::TEntity entity;
            entity.Interval.Begin = entityProto.GetStartToken();
            entity.Interval.End = entityProto.GetEndToken();
            entity.Type = NGranet::NEntityTypePrefixes::ALICE_TYPE_PARSER + entityProto.GetType();
            entity.Type.to_lower();
            entity.Value = entityProto.GetValue();
            entity.LogProbability = NGranet::NEntityLogProbs::UNKNOWN; // TODO(smirnovpavel): define LogProbability
            entities.emplace_back(entity);
        }
    }

    const auto& tokensProto = srcResult->GetResult().GetTokens();
    const TVector<TString> tokens(tokensProto.begin(), tokensProto.end());

    AlignedEntities.AddEntities(tokens, std::move(entities));
}

void TEntitiesCollector::CollectExternalMarkupEntities(const NProto::TExternalMarkupProto& markup,
                                                       const TVector<TString>& tokens)
{
    AlignedEntities.AddEntities(tokens, CollectGeo(markup));
    if (IsPASkills) {
        AlignedEntities.AddEntities(tokens, CollectPASkillsGeo(markup));
        AlignedEntities.AddEntities(tokens, CollectPASkillsDate(markup));
        AlignedEntities.AddEntities(tokens, CollectPASkillsFio(markup));
    }
}

// static
TVector<TString> TEntitiesCollector::CollectExternalMarkupTokens(const NProto::TExternalMarkupProto& markup) {
    TVector<TString> tokens(Reserve(markup.GetTokens().size()));
    for (const NProto::TExternalMarkupProto_TToken& token : markup.GetTokens()) {
        tokens.push_back(token.GetText());
    }
    return tokens;
}

void TEntitiesCollector::CollectGranetEntities(const NProto::TGranetResult* srcResult) {
    if (!srcResult) {
        return;
    }
    TVector<NGranet::TEntity> entities;
    for (const NProto::TGranetEntity& srcEntity : srcResult->GetAllEntities()) {
        if (srcEntity.GetSource() == NGranet::NEntitySources::GRANET) {
            ReadEntity(srcEntity, &entities.emplace_back());
        }
    }
    if (entities.empty()) {
        return;
    }
    TVector<TString> tokens;
    for (const NProto::TGranetToken& token : srcResult->GetTokens()) {
        tokens.push_back(token.GetText());
    }
    AlignedEntities.AddEntities(tokens, std::move(entities));
}

void TEntitiesCollector::CollectEntityFinderEntities(const NProto::TEntityFinderResult* entityFinderResult,
                                                     const TVector<TString>& tokens)
{
    if (!entityFinderResult) {
        return;
    }
    AlignedEntities.AddEntities(tokens, CollectEntityFinder(*entityFinderResult));
}

void TEntitiesCollector::FindWorkaroundEntities(const NProto::TAliceRequest* request) {
    if (request == nullptr) {
        return;
    }
    const TString text = request->GetProcessedText();
    const TVector<TString> tokens = StringSplitter(text).Split(' ').SkipEmpty().ToList<TString>();
    TVector<NGranet::TEntity> entities;
    TWorkaroundEntitiesFinder(tokens, IsPASkills, &entities).Find();
    AlignedEntities.AddEntities(tokens, std::move(entities));
}

const TAlignedEntities& TEntitiesCollector::GetAlignedEntities() const {
    return AlignedEntities;
}

} // namespace NBg::NAliceEntityCollector
