#include "pronoun_factors.h"

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/split.h>

namespace NNlg {

namespace NPrivate {

static THashMap<TString, size_t> Enumerate(const TVector<TString>& v) {
    THashMap<TString, size_t> ids;
    for (size_t i = 0; i < v.size(); ++i) {
        ids[v[i]] = i;
    }
    return ids;
}

}

const TVector<TString> TPronounFactors::Pronouns = {
    "я", "что", "ты", "нет", "да", "кто", "вы", "он", "она", "сама", "сам", "они", "сами", "само"
};

const THashMap<TString, size_t> TPronounFactors::PronounIds = NPrivate::Enumerate(Pronouns);

void TPronounFactors::AppendPronounFactors(TStringBuf text, TVector<float>* factors) const {
    size_t offset = factors->size();
    factors->resize(factors->size() + Pronouns.size());
    for (auto split : StringSplitter(text).Split(' ')) {
        if (const size_t* id = PronounIds.FindPtr(split.Token())) {
            (*factors)[offset + *id] += 1.0;
        }
    }
}

void TPronounFactors::AppendQueryFactors(const TVector<TString>& queryTurns, TVector<TVector<float>>* factors) const {
    for (size_t doc = 0; doc < factors->size(); ++doc) {
        if (doc == 0) {
            for (const auto& turn : queryTurns) {
                AppendPronounFactors(turn, &(*factors)[doc]);
            }
        } else {
            size_t numTextFeatures = (*factors)[0].size() - (*factors)[doc].size();
            (*factors)[doc].insert((*factors)[doc].end(), (*factors)[0].end() - numTextFeatures, (*factors)[0].end());
        }
    }
}

void TPronounFactors::AppendContextFactors(const TVector<TVector<TString>>& contexts, TVector<TVector<float>>* factors) const {
    for (size_t doc = 0; doc < factors->size(); ++doc) {
        for (const auto& turn : contexts[doc]) {
            AppendPronounFactors(turn, &(*factors)[doc]);
        }
    }
}

void TPronounFactors::AppendReplyFactors(const TVector<TString>& replies, TVector<TVector<float>>* factors) const {
    for (size_t doc = 0; doc < factors->size(); ++doc) {
        AppendPronounFactors(replies[doc], &(*factors)[doc]);
    }
}

void TPronounFactors::AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const {
    AppendQueryFactors(ctx.QueryContext, factors);
    AppendContextFactors(ctx.Contexts, factors);
    AppendReplyFactors(ctx.Replies, factors);
}

}

