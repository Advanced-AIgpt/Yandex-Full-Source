#include "factors.h"

#include <alice/library/parsed_user_phrase/parsed_sequence.h>
#include <alice/library/parsed_user_phrase/utils.h>

#include <util/generic/array_size.h>
#include <util/string/builder.h>
#include <util/system/yassert.h>

#include <cctype>

namespace NVideoCommon {
namespace NShowOrGallery {
namespace {
const TStringBuf NUMBERS[] = {"ноль",       "один",        "два",        "три",          "четыре",
                              "пять",       "шесть",       "семь",       "восемь",       "девять",
                              "десять",     "одиннадцать", "двенадцать", "тринадцать",   "четырнадцать",
                              "пятнадцать", "шестнадцать", "семнадцать", "восемнадцать", "девятнадцать"};

struct TNamesCollector {
    TNamesCollector(TVector<TString>& names, size_t index)
        : Names(names)
        , Index(index) {
    }

    void operator()(double /* value */, TStringBuf name) {
        Names.push_back(TStringBuilder() << name << "_" << Index);
    }

    TVector<TString>& Names;
    const size_t Index;
};

struct TValuesCollector {
    explicit TValuesCollector(TVector<float>& values)
        : Values(values) {
    }

    void operator()(double value, TStringBuf /* name */) {
        Values.push_back(static_cast<float>(value));
    }

    TVector<float>& Values;
};

TStringBuf FixSmallNumber(TStringBuf word) {
    if (word.empty() || word.size() > 2)
        return word;

    size_t number = 0;
    for (size_t i = 0; i < word.size(); ++i) {
        if (!isdigit(word[i]))
            return word;
        number = number * 10 + word[i] - '0';
    }

    if (number >= Y_ARRAY_SIZE(NUMBERS))
        return word;

    return NUMBERS[number];
}

TString NormalizeName(TStringBuf name) {
    TStringBuilder builder;

    NParsedUserPhrase::ForEachToken(TString{name},
                                    [&builder](TStringBuf word) {
                                        if (!builder.empty())
                                            builder << ' ';
                                        Y_ASSERT(!word.empty());
                                        builder << FixSmallNumber(word);
                                    });

    return builder;
}
} // namespace

// TFactors::TEntry ------------------------------------------------------------
void TFactors::TEntry::Clear() {
    RelevancePrediction = 0;
    Relevance = 0;
    Rating = 0;
    Similarity = 0;
}

// TFactors --------------------------------------------------------------------
// static
float TFactors::CalcSimilarity(TStringBuf query, TStringBuf name) {
    const NParsedUserPhrase::TParsedSequence q(NormalizeName(query));
    const NParsedUserPhrase::TParsedSequence n(NormalizeName(name));
    return q.Match(n);
}

void TFactors::GetNames(TVector<TString>& names) const {
    for (size_t i = 0; i < TOP_RESULTS; ++i) {
        TNamesCollector collector(names, i);
        Entries[i].Visit(collector);
    }

    for (size_t i = 0; i < Y_ARRAY_SIZE(RelevanceRatios); ++i)
        names.emplace_back(TStringBuilder() << "relevance_ratio_" << i);
}

void TFactors::GetValues(TVector<float>& values) const {
    TValuesCollector collector(values);
    for (const auto& entry : Entries)
        entry.Visit(collector);

    for (size_t i = 0; i < Y_ARRAY_SIZE(RelevanceRatios); ++i)
        values.emplace_back(RelevanceRatios[i]);
}
} // namespace NShowOrGallery
} // namespace NVideoCommon
