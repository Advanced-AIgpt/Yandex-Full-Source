#pragma once

#include "state.h"
#include <alice/nlu/granet/lib/sample/sample.h>

namespace NGranet {

// ~~~~ TSimpleIdGenerator ~~~~

template <class Key>
class TSimpleIdGenerator : public TMoveOnly {
public:
    ui32 EnsureId(const Key& key) {
        const auto& [it, isNew] = Map.try_emplace(key, static_cast<ui32>(Map.size()));
        return it->second;
    }

private:
    THashMap<Key, ui32> Map;
};

// ~~~~ TParserStateDumper ~~~~

// Print parser states as table.
class TParserStateDumper : public TMoveOnly {
public:
    void DumpStateList(bool isFiller, int pos, const TParserStateList& list, IOutputStream* log, const TString& indent = "");
    void DumpState(const TParserState& state, IOutputStream* log, const TString& indent = "");

private:
    void DumpElements(const TParserStateList& list, IOutputStream* log, const TString& indent = "");
    void DumpStates(const TParserStateList& list, IOutputStream* log, const TString& indent = "");
    static TString PrintElement(const TGrammarElement& element);

private:
    TSimpleIdGenerator<TParserStateKey> StateKeyIds;
    TSimpleIdGenerator<TSearchIterator<TRuleTrie>> TrieIteratorIds;
};

} // namespace NGranet
