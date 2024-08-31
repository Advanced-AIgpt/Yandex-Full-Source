#pragma once

#include <alice/nlu/granet/lib/sample/entity.h>
#include <util/generic/vector.h>

namespace NBg::NAliceEntityCollector {

class TWorkaroundEntitiesFinder {
public:
    TWorkaroundEntitiesFinder(const TVector<TString>& tokens, bool isPASkills, TVector<NGranet::TEntity>* entities);
    void Find();

private:
    void FindNumberByPattern();
    bool MatchPattern(const TVector<TString>& pattern, size_t from) const;
    void CreateNumberEntity(const NNlu::TInterval& pos, double value, double logProbCoeff);

private:
    const TVector<TString>& Tokens;
    const bool IsPASkills = false;
    TVector<NGranet::TEntity>* Entities;
};

} // namespace NBg::NAliceEntityCollector
