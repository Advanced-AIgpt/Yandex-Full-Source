#pragma once

#include <dict/libs/trie/trie.h>
#include <dict/libs/trie/virtual_trie_iterator.h>

namespace NAlice::NBeggins::NBertTfApplier {
class TTrie {
public:
    TTrie(std::unique_ptr<NDict::NUtil::WTrie> trie);
    template <class TStrFwdIter>
    size_t MaxMatchingPrefixLen(TStrFwdIter begin, TStrFwdIter end);
    static std::unique_ptr<TTrie> FromFile(const TString& path);

private:
    std::unique_ptr<NDict::NUtil::WTrie> Trie;
};

template <class TStrFwdIter>
size_t TTrie::MaxMatchingPrefixLen(TStrFwdIter begin, TStrFwdIter end) {
    auto trieIter = WVirtualTrieIterator(Trie->GetRoot());
    size_t matchLength = 0;
    size_t maxMatchLength = 0;
    while (begin != end) {
        auto current = begin++;
        if (!trieIter.Descend(current->data(), Trie.get())) {
            break;
        }
        ++matchLength;
        if (trieIter.IsTerminal()) {
            maxMatchLength = matchLength;
        }
    }
    return maxMatchLength;
}
} // namespace NAlice::NBeggins::NBertTfApplier
