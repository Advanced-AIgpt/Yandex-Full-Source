#include "token_id.h"
#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <util/string/builder.h>

namespace NGranet {

// ~~~~ TWordTrie ~~~~

TTokenId GetWordTokenId(const TWordTrie& trie, TStringBuf token, bool isLemma) {
    ui32 index = 0;
    if (!trie.Find(token, &index)) {
        return TOKEN_UNKNOWN;
    }
    return NTokenId::FromWordIndex(index, isLemma);
}

// ~~~~ TTokenPool ~~~~

TTokenPool::TTokenPool() {
    AddPredefined("<<Unknown>>", TOKEN_UNKNOWN);
    AddPredefined(".", TOKEN_WILDCARD);
    // Fixed id of empty string. For debugging.
    AddPredefined("", 2);
}

TTokenPool::TTokenPool(const TWordTrie& trie) {
    if (trie.IsEmpty()) {
        return;
    }
    FreeIndex = 0;
    for (const auto& [word, id] : trie) {
        WordToIndex.Add(word, id);
        IndexToWord[id] = word;
        FreeIndex = Max(FreeIndex, id);
    }
    FreeIndex++;
}

void TTokenPool::AddPredefined(TStringBuf word, TTokenId expectedId) {
    const TTokenId id = AddWord(word);
    Y_ENSURE(id == expectedId);
}

TTokenId TTokenPool::AddWord(TStringBuf word, bool isLemma) {
    return NTokenId::FromWordIndex(AddWord(word), isLemma);
}

ui32 TTokenPool::AddWord(TStringBuf word) {
    ui32 id = 0;
    if (WordToIndex.Find(word, &id)) {
        return id;
    }
    id = FreeIndex;
    FreeIndex++;
    WordToIndex.Add(word, id);
    IndexToWord.emplace(id, word);
    return id;
}

const TString& TTokenPool::GetWord(TTokenId id) const {
    return IndexToWord.at(NTokenId::ToWordIndex(id));
}

void TTokenPool::BuildTrie(TWordTrie* trie) const {
    Y_ENSURE(trie);
    TBufferOutput buffer;
    WordToIndex.Save(buffer);
    trie->Init(TBlob::FromBuffer(buffer.Buffer()));
}

void TTokenPool::Dump(IOutputStream* log, bool isDeep, const TString& indent) const {
    Y_ENSURE(log);
    size_t counter = 0;
    for (const TTokenId id : OrderedSetOfKeys(IndexToWord)) {
        *log << indent;
        if (!isDeep && counter >= 5) {
            *log << "..." << Endl;
            break;
        }
        *log << id << ": " << IndexToWord.at(id) << Endl;
        counter++;
    }
}

TString TTokenPool::PrintWordToken(TTokenId id) const {
    if (id == TOKEN_WILDCARD) {
        return ".";
    }
    const char prefix = NTokenId::IsLemmaWord(id) ? '~' : '=';
    return TString::Join(prefix, GetWord(id));
}

} // namespace NGranet
