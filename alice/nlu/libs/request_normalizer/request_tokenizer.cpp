#include "request_tokenizer.h"
#include <util/generic/hash.h>
#include <util/string/subst.h>
#include <util/string/type.h>

namespace NNlu {

static const THashMap<TString, TString> Patterns = {
    // only alphabetic
    {"ALPHA", R"([^\PL\d_]+)"},
    // alphabetic + numbers
    {"ALPHANUM", R"([^\PL_]+)"},
    // multi-words with separator (multi-word, multi/word, etc.)
    // partial words contain only alphabetic symbols (any 1-word or word-1 will be ignored)
    {"COMPOSED_WORD", R"({ALPHA}(?:[\-/\\]{ALPHA})+)"},
    // any number (int, float)
    {"NUMBER", R"((?:\d+[,\.]\d+|\d+))"},
    // negative number -1 only at the start of the token
    {"SIGNED_NUMBER", R"(\-{NUMBER})"},
    // datetime of type dd.mm.yy{yy}
    {"DATE", R"(\d{1,2}[\.,\\/\-]\d{1,2}[\.,\\/\-]\d{2,4})"},
    // time of type hh:mm
    {"TIME", R"(\d{1,2}:\d\d)"},
    // arithmetic operations, if inside tokens, should be surrounded by digits
    {"ARITHMETIC", R"([\+\-/\*=\^%])"},
    // currency symbols
    {"CURRENCY", "[$\u20ac\u20bd]"},
    // special cases with glued punctuation
    // TODO(samoylovboris) What's this? Find example, add to unit-test or remove.
    {"GLUED_PUNCT", R"(\pL\+\+)"},
    // token of utterance
    // common part of TOKEN and GLUED_TOKEN
    {"TOKEN_COMMON", "{GLUED_PUNCT}|{COMPOSED_WORD}|{DATE}|{TIME}|{ARITHMETIC}|{NUMBER}|{ALPHANUM}|{CURRENCY}"},
    // token of utterance
    {"TOKEN", "({TOKEN_COMMON}|{SIGNED_NUMBER})"},
    {"GLUED_TOKEN", "({TOKEN_COMMON})"},
};

// Get preprocessed patterns[name].
// Replaces all occurrences of the {NAME} by the corresponded subexpression.
static TString GetPreprocessedPattern(const THashMap<TString, TString>& patterns, const TString& name) {
    TString result = patterns.at(name);
    size_t depth = 0;
    while (true) {
        TString transformed = result;
        for (const auto& [childName, childExpression] : patterns) {
            if (name != childName && transformed.Contains(childName)) {
                SubstGlobal(transformed, "{" + childName + "}", childExpression);
            }
        }
        if (transformed == result) {
            break;
        }
        result = transformed;
        // Prevent recursion.
        depth++;
        Y_ENSURE(depth < patterns.size());
    }
    return result;
}

static re2::RE2::Options MakeReOptions() {
    re2::RE2::Options opts;
    opts.set_longest_match(true);
    return opts;
}

TRequestTokenizer::TRequestTokenizer()
    : TokenPattern(GetPreprocessedPattern(Patterns, "TOKEN"), MakeReOptions())
    , GluedTokenPattern(GetPreprocessedPattern(Patterns, "GLUED_TOKEN"), MakeReOptions())
{
    Y_ENSURE(TokenPattern.ok());
    Y_ENSURE(GluedTokenPattern.ok());
}

TVector<TStringBuf> TRequestTokenizer::Tokenize(TStringBuf text, TVector<TStringBuf>* delimiters) {
    return Singleton<TRequestTokenizer>()->DoTokenize(text, delimiters);
}

TVector<TStringBuf> TRequestTokenizer::DoTokenize(TStringBuf textString, TVector<TStringBuf>* delimiters) const {
    TVector<TStringBuf> tokens;
    re2::StringPiece text(textString.data(), textString.length());
    re2::StringPiece token;
    size_t from = 0;
    while (!tokens.empty() && GluedTokenPattern.Match(text, from, text.length(), re2::RE2::ANCHOR_START, &token, 1)
        || TokenPattern.Match(text, from, text.length(), re2::RE2::UNANCHORED, &token, 1))
    {
        const size_t tokenBegin = token.data() - text.data();
        const size_t tokenEnd = tokenBegin + token.length();
        Y_ASSERT(from <= tokenBegin && tokenBegin <= tokenEnd && tokenEnd <= text.length());
        if (delimiters) {
            delimiters->emplace_back(text.data() + from, tokenBegin - from);
        }
        tokens.emplace_back(token.data(), token.length());
        from = tokenEnd;
    }
    if (delimiters) {
        delimiters->emplace_back(text.data() + from, text.length() - from);
    }
    return tokens;
}

} // namespace NNlu
