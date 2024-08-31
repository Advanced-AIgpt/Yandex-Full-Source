#include "inflector.h"
#include <kernel/inflectorlib/phrase/complexword.h>
#include <library/cpp/string_utils/indent_text/indent_text.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/vector.h>

namespace NGranet {

static const TUtf16String OUTPUT_DELIMITER = u" ";

TUtf16String InflectPhrase(TWtringBuf phrase, ELanguage lang, const TGramBitSet& destGrams) {
    NInfl::TSimpleAutoColloc colloc;
    for (TWtringBuf word : StringSplitter(phrase).Split(u' ').SkipEmpty()) {
        colloc.AddWord(NInfl::TComplexWord(lang, word), false);
    }

    colloc.GuessOneLang(); // TODO(samoylovboris) Try to remove
    colloc.GuessMainWord();
    colloc.ReAgree();

    TVector<TUtf16String> inflected;
    if (!colloc.Inflect(destGrams, inflected)) {
        return {};
    }
    return ::JoinStrings(inflected, OUTPUT_DELIMITER);
}

TString InflectPhrase(TStringBuf phrase, ELanguage lang, const TGramBitSet& destGrams) {
    return WideToUTF8(InflectPhrase(UTF8ToWide(phrase), lang, destGrams));
}

} // namespace NGranet
