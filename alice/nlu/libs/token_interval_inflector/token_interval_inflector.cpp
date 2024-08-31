#include "token_interval_inflector.h"
#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/string/join.h>

namespace NAlice {
    namespace {
        size_t CountTokens(const TString& text) {
            if (text.empty()) {
                return 0;
            }
            return Count(text, ' ') + 1;
        }
    } // namespace anonymous

    TString TTokenIntervalInflector::Inflect(const TVector<TString>& tokens,
                                             const TVector<TString>& tokenGrammemes,
                                             const NNlu::TInterval& tokenInterval,
                                             const TString& targetCase) const {
        Y_ENSURE(tokenInterval.End <= tokens.size());
        Y_ENSURE(tokenGrammemes.size() == tokens.size());

        if (tokenInterval.Length() == 0) {
            return "";
        }

        TString inflected;

        if (TryInflectWithGrammemes(tokens, tokenGrammemes, tokenInterval, targetCase, &inflected)) {
            return inflected;
        }

        if (TryInflectDefault(tokens, tokenInterval, targetCase, &inflected)) {
            return inflected;
        }

        const TString notInflected = JoinRange(" ", tokens.begin() + tokenInterval.Begin, tokens.begin() + tokenInterval.End);
        return notInflected;
    }

    bool TTokenIntervalInflector::TryInflectWithGrammemes(const TVector<TString>& tokens,
                                                          const TVector<TString>& tokenGrammemes,
                                                          const NNlu::TInterval& tokenInterval,
                                                          const TString& targetCase,
                                                          TString* result) const {
        TVector<TString> tokensWithHints;
        tokensWithHints.reserve(tokenInterval.Length());

        for (size_t tokenPos = tokenInterval.Begin; tokenPos < tokenInterval.End; ++tokenPos) {
            const TString& grammemes = tokenGrammemes[tokenPos];
            const auto hint = grammemes.empty() ? TString("") : ("{grams=" + grammemes + "}");
            tokensWithHints.push_back(tokens[tokenPos] + hint);
        }

        return TryInflect(tokensWithHints, targetCase, result);
    }

    bool TTokenIntervalInflector::TryInflectDefault(const TVector<TString>& tokens,
                                                    const NNlu::TInterval& tokenInterval,
                                                    const TString& targetCase,
                                                    TString* result) const {
        const TArrayRef<const TString> intervalTokens(tokens.begin() + tokenInterval.Begin, tokens.begin() + tokenInterval.End);
        return TryInflect(intervalTokens, targetCase, result);
    }

    bool TTokenIntervalInflector::TryInflect(const TArrayRef<const TString>& tokens,
                                             const TString& targetCase,
                                             TString* result) const {
        const TString text = JoinSeq(" ", tokens);
        const TString inflected = WideToUTF8(Inflector.Inflect(UTF8ToWide(text), targetCase));
        if (CountTokens(inflected) != tokens.size()) {
            return false;
        }
        *result = inflected;
        return true;
    }
} // namespace NAlice
