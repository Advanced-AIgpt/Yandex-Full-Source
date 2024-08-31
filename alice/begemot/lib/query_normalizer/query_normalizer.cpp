#include "query_normalizer.h"

#include <util/string/join.h>

namespace NBg {

namespace {

const TVector<NBg::TPrefixNormalizationRule> SEARCH_NORMALIZATION_RULES = {
    {std::make_shared<re2::RE2>("((.* |^)про|((найди|поищи)(| мне)))"), "acc"},
    {std::make_shared<re2::RE2>("(.* |^)о(б|)"), "loc"},
};

}

TString NormalizeQueryByPrefix(const TVector<TPrefixNormalizationRule>& rules,
                               const TVector<TString>& tokens,
                               const NNlu::TInterval& queryInterval,
                               const NAlice::TTokenIntervalInflector& inflector)
{
    if (queryInterval.Begin >= tokens.size()) {
        return {};
    }
    if (queryInterval.Begin == 0) {
        return JoinRange(" ", tokens.begin(), tokens.begin() + queryInterval.End);
    }
    const TString prefix = JoinRange(" ", tokens.begin(), tokens.begin() + queryInterval.Begin);
    for (const auto& rule : rules) {
        if (RE2::PartialMatch(re2::StringPiece(prefix), *rule.ExpectedPrefix)) {
            TVector<TString> grammems(tokens.size());
            for (size_t i = queryInterval.Begin; i < queryInterval.End; ++i) {
                grammems[i] = rule.SourceCase;
            }
            return inflector.Inflect(tokens, grammems, queryInterval, "nom");
        }
    }
    return JoinRange(" ", tokens.begin() + queryInterval.Begin, tokens.begin() + queryInterval.End);
}

TString NormalizeSearchQueryByPrefix(const TVector<TString>& tokens,
                                     const NNlu::TInterval& queryInterval,
                                     const NAlice::TTokenIntervalInflector& inflector)
{
    return NormalizeQueryByPrefix(SEARCH_NORMALIZATION_RULES, tokens, queryInterval, inflector);
}

} // namespace NBg