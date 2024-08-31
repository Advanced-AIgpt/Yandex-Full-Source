#include "classifier.h"

#include <util/generic/algorithm.h>

#include <tuple>

namespace NAlice {

void RankByPriorityAndPredicts(
    TVector<TScenarioResponse>& responses,
    TScenarioPriorityGetter getPriority,
    TScenarioLanguageGetter getLanguage,
    const THashMap<TString, double>& postclassifierPredicts,
    const ELanguage requestLanguage
) {
    Sort(responses, [&](const TScenarioResponse& lhs, const TScenarioResponse& rhs) {
        const double lPriority = getPriority(lhs);
        const double rPriority = getPriority(rhs);
        if (lPriority != rPriority) {
            return lPriority > rPriority;
        }

        const auto* lPredict = postclassifierPredicts.FindPtr(lhs.GetScenarioName());
        const auto* rPredict = postclassifierPredicts.FindPtr(rhs.GetScenarioName());

        if (lPredict && rPredict) {
            // firstly prioritize scenario with same language response
            // then sort by postclassifier predictions
            // otherwise ensure order by scenario name

            auto makeComparatorKey = [&](const TScenarioResponse& resp) {
                return std::make_tuple(
                    getLanguage(resp) != requestLanguage,
                    -postclassifierPredicts.at(resp.GetScenarioName()),
                    resp.GetScenarioName()
                );
            };

            return makeComparatorKey(lhs) < makeComparatorKey(rhs);
        }
        if (lPredict) {
            return true;
        }
        return false;
    });
}

} // namespace NAlice
