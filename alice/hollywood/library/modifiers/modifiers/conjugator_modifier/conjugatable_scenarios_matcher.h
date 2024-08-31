#pragma once

#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/modifiers/modifiers/conjugator_modifier/proto/conjugatable_scenarios_config.pb.h>
#include <alice/protos/data/language/language.pb.h>
#include <util/generic/hash_set.h>


namespace NAlice::NHollywood::NModifiers {

class TConjugatableScenariosMatcher {
public:
    explicit TConjugatableScenariosMatcher(const TConjugatableScenariosConfig& conjugatableScenariosConfig);
    bool IsConjugatableLanguageScenario(const ELang language, const TString& productScenarioName, const TExpFlags& expFlags) const;
private:
    using TLanguageProductScenarioName = std::tuple<ELang, TString>;
private:
    THashSet<TLanguageProductScenarioName> EnabledLanguageScenarioTuples_;
};

} // namespace NAlice::NHollywood::NModifiers
