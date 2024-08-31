#include "basic_text_factors.h"

#include <util/generic/strbuf.h>

namespace NNlg {

void TBasicTextFactors::AppendPunctFactors(TStringBuf text, TVector<float>* factors, const TString& punctuation) const {
    factors->resize(factors->size() + punctuation.size());
    for (size_t i = 0; i < text.size(); ++i) {
        size_t pos = punctuation.find(text[i]);
        if (pos == TString::npos) {
            continue;
        }
        (*factors)[factors->size() - punctuation.size() + pos] += 1;
    }
}

void TBasicTextFactors::AppendWordLengthFactors(TStringBuf text, TVector<float>* factors) const {
    size_t numSpaces = std::count(text.begin(), text.end(), ' ');
    size_t numWords = numSpaces + !text.empty();
    size_t sumWordLens = text.size() - numSpaces;
    factors->push_back(numWords);
    factors->push_back(sumWordLens);
    factors->push_back(numWords ? static_cast<double>(sumWordLens) / numWords : 0.0);

    int lastSpace = -1;
    int minWordLen = 0;
    int maxWordLen = 0;
    for (size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == ' ') {
            int wordLen = i - lastSpace - 1;
            if (minWordLen == 0 || minWordLen > wordLen) {
                minWordLen = wordLen;
            }
            if (maxWordLen == 0 || maxWordLen < wordLen) {
                maxWordLen = wordLen;
            }
            lastSpace = i;
        }
    }
    factors->push_back(minWordLen);
    factors->push_back(maxWordLen);
}

void TBasicTextFactors::AppendQueryFactors(const TVector<TString>& queryTurns, TVector<TVector<float>>* factors) const {
    for (size_t doc = 0; doc < factors->size(); ++doc) {
        if (doc == 0) {
            for (const auto& turn : queryTurns) {
                AppendPunctFactors(turn, &(*factors)[doc]);
                AppendWordLengthFactors(turn, &(*factors)[doc]);
            }
        } else {
            size_t numTextFeatures = (*factors)[0].size() - (*factors)[doc].size();
            (*factors)[doc].insert((*factors)[doc].end(), (*factors)[0].end() - numTextFeatures, (*factors)[0].end());
        }
    }
}

void TBasicTextFactors::AppendContextFactors(const TVector<TVector<TString>>& contexts, TVector<TVector<float>>* factors) const {
    for (size_t doc = 0; doc < factors->size(); ++doc) {
        for (const auto& turn : contexts[doc]) {
            AppendPunctFactors(turn, &(*factors)[doc]);
            AppendWordLengthFactors(turn, &(*factors)[doc]);
        }
    }
}

void TBasicTextFactors::AppendReplyFactors(const TVector<TString>& replies, TVector<TVector<float>>* factors) const {
    for (size_t doc = 0; doc < factors->size(); ++doc) {
        AppendPunctFactors(replies[doc], &(*factors)[doc]);
        AppendWordLengthFactors(replies[doc], &(*factors)[doc]);
    }
}

void TBasicTextFactors::AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const {
    AppendQueryFactors(ctx.QueryContext, factors);
    AppendContextFactors(ctx.Contexts, factors);
    AppendReplyFactors(ctx.Replies, factors);
}

}

