#include "syntax.h"
#include "compiler_check.h"
#include <util/string/escape.h>

namespace NGranet::NCompiler {

// ~~~~ Utils ~~~~

void ReadSynonyms(const TTextView& source, TStringBuf line, ESynonymFlags& synonymFlagsMask, ESynonymFlags& synonymFlags, bool isEnable) {
    const TVector<TStringBuf> synonymTypes = StringSplitter(line).Split(',').ToList<TStringBuf>();
    GRANET_COMPILER_CHECK(!synonymTypes.empty(), source, MSG_SYNONYM_TYPES_ARE_EMPTY);
    for (TStringBuf synonymType : synonymTypes) {
        const ESynonymFlags* diff = SynonymInfoTable.FindPtr(StripString(synonymType));
        GRANET_COMPILER_CHECK(diff, source, MSG_UNKNOWN_SYNONYM_TYPE, synonymType);
        synonymFlagsMask |= *diff;
        if (isEnable) {
            synonymFlags |= *diff;
        } else {
            synonymFlags &= ~*diff;
        }
    }
}

TString SafeUnquote(const TTextView& source, TStringBuf str, const TRegExMatch* notQuotedStringCheck) {
    str = StripString(str);
    if (!TryRemoveBraces(&str, '"', '"')) {
        if (notQuotedStringCheck != nullptr && !source.GetSourceText()->IsCompatibilityMode) {
            GRANET_COMPILER_CHECK(notQuotedStringCheck->Match(TString(str).c_str()), source,
                MSG_NOT_QUOTED_STRING_PARSING_ERROR, str);
        }
        return TString(str);
    }
    try {
        return UnescapeC(str);
    } catch (const yexception& e) {
        GRANET_COMPILER_CHECK(false, source, MSG_STRING_LITERAL_PARSER_ERROR, e.what());
    }
    return "";
}

TString SafeUnquote(const TTextView& source, const TRegExMatch* notQuotedStringCheck) {
    return SafeUnquote(source, source.Str(), notQuotedStringCheck);
}

} // namespace NGranet::NCompiler
