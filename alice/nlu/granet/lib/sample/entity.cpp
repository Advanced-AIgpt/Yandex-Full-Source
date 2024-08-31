#include "entity.h"
#include <alice/nlu/granet/lib/utils/json_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>

using namespace NJson;

namespace NGranet {

namespace NScheme {
    static const TStringBuf Type = "Type";
    static const TStringBuf Name = "Name";
    static const TStringBuf Begin = "Begin";
    static const TStringBuf End = "End";
    static const TStringBuf Value = "Value";
    static const TStringBuf Flags = "Flags";
    static const TStringBuf Source = "Source";
    static const TStringBuf LogProbability = "LogProbability";
    static const TStringBuf Quality = "Quality";
}

// static
TEntity TEntity::FromJson(const TJsonValue& json) {
    TEntity entity;
    entity.Type = json[NScheme::Type].GetStringSafe();
    entity.Interval.Begin = json[NScheme::Begin].GetUIntegerSafe();
    entity.Interval.End = json[NScheme::End].GetUIntegerSafe();
    entity.Value = json[NScheme::Value].GetStringSafe("");
    entity.Flags = json[NScheme::Flags].GetStringSafe("");
    entity.Source = json[NScheme::Source].GetStringSafe("");
    entity.LogProbability = json[NScheme::LogProbability].GetDoubleSafe(0);
    entity.Quality = json[NScheme::Quality].GetDoubleSafe(0);
    Y_ENSURE_EX(entity.Interval.Valid(), (TJsonException() << "Invalid entity."));
    return entity;
}

TJsonValue TEntity::ToJson() const {
    TJsonValue json;
    json[NScheme::Type] = Type;
    json[NScheme::Begin] = Interval.Begin;
    json[NScheme::End] = Interval.End;
    json[NScheme::Value] = Value;
    json[NScheme::Flags] = Flags;
    json[NScheme::Source] = Source;
    json[NScheme::LogProbability] = LogProbability;
    json[NScheme::Quality] = Quality;
    return json;
}

TJsonValue SaveEntitiesToJsonCompact(const TVector<TEntity>& entities) {
    TJsonValue json;
    TJsonValue::TArray& types = json[NScheme::Type].SetType(JSON_ARRAY).GetArraySafe();
    TJsonValue::TArray& beginnings = json[NScheme::Begin].SetType(JSON_ARRAY).GetArraySafe();
    TJsonValue::TArray& endings = json[NScheme::End].SetType(JSON_ARRAY).GetArraySafe();
    TJsonValue::TArray& values = json[NScheme::Value].SetType(JSON_ARRAY).GetArraySafe();
    TJsonValue::TArray& flags = json[NScheme::Flags].SetType(JSON_ARRAY).GetArraySafe();
    TJsonValue::TArray& sources = json[NScheme::Source].SetType(JSON_ARRAY).GetArraySafe();
    TJsonValue::TArray& logprobs = json[NScheme::LogProbability].SetType(JSON_ARRAY).GetArraySafe();
    TJsonValue::TArray& qualities = json[NScheme::Quality].SetType(JSON_ARRAY).GetArraySafe();
    for (const TEntity& entity : entities) {
        types.push_back(entity.Type);
        beginnings.push_back(entity.Interval.Begin);
        endings.push_back(entity.Interval.End);
        values.push_back(entity.Value);
        flags.push_back(entity.Flags);
        sources.push_back(entity.Source);
        logprobs.push_back(entity.LogProbability);
        qualities.push_back(entity.Quality);
    }
    return json;
}

// For Begemot response
TVector<TEntity> LoadEntitiesFromBegemotJson(const TJsonValue& json) {
    TVector<TEntity> entities(Reserve(json.GetArray().size()));
    for (const TJsonValue& entityJson : json.GetArray()) {
        entities.push_back(TEntity::FromJson(entityJson));
    }
    return entities;
}

TVector<TEntity> LoadEntitiesFromDatasetJson(const TJsonValue& json) {
    const TStringBuf typeKey = json.Has(NScheme::Type) ? NScheme::Type : NScheme::Name;
    const TJsonValue::TArray& types = json[typeKey].GetArraySafe();
    const TJsonValue::TArray& beginnings = json[NScheme::Begin].GetArraySafe();
    const TJsonValue::TArray& endings = json[NScheme::End].GetArraySafe();
    const TJsonValue::TArray& values = json[NScheme::Value].GetArray();
    const TJsonValue::TArray& flags = json[NScheme::Flags].GetArray();
    const TJsonValue::TArray& sources = json[NScheme::Source].GetArray();
    const TJsonValue::TArray& logprobs = json[NScheme::LogProbability].GetArray();
    const TJsonValue::TArray& qualities = json[NScheme::Quality].GetArray();

    const size_t count = types.size();
    Y_ENSURE_EX(beginnings.size() == count
        && endings.size() == count
        && (values.empty() || values.size() == count) // optional
        && (flags.empty() || flags.size() == count) // optional
        && (sources.empty() || sources.size() == count) // optional
        && (logprobs.empty() || logprobs.size() == count) // optional
        && (qualities.empty() || qualities.size() == count), // optional
        (TJsonException() << "Invalid entities."));

    TVector<TEntity> entities(Reserve(count));
    for (size_t i = 0; i < count; ++i) {
        TEntity entity;
        entity.Type = types[i].GetStringSafe();
        entity.Interval.Begin = beginnings[i].GetUIntegerSafe();
        entity.Interval.End = endings[i].GetUIntegerSafe();
        entity.Value = !values.empty() ? values[i].GetStringSafe() : "";
        entity.Flags = !flags.empty() ? flags[i].GetStringSafe() : "";
        entity.Source = !sources.empty() ? sources[i].GetStringSafe() : "";
        entity.LogProbability = !logprobs.empty() ? logprobs[i].GetDoubleSafe() : 0;
        entity.Quality = !qualities.empty() ? qualities[i].GetDoubleSafe() : 0;
        Y_ENSURE_EX(entity.Interval.Valid(), (TJsonException() << "Invalid entities."));
        entities.push_back(std::move(entity));
    }
    return entities;
}

TVector<TEntity> LoadEntitiesFromJson(const TJsonValue& json) {
    if (!json.IsDefined()) {
        return {};
    }
    if (json.IsArray()) {
        return LoadEntitiesFromBegemotJson(json);
    }
    if (json.IsMap()) {
        return LoadEntitiesFromDatasetJson(json);
    }
    Y_ENSURE_EX(false, (TJsonException() << "Invalid entities."));
    return {};
}

void RemoveInvalidEntities(size_t tokenCount, TVector<TEntity>* entities) {
    Y_ENSURE(entities);
    EraseIf(*entities, [tokenCount] (const TEntity& entity) {
        return entity.Interval.End > tokenCount;
    });
}

} // namespace NGranet
