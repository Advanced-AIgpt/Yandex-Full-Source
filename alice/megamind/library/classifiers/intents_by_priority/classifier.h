#pragma once

#include <alice/megamind/library/response/response.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/vector.h>

#include <functional>

namespace NAlice {

using TScenarioPriorityGetter = std::function<double(const TScenarioResponse&)>;
using TScenarioLanguageGetter = std::function<ELanguage(const TScenarioResponse&)>;


void RankByPriorityAndPredicts(
    TVector<TScenarioResponse>& responses,
    TScenarioPriorityGetter getPriority,
    TScenarioLanguageGetter getLanguage,
    const THashMap<TString, double>& postclassifierPredicts,
    const ELanguage requestLanguage
);

} // namespace NAlice
