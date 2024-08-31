#pragma once

#include <alice/nlu/libs/entity_searcher/entity_searcher_types.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/langs/langs.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

namespace NAlice::NNlu {

struct TEntityConfig {
    struct TValue {
        TString Value;
        TVector<TString> Samples;
    };
    TString Name;
    bool Inflect = false;
    bool InflectNumbers = false;
    TVector<TValue> Values;
    ELanguage Language = ELanguage::LANG_RUS;
};

void ReadEntitiesFromJson(const NJson::TJsonValue& entitiesJson, TVector<TEntityConfig>* entities);

TVector<TEntityString> MakeEntityStrings(const TMaybe<THashSet<TString>>& entityTypes,
                                         const TVector<TEntityConfig>& entities);

TBlob BuildOccurrenceSearcherData(const TVector<TEntityString>& customEntityStrings);

} // namespace NAlice::NNlu
