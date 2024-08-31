#include "builder.h"
#include <dict/dictutil/dictutil.h>
#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/langs/langs.h>
#include <util/charset/wide.h>
#include <util/generic/adaptor.h>
#include <util/string/subst.h>

namespace {

const double PROBABILITY_OF_ENTITY_BY_REPETITION = 0.85;

TString NormalizeText(TStringBuf original, ELanguage lang) {
    TUtf16String result = ToLower(lang, UTF8ToWide(original));
    SubstGlobal(result, u'ё', u'е');
    return WideToUTF8(result);
}

} // namespace

namespace NAlice {

TNonsenseEntitiesBuilder::TNonsenseEntitiesBuilder(const TVector<TString>& tokens, const TVector<double>& taggerProbs)
    : Tokens(tokens)
    , TaggerProbs(taggerProbs)
{
    Y_ENSURE(Tokens.size() == TaggerProbs.size());
}

TVector<TNonsenseEntityHypothesis> TNonsenseEntitiesBuilder::Build() {
    BuildByTagger();
    BuildByRepetitions();
    return MakeResult();
}

void TNonsenseEntitiesBuilder::BuildByTagger() {
    for (const double threshold : CalculateThresholds()) {
        size_t i = 0;
        while (i < TaggerProbs.size()) {
            if (TaggerProbs[i] < threshold) {
                ++i;
                continue;
            }
            const size_t start = i;
            double prob = 1.;
            while (i < TaggerProbs.size() && TaggerProbs[i] >= threshold) {
                prob = Min(prob, TaggerProbs[i]);
                ++i;
            }
            AddEntity({start, i}, prob);
        }
    }
}

TVector<double> TNonsenseEntitiesBuilder::CalculateThresholds() const {
    TVector<double> sortedProbs = TaggerProbs;
    Sort(sortedProbs);
    TVector<double> thresholds;
    for (const auto& prob : Reversed(sortedProbs)) {
        // Truncate probabilities to grid 0.8, 0.7, ... 0.1
        const double grid = 10.;
        const double threshold = Min(grid - 2, floor(prob * grid)) / grid;
        if (threshold <= 0.) {
            continue;
        }
        if (!thresholds.empty() && thresholds.back() == threshold) {
            continue;
        }
        thresholds.push_back(threshold);
    }
    return thresholds;
}

void TNonsenseEntitiesBuilder::BuildByRepetitions() {
    if (Tokens.size() <= 1 || Tokens.size() > 100) {
        return;
    }
    const TVector<ui32> tokens = BuildTokenIds();
    TVector<NNlu::TInterval> repetitions = FindRepetitions(tokens);
    MergeRepetitions(&repetitions);
    WriteRepetitions(repetitions);
}

TVector<ui32> TNonsenseEntitiesBuilder::BuildTokenIds() const {
    THashMap<TString, ui32> tokenToId;
    TVector<ui32> ids;
    for (const TString& token : Tokens) {
        const TString normalized = NormalizeText(token, LANG_RUS);
        const auto [it, isNew] = tokenToId.try_emplace(normalized, tokenToId.size());
        ids.push_back(it->second);
    }
    return ids;
}

// static
TVector<NNlu::TInterval> TNonsenseEntitiesBuilder::FindRepetitions(const TVector<ui32>& tokens) {
    TVector<NNlu::TInterval> repetitions;
    for (size_t start1 = 0; start1 < tokens.size(); ++start1) {
        const size_t maxLength = (tokens.size() - start1) / 2;
        for (size_t length = 1; length <= maxLength; ++length) {
            const size_t start2 = start1 + length;
            Y_ENSURE(start2 + length <= tokens.size());
            if (tokens[start1] != tokens[start2]) {
                continue;
            }
            bool hasMismatch = false;
            for (size_t i = 0; i < length; ++i) {
                if (tokens[start1 + i] != tokens[start2 + i]) {
                    hasMismatch = true;
                    break;
                }
            }
            if (hasMismatch) {
                continue;
            }
            repetitions.push_back({start1, start2});
        }
    }
    return repetitions;
}

// static
void TNonsenseEntitiesBuilder::MergeRepetitions(TVector<NNlu::TInterval>* repetitions) {
    if (repetitions->size() <= 1) {
        return;
    }
    TVector<NNlu::TInterval> result;
    NNlu::TInterval accumulator;
    Sort(*repetitions);
    for (const NNlu::TInterval& current : *repetitions) {
        if (accumulator.Empty()) {
            accumulator = current;
            continue;
        }
        if (current.Begin > accumulator.End) {
            result.push_back(accumulator);
            accumulator = current;
            continue;
        }
        accumulator.End = current.End;
    }
    result.push_back(accumulator);
    *repetitions = std::move(result);
}

void TNonsenseEntitiesBuilder::WriteRepetitions(const TVector<NNlu::TInterval>& repetitions) {
    for (const NNlu::TInterval& interval : repetitions) {
        AddEntity(interval, PROBABILITY_OF_ENTITY_BY_REPETITION);
    }
}

void TNonsenseEntitiesBuilder::AddEntity(const NNlu::TInterval& interval, double prob) {
    double& value = Entities[interval];
    value = Max(value, prob);
}

TVector<TNonsenseEntityHypothesis> TNonsenseEntitiesBuilder::MakeResult() const {
    TVector<TNonsenseEntityHypothesis> result;
    for (const auto& [interval, prob] : Entities) {
        result.push_back(TNonsenseEntityHypothesis{.Interval = interval, .Prob = prob});
    }
    SortBy(result, [](const TNonsenseEntityHypothesis& item) {return std::pair(-item.Prob, item.Interval);});
    return result;
}

} // namespace NAlice
