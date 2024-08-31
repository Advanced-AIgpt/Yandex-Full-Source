#include "entity_collecting.h"

#include "entity_to_proto.h"

#include <alice/nlu/granet/lib/sample/entity_utils.h>

#include <util/generic/algorithm.h>
#include <util/string/split.h>

#include <functional>

namespace NBg::NAliceEntityCollector {

TVector<TString> CollectFstEntities(const ::google::protobuf::RepeatedPtrField<NAlice::NNlu::TFstEntity>& srcEntities,
                                    std::function<void(NGranet::TEntity)> onConverted) {
    if (AllOf(srcEntities, [](const auto& entity) {return entity.GetType().empty();})) {
        return {};
    }
    TVector<TString> tokens;
    for (const NAlice::NNlu::TFstEntity& srcEntity : srcEntities) {
        const size_t entityBegin = tokens.size();
        const TString& token = srcEntity.GetStringValue();
        for (const TStringBuf subToken : StringSplitter(token).Split(' ')) {
            tokens.push_back(TString(subToken));
        }
        // FST rules save padding as TEntity with empty type.
        if (srcEntity.GetType().empty()) {
            continue;
        }

        NGranet::TEntity entity;
        entity.Interval.Begin = entityBegin;
        entity.Interval.End = tokens.size();
        entity.Type = srcEntity.GetType();
        entity.Value = srcEntity.GetValue();
        entity.Quality = srcEntity.GetWeight();
        entity.LogProbability = NGranet::NEntityLogProbs::SYS;

        onConverted(std::move(entity));
    }

    return tokens;
}

void CollectFstEntities(const ::google::protobuf::RepeatedPtrField<NAlice::NNlu::TFstEntity>& srcEntities,
                        TAlignedEntities* alignedEntities, bool IsPASkills) {
    Y_ENSURE(alignedEntities);
    TVector<NGranet::TEntity> entities;
    auto onConverted = [&entities, &IsPASkills](NGranet::TEntity&& entity) {
        TString srcType = entity.Type;

        srcType.to_lower();
        entity.Type = NGranet::NEntityTypePrefixes::SYS + srcType;
        entities.emplace_back(entity);

        // Deprecated prefix
        entity.Type = NGranet::NEntityTypePrefixes::FST + srcType;
        if (NGranet::ALLOWED_FST_TYPES.contains(entity.Type)) {
            entities.emplace_back(entity);
        }

        if (IsPASkills) {
            if (srcType == TStringBuf("num") || srcType == TStringBuf("float")) {
                entity.Type = NGranet::NEntityTypes::PA_SKILLS_NUMBER;
                entities.emplace_back(entity);
            }
        }
    };

    const TVector<TString> tokens = CollectFstEntities(srcEntities, onConverted);
    alignedEntities->AddEntities(tokens, std::move(entities));
}

void CollectAliceEntities(const ::google::protobuf::RepeatedPtrField<NProto::TAliceEntity>& srcEntities,
                          const TString& text, TAlignedEntities* alignedEntities) {
    Y_ENSURE(alignedEntities);
    TVector<NGranet::TEntity> entities;
    for (const NProto::TAliceEntity& srcEntity : srcEntities) {
        ReadEntity(srcEntity, &entities.emplace_back());
    }
    if (entities.empty()) {
        return;
    }
    const TVector<TString> tokens = StringSplitter(text).Split(' ').template ToList<TString>();
    alignedEntities->AddEntities(tokens, std::move(entities));
}

} // namespace NBg::NAliceEntityCollector
