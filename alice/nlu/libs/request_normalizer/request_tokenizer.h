#pragma once

#include <re2/re2.h>
#include <util/generic/vector.h>

namespace NNlu {

class TRequestTokenizer : public TNonCopyable {
public:
    // Returns tokens of string.
    // Tokens with 'delimiters' is partition of 'text'. Without overlapping and gaps.
    // Size of delimiters is always equal tokens size plus 1.
    static TVector<TStringBuf> Tokenize(TStringBuf text, TVector<TStringBuf>* delimiters = nullptr);

private:
    TRequestTokenizer();

    TVector<TStringBuf> DoTokenize(TStringBuf text, TVector<TStringBuf>* delimiters) const;

    Y_DECLARE_SINGLETON_FRIEND()

private:
    const RE2 TokenPattern;
    const RE2 GluedTokenPattern;
};

} // namespace NNlu
