#include "text_intersect_factors.h"

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/split.h>
#include <util/string/join.h>

namespace NNlg {

namespace {

static THashMap<TStringBuf, ui32> AccumulateFrequencies(const TVector<THashMap<TStringBuf, ui32>>& maps) {
    THashMap<TStringBuf, ui32> sum;
    for (const auto& map : maps) {
        for (const auto& pair : map) {
            sum[pair.first] += pair.second;
        }
    }
    return sum;
}

template<class TGetFrequenciesFunc>
void AppendIntersectionFactors(TGetFrequenciesFunc getFrequencies, const TVector<TString>& queryTurns, const TVector<TVector<TString>>& contexts, const TVector<TString>& replies, TVector<TVector<float>>* factors) {
    TVector<THashMap<TStringBuf, ui32>> queryFreqs;
    queryFreqs.reserve(queryTurns.size() + 1);
    for (const auto& turn : queryTurns) {
        queryFreqs.push_back(getFrequencies(turn));
    }
    queryFreqs.push_back(AccumulateFrequencies(queryFreqs));

    TVector<float> queryFactors;
    queryFactors.reserve(queryFreqs.size() * (queryFreqs.size() - 1) / 2);
    for (size_t i = 0; i < queryFreqs.size(); ++i) {
        for (size_t j = i + 1; j < queryFreqs.size(); ++j) {
            float f = TTextIntersectionFactors::CalcIntersectionFraction(queryFreqs[i], queryFreqs[j]);
            queryFactors.push_back(f);
        }
    }

    for (size_t doc = 0; doc < contexts.size(); ++doc) {
        TVector<THashMap<TStringBuf, ui32>> docFreqs;
        docFreqs.reserve(contexts[doc].size() + 2);
        for (const auto& turn : contexts[doc]) {
            docFreqs.push_back(getFrequencies(turn));
        }
        docFreqs.push_back(AccumulateFrequencies(docFreqs));
        docFreqs.push_back(getFrequencies(replies[doc]));

        auto& docFactors = (*factors)[doc];
        docFactors.insert(docFactors.end(), queryFactors.begin(), queryFactors.end());

        for (size_t i = 0; i < docFreqs.size(); ++i) {
            for (size_t j = i + 1; j < docFreqs.size(); ++j) {
                float f = TTextIntersectionFactors::CalcIntersectionFraction(docFreqs[i], docFreqs[j]);
                docFactors.push_back(f);
            }
        }

        for (const auto& query : queryFreqs) {
            for (const auto& doc : docFreqs) {
                float f = TTextIntersectionFactors::CalcIntersectionFraction(query, doc);
                docFactors.push_back(f);
            }
        }
    }
}

}

THashMap<TStringBuf, ui32> TTextIntersectionFactors::GetWordFrequencies(TStringBuf text) {
    THashMap<TStringBuf, ui32> freq;
    for (auto split : StringSplitter(text).Split(' ')) {
        ++freq[split.Token()];
    }
    return freq;
}

THashMap<TStringBuf, ui32> TTextIntersectionFactors::GetTrigramFrequencies(TStringBuf text) {
    const int N = 3;
    THashMap<TStringBuf, ui32> freq;
    for (size_t i = 0; i + N - 1 < text.size(); ++i) {
        ++freq[TStringBuf(text.data() + i, text.data() + i + N)];
    }
    return freq;
}

ui32 TTextIntersectionFactors::CalcIntersection(const THashMap<TStringBuf, ui32>& a, const THashMap<TStringBuf, ui32>& b) {
    if (a.size() > b.size()) {
        return CalcIntersection(b, a);
    }
    ui32 intersect = 0;
    for (const auto& pair : a) {
        if (const ui32* freq = b.FindPtr(pair.first)) {
            intersect += Min(pair.second, *freq);
        }
    }
    return intersect;
}

float TTextIntersectionFactors::CalcIntersectionFraction(const THashMap<TStringBuf, ui32>& a, const THashMap<TStringBuf, ui32>& b) {
    ui32 intersect = CalcIntersection(a, b);
    ui32 denom = Max(a.size(), b.size());
    return denom ? intersect / (denom + 0.0) : 0.0;
}

void TTextIntersectionFactors::AppendWordIntersectionFactors(const TVector<TString>& queryTurns, const TVector<TVector<TString>>& contexts, const TVector<TString>& replies, TVector<TVector<float>>* factors) const {
    AppendIntersectionFactors(GetWordFrequencies, queryTurns, contexts, replies, factors);
}

void TTextIntersectionFactors::AppendTrigramIntersectionFactors(const TVector<TString>& queryTurns, const TVector<TVector<TString>>& contexts, const TVector<TString>& replies, TVector<TVector<float>>* factors) const {
    AppendIntersectionFactors(GetTrigramFrequencies, queryTurns, contexts, replies, factors);
}

void TTextIntersectionFactors::AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const {
    AppendWordIntersectionFactors(ctx.QueryContext, ctx.Contexts, ctx.Replies, factors);
    AppendTrigramIntersectionFactors(ctx.QueryContext, ctx.Contexts, ctx.Replies, factors);
}

}

