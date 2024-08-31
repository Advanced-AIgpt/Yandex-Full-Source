#include "trie.h"

using namespace NDict::NUtil;

namespace NAlice::NBeggins::NBertTfApplier {
TTrie::TTrie(std::unique_ptr<WTrie> trie)
    : Trie(std::move(trie)) {
    Y_ENSURE(Trie);
}

std::unique_ptr<TTrie> TTrie::FromFile(const TString& filename) {
    return std::make_unique<TTrie>(std::unique_ptr<WTrie>(WTrie::FromFile(filename)));
}
} // namespace NAlice::NBeggins::NBertTfApplier
