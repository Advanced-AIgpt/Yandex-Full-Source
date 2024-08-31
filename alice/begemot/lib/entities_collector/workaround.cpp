#include "workaround.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/generic/xrange.h>

using namespace NNlu;
using namespace NGranet;

namespace NBg::NAliceEntityCollector {

struct TNumberPattern {
    TVector<TString> Tokens;
    double Value = 0;
    double LogProbCoeff = 1;
};

static const TVector<TNumberPattern> NUMBER_PATTERNS = {
    {{"полторы"}, 1.5, 1},
    {{"полтора"}, 1.5, 1},
    {{"2", "и", "5"}, 2.5, 3},
    {{"2", "500"}, 2500, 2}
};

TWorkaroundEntitiesFinder::TWorkaroundEntitiesFinder(const TVector<TString>& tokens, bool isPASkills,
        TVector<NGranet::TEntity>* entities)
    : Tokens(tokens)
    , IsPASkills(isPASkills)
    , Entities(entities)
{
    Y_ENSURE(Entities);
}

void TWorkaroundEntitiesFinder::Find() {
    FindNumberByPattern();
}

void TWorkaroundEntitiesFinder::FindNumberByPattern() {
    for (size_t i = 0; i < Tokens.size(); ++i) {
        for (const auto& pattern : NUMBER_PATTERNS) {
            if (MatchPattern(pattern.Tokens, i)) {
                CreateNumberEntity({i, i + pattern.Tokens.size()}, pattern.Value, pattern.LogProbCoeff);
                break;
            }
        }
    }
}

bool TWorkaroundEntitiesFinder::MatchPattern(const TVector<TString>& pattern, size_t from) const {
    if (from + pattern.size() > Tokens.size()) {
        return false;
    }
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] != Tokens[from + i]) {
            return false;
        }
    }
    return true;
}

void TWorkaroundEntitiesFinder::CreateNumberEntity(const TInterval& pos, double value, double logProbCoeff) {
    TEntity entity;
    entity.Interval = pos;
    entity.Value = ToString(value);
    entity.Quality = 0;
    entity.LogProbability = NEntityLogProbs::SYS * logProbCoeff;
    entity.Type = round(value) == value ? NEntityTypes::SYS_NUM : NEntityTypes::SYS_FLOAT;
    Entities->push_back(entity);
    if (IsPASkills) {
        entity.Type = NEntityTypes::PA_SKILLS_NUMBER;
        Entities->push_back(entity);
    }
}

} // namespace NBg::NAliceEntityCollector
