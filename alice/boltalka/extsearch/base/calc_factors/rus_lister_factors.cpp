#include "rus_lister_factors.h"

#include <util/digest/city.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/string/split.h>

namespace NNlg {

const ui8 TRusListerFactors::TagBitCount = 7;
const ui64 TRusListerFactors::TagMarkerBit = 1ULL << (TagBitCount - 1);
const ui64 TRusListerFactors::TagMask = TagMarkerBit - 1;

void TRusListerFactors::AddFileLine(TStringBuf line) {
    TString key, value;
    Split(line, '\t', key, value);
    ui64 mask = 0;
    ui32 numTags = 0;
    for (auto split : StringSplitter(value).Split(' ')) {
        ui64 id = FromString<ui64>(split.Token());
        NumTags = Max<size_t>(NumTags, id + 1);
        Y_VERIFY(~id & TagMarkerBit);
        mask |= (id | TagMarkerBit) << (numTags * TagBitCount);
        ++numTags;
    }
    ui64 hkey = CityHash64(key.data(), key.size());
    WordTags[hkey] = mask;
}

TRusListerFactors::TRusListerFactors(const TString& filename)
    : NumTags(0)
{
    TFileInput input(filename);
    for (TString line; input.ReadLine(line); ) {
        AddFileLine(line);
    }
}

TRusListerFactors::TRusListerFactors(const TVector<TString>& fileLines)
    : NumTags(0)
{
    for (const auto& line : fileLines) {
        AddFileLine(line);
    }
}

void TRusListerFactors::AppendRusListerFactors(TStringBuf text, TVector<float>* factors) const {
    size_t offset = factors->size();
    factors->resize(factors->size() + NumTags);
    ui32 numTokens = 0;
    ui32 outOfVocab = 0;
    for (auto split : StringSplitter(text).Split(' ')) {
        ++numTokens;
        ui64 hkey = CityHash64(split.Token().data(), split.Token().size());
        if (const ui64* maskPtr = WordTags.FindPtr(hkey)) {
            for (ui64 mask = *maskPtr; mask; mask >>= TagBitCount) {
                ui64 tag = mask & TagMask;
                (*factors)[offset + tag] += 1.0;
            }
        } else {
            ++outOfVocab;
        }
    }
    factors->push_back(numTokens ? outOfVocab / (numTokens + 0.0) : 0.0);
}

void TRusListerFactors::AppendQueryFactors(const TVector<TString>& queryTurns, TVector<TVector<float>>* factors) const {
    for (size_t doc = 0; doc < factors->size(); ++doc) {
        if (doc == 0) {
            for (const auto& turn : queryTurns) {
                AppendRusListerFactors(turn, &(*factors)[doc]);
            }
        } else {
            size_t numTextFeatures = (*factors)[0].size() - (*factors)[doc].size();
            (*factors)[doc].insert((*factors)[doc].end(), (*factors)[0].end() - numTextFeatures, (*factors)[0].end());
        }
    }
}

void TRusListerFactors::AppendContextFactors(const TVector<TVector<TString>>& contexts, TVector<TVector<float>>* factors) const {
    for (size_t doc = 0; doc < factors->size(); ++doc) {
        for (const auto& turn : contexts[doc]) {
            AppendRusListerFactors(turn, &(*factors)[doc]);
        }
    }
}

void TRusListerFactors::AppendReplyFactors(const TVector<TString>& replies, TVector<TVector<float>>* factors) const {
    for (size_t doc = 0; doc < factors->size(); ++doc) {
        AppendRusListerFactors(replies[doc], &(*factors)[doc]);
    }
}

void TRusListerFactors::AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const {
    AppendQueryFactors(ctx.QueryContext, factors);
    AppendContextFactors(ctx.Contexts, factors);
    AppendReplyFactors(ctx.Replies, factors);
}

}

