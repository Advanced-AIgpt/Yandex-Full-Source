#pragma once

#include <library/cpp/langmask/langmask.h>
#include <kernel/lemmer/dictlib/grambitset.h>

namespace NGranet {

// TODO(samoylovboris) Make inflection improvements:
//      - (low) Cached inflector (key is chain of TTokenId).
//      - (low) Hint from user about source grammemes.
//      - (low) Hint from user about main word in phrase.
//      - (medium) Generate all possible iflection variants for different results of detection of
//        pluralization, main word, source grammemes, lemmas, etc.

TUtf16String InflectPhrase(TWtringBuf phrase, ELanguage lang, const TGramBitSet& destGrams);
TString InflectPhrase(TStringBuf phrase, ELanguage lang, const TGramBitSet& destGrams);

} // namespace NGranet
