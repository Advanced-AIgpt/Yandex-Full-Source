#include "tokenizer.h"

#include <alice/nlu/libs/normalization/normalize.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/string/split.h>

namespace NNlu {

TVector<TString> TTokenizerBase::GetNormalizedTokens() const {
    const TVector<TString> tokens = GetOriginalTokens();
    TVector<TString> result(Reserve(tokens.size()));
    for (const TString& token : tokens) {
        result.push_back(NormalizeWord(token, Lang));
    }
    return result;
}

TSmartTokenizer::TSmartTokenizer(TStringBuf line, ELanguage lang)
    : TTokenizerBase(lang)
    , Line(line)
{
    WideLine = UTF8ToWide(Line);
    TNlpTokenizer(*this, false).Tokenize(WideLine, false, TLangMask(Lang, LANG_ENG));
    Y_ASSERT(CurrentTokenEnd == WideLine.length());
}

void TSmartTokenizer::OnToken(const TWideToken& token, size_t originalLength, NLP_TYPE) {
    CurrentTokenBegin = CurrentTokenEnd;
    CurrentTokenEnd += originalLength;
    Y_ASSERT(CurrentTokenEnd <= WideLine.length());

    for (const TCharSpan& subToken : token.SubTokens) {
        Tokens.push_back(TUtf16String(token.Token + subToken.Pos, subToken.Len));

        TInterval interval;
        interval.Begin = CurrentTokenBegin + subToken.Pos;
        interval.End = interval.Begin + subToken.Len;

        // Crop approximated subtoken interval by actual interval of token.
        const size_t prevEnd = IntervalsInWideLine.empty() ? 0 : IntervalsInWideLine.back().End;
        interval.Begin = Max(Min(interval.Begin, CurrentTokenEnd), prevEnd);
        interval.End = Max(Min(interval.End, CurrentTokenEnd), prevEnd);

        IntervalsInWideLine.push_back(interval);
    }
}

TVector<TString> TSmartTokenizer::GetOriginalTokens() const {
    TVector<TString> result(Reserve(Tokens.size()));
    for (const TUtf16String& token : Tokens) {
        result.push_back(WideToUTF8(token));
    }
    return result;
}

TVector<TInterval> TSmartTokenizer::GetTokensIntervals() const {
    TVector<TInterval> intervals(Reserve(Tokens.size()));
    size_t srcOffset = 0;
    size_t destOffset = 0;

    for (const TInterval& srcToken : IntervalsInWideLine) {
        Y_ASSERT(srcOffset <= srcToken.Begin);
        Y_ASSERT(srcToken.End <= WideLine.length());

        destOffset += WideToUTF8(WideLine.data() + srcOffset, srcToken.Begin - srcOffset).length();
        const size_t destBegin = destOffset;
        destOffset += WideToUTF8(WideLine.data() + srcToken.Begin, srcToken.Length()).length();
        intervals.push_back({destBegin, destOffset});
        srcOffset = srcToken.End;
    }

    Y_ASSERT(srcOffset <= WideLine.length());
    destOffset += WideToUTF8(WideLine.data() + srcOffset, WideLine.length() - srcOffset).length();
    Y_ENSURE(destOffset == Line.length());

    return intervals;
}

TWhiteSpaceTokenizer::TWhiteSpaceTokenizer(TStringBuf line, ELanguage lang)
    : TTokenizerBase(lang)
{
    TUtf16String wideLine = UTF8ToWide(line);

    for (auto&& token : StringSplitter(wideLine).SplitByFunc(IsWhitespace).SkipEmpty()) {
        Tokens.push_back(WideToUTF8(token));
    }
}

TVector<TString> TWhiteSpaceTokenizer::GetOriginalTokens() const {
    return Tokens;
}

} // namespace NNlu
