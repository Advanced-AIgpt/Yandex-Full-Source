#pragma once

#include "string_pool.h"
#include <library/cpp/containers/comptrie/comptrie.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NGranet {

// ~~~~ TElementId ~~~~

// TGrammarElement id (is equal to index of element in TGrammar).
using TElementId = ui32;

const TElementId UNDEFINED_ELEMENT_ID = Max<TElementId>();

// ~~~~ TTokenId ~~~~

using TTokenId = ui32;

// Unknown word (not found in token pool)
const TTokenId TOKEN_UNKNOWN = 0;
// Wildcard - matched to any word.
const TTokenId TOKEN_WILDCARD = 1;

const TTokenId TOKEN_TYPE_MASK = 0xFFu << 24u;
const TTokenId TOKEN_VALUE_MASK = ~TOKEN_TYPE_MASK;

const TTokenId TOKEN_TYPE_SPECIAL = 0;
const TTokenId TOKEN_TYPE_EXACT_WORD = 1u << 24u;
const TTokenId TOKEN_TYPE_LEMMA_WORD = 2u << 24u;
const TTokenId TOKEN_TYPE_ELEMENT = 3u << 24u;
const TTokenId TOKEN_TYPE_SLOT_EDGE = 4u << 24u;

const TTokenId TOKEN_SLOT_BEGIN = TOKEN_TYPE_SLOT_EDGE;

// If you need different types for TTokenId and TElementId, please check all places
// where TRuleTrie is used. After that you can remove these assertions.
static_assert(std::is_same<TElementId, TTokenId>::value, "TTokenId != TElementId");
static_assert(std::is_same<TStringId, TTokenId>::value, "TTokenId != TElementId");

// ~~~~ NTokenId ~~~~

namespace NTokenId {

    // Token type

    inline TTokenId GetType(TTokenId id) {
        return id & TOKEN_TYPE_MASK;
    }

    inline bool IsSpecial(TTokenId id) {
        return GetType(id) == TOKEN_TYPE_SPECIAL;
    }
    inline bool IsExactWord(TTokenId id) {
        return GetType(id) == TOKEN_TYPE_EXACT_WORD;
    }
    inline bool IsLemmaWord(TTokenId id) {
        return GetType(id) == TOKEN_TYPE_LEMMA_WORD;
    }
    inline bool IsElement(TTokenId id) {
        return GetType(id) == TOKEN_TYPE_ELEMENT;
    }
    inline bool IsSlotEdge(TTokenId id) {
        return GetType(id) == TOKEN_TYPE_SLOT_EDGE;
    }

    inline bool IsWord(TTokenId id) {
        return IsExactWord(id) || IsLemmaWord(id);
    }

    // Token data

    inline TElementId ToElementId(TTokenId id) {
        Y_ASSERT(IsElement(id));
        return id & TOKEN_VALUE_MASK;
    }
    inline TTokenId FromElementId(TElementId id) {
        Y_ASSERT((TOKEN_VALUE_MASK & id) == id);
        return id | TOKEN_TYPE_ELEMENT;
    }

    inline ui32 ToWordIndex(TTokenId id) {
        Y_ASSERT(IsWord(id) || IsSpecial(id));
        return id & TOKEN_VALUE_MASK;
    }
    inline TTokenId FromWordIndex(ui32 index, bool isLemma) {
        Y_ASSERT((TOKEN_VALUE_MASK & index) == index);
        return index | (isLemma ? TOKEN_TYPE_LEMMA_WORD : TOKEN_TYPE_EXACT_WORD);
    }

    inline TStringId ToSlotMarkupStringId(TTokenId id) {
        Y_ASSERT(IsSlotEdge(id));
        return id & TOKEN_VALUE_MASK;
    }
    inline TTokenId FromSlotMarkupStringId(TStringId id) {
        Y_ASSERT((TOKEN_VALUE_MASK & id) == id);
        return id | TOKEN_TYPE_SLOT_EDGE;
    }
}

// ~~~~ TWordTrie ~~~~

using TWordTrie = TCompactTrie<char, TTokenId>;

TTokenId GetWordTokenId(const TWordTrie& trie, TStringBuf token, bool isLemma);

// ~~~~ TTokenPool ~~~~

class TTokenPool {
public:
    TTokenPool();
    explicit TTokenPool(const TWordTrie& trie);

    TTokenId AddWord(TStringBuf word, bool isLemma);
    const TString& GetWord(TTokenId id) const;

    void BuildTrie(TWordTrie* trie) const;

    void Dump(IOutputStream* log, bool isDeep = true, const TString& indent = "") const;
    TString PrintWordToken(TTokenId id) const;

private:
    void AddPredefined(TStringBuf token, TTokenId expectedId);
    TTokenId AddWord(TStringBuf word);

private:
    TCompactTrie<char, TTokenId>::TBuilder WordToIndex;
    THashMap<TTokenId, TString> IndexToWord;
    ui32 FreeIndex = 0;
};

} // namespace NGranet
