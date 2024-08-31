#pragma once

#include "literals.h"
#include "trie.h"

#include <dict/libs/segmenter/segmenter.h>
#include <dict/libs/trie/trie.h>

#include <util/generic/vector.h>

namespace NAlice::NBeggins::NBertTfApplier {

class TTokenizer {
public:
    struct TResult {
        TVector<TUtf32String> Tokens;
        TVector<bool> IsContinuation;
    };

    TTokenizer(std::unique_ptr<TTrie> startTrie, std::unique_ptr<TTrie> continuationTrie);
    TResult Tokenize(const TUtf32String& text) const;

private:
    std::unique_ptr<TTrie> StartTrie;
    std::unique_ptr<TTrie> ContiuationTrie;
    std::unique_ptr<NDict::NSegmenter::ISegmenter> BaseSegmenter;
};
} // namespace NAlice::NBeggins::NBertTfApplier
