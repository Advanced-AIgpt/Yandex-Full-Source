#include "entity_searcher_builder.h"

#include "entity_searcher_utils.h"

#include <library/cpp/containers/comptrie/comptrie_builder.h>

#include <util/generic/algorithm.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/string/split.h>
#include <util/system/yassert.h>

namespace NAlice::NNlu {

namespace{

TBlob Save(const TTrie::TBuilder& builder) {
    TBufferStream buffer;
    builder.Save(buffer);
    return TBlob::FromStream(buffer);
}

TVector<TString> ReverseStringToId(const THashMap<TString, TTokenId>& stringToId) {
    TVector<TString> idToString(stringToId.size());
    for (auto& [key, value] : stringToId) {
        idToString[value] = key;
    }
    return idToString;
}

} // namespace

TVector<TTokenId> TEntitySearcherDataBuilder::GetIndexes(const TString& str) {
    TVector<TTokenId> result;
    for (const auto& it : StringSplitter(str).Split(' ')) {
        result.push_back(UpdateStringToId(it.Token(), &StringToId));
    }
    return result;
}

TEntitySearcherData TEntitySearcherDataBuilder::Build(TVector<TEntityString> entityStrings) {
    THashMap<TStringBuf, TVector<size_t>> map;
    for (size_t id = 0; id < entityStrings.size(); ++id) {
        map[entityStrings[id].Sample].push_back(id);
    }
    TTrie::TBuilder builder;
    for (auto& [sample, entityStringsIndexes] : map) {
        builder.Add(GetIndexes(entityStrings[entityStringsIndexes[0]].Sample), std::move(entityStringsIndexes));
    }
    return {Save(builder), std::move(entityStrings), ReverseStringToId(StringToId)};
}

} // namespace NAlice::NNlu
