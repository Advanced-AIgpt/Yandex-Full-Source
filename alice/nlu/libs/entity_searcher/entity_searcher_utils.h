#pragma once

#include "entity_searcher_types.h"

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NNlu {

TTokenId UpdateStringToId(TStringBuf s, THashMap<TString, TTokenId>* stringToId);

TTokenId GetIdFromString(TStringBuf s, const THashMap<TString, TTokenId>& stringToId);

template <typename Container>
TVector<TTokenId> GetTokenIds(const Container& container, const THashMap<TString, TTokenId>* stringToId) {
    TVector<TTokenId> result;
    for (const auto& s : container) {
        result.push_back(GetIdFromString(s, *stringToId));
    }
    return result;
}

} // namespace NAlice::NNlu
