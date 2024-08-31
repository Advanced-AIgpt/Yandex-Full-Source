#include "entity_searcher_utils.h"

namespace NAlice::NNlu {

TTokenId UpdateStringToId(TStringBuf s, THashMap<TString, TTokenId>* stringToId) {
    return stringToId->insert({TString(s), stringToId->size()}).first->second;
}

TTokenId GetIdFromString(TStringBuf s, const THashMap<TString, TTokenId>& stringToId) {
    return stringToId.contains(s) ? stringToId.at(s) : UNKNOWN_TOKEN;
}

} // namespace NAlice::NNlu
