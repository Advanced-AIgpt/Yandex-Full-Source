#pragma once

#include <alice/nlu/libs/interval/interval.h>
#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NAlice {

struct TNonsenseEntityHypothesis {
    NNlu::TInterval Interval;
    double Prob = 0;

    DECLARE_TUPLE_LIKE_TYPE(TNonsenseEntityHypothesis, Interval, Prob);
};

class TNonsenseEntitiesBuilder : public TMoveOnly {
public:
    TNonsenseEntitiesBuilder(const TVector<TString>& tokens, const TVector<double>& taggerProbs);

    TVector<TNonsenseEntityHypothesis> Build();

private:
    void BuildByTagger();
    TVector<double> CalculateThresholds() const;
    void BuildByRepetitions();
    TVector<ui32> BuildTokenIds() const;
    static TVector<NNlu::TInterval> FindRepetitions(const TVector<ui32>& tokens);
    static void MergeRepetitions(TVector<NNlu::TInterval>* repetitions);
    void WriteRepetitions(const TVector<NNlu::TInterval>& repetitions);
    void AddEntity(const NNlu::TInterval& interval, double prob);
    TVector<TNonsenseEntityHypothesis> MakeResult() const;

private:
    const TVector<TString> Tokens;
    const TVector<double> TaggerProbs;
    THashMap<NNlu::TInterval, double> Entities;
};

} // namespace NAlice
