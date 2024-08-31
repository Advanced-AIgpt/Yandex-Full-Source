#pragma once

#include "entity_searcher.h"
#include "entity_searcher_types.h"

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
 
namespace NAlice::NNlu {
 
class TEntitySearcherDataBuilder {
public:
    TEntitySearcherDataBuilder() = default;
 
    TEntitySearcherData Build(TVector<TEntityString> entities);
 
private:
    THashMap<TString, TTokenId> StringToId;
    TVector<TTokenId> GetIndexes(const TString& str);
};
 
} // namespace NAlice::NNlu
