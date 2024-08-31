#pragma once

#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>
#include <alice/nlu/libs/interval/interval.h>
#include <library/cpp/dbg_output/dump.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/generic/vector.h>

namespace NGranet {

// ~~~~ TEntity ~~~~

// Entity occurrence.
struct TEntity {
    // Position in text (indexes of tokens or chars)
    NNlu::TInterval Interval;
    // Type of entity.
    TString Type;
    // Value which identifies this instance of entity.
    // Example: id of playlist (text is name of playlist, 'Value' is id of playlist), normalized date, normalized number.
    TString Value;
    // Some flags joined by comma (see NEntityFlags). Used as additional information for parser.
    TString Flags;
    // Name of subsystem which produced this entity (see NEntitySources).
    TString Source;
    // Bayesian log probability for Granet (it's not probability or quality of entity).
    double LogProbability = 0;
    // Quality of named entity.
    // Some entity finders can supply confidence of entity which can be used in other classifiers.
    // Quality not normalized, it's just a number. Some finders emit float value in range [0..1],
    // some finders - integer value in range [0..inf), some finders don't set this field.
    double Quality = 0;

    DECLARE_TUPLE_LIKE_TYPE(TEntity, Interval, Type, Value, Flags, Source, LogProbability, Quality);

    static TEntity FromJson(const NJson::TJsonValue& json);
    NJson::TJsonValue ToJson() const;
};

// ~~~~ TEntity functions ~~~~

NJson::TJsonValue SaveEntitiesToJsonCompact(const TVector<TEntity>& entities);
TVector<TEntity> LoadEntitiesFromJson(const NJson::TJsonValue& json);

// Remove entities with invalid interval (in case of changes in tokenization algorithm).
void RemoveInvalidEntities(size_t tokenCount, TVector<TEntity>* entities);

} // namespace NGranet

DEFINE_DUMPER(NGranet::TEntity, Interval, Type, Value, Flags, Source, LogProbability, Quality);

inline IOutputStream& operator<<(IOutputStream& out, const NGranet::TEntity& entity) {
    out << DbgDump(entity);
    return out;
}

template <>
struct THash<NGranet::TEntity>: public TTupleLikeTypeHash {
};
