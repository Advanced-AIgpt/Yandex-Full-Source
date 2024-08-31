#include "modifier_registry.h"

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/yexception.h>

namespace NAlice::NHollywood::NModifiers {

TModifierRegistry& TModifierRegistry::Get() {
    return *Singleton<TModifierRegistry>();
};

TVector<TBaseModifierPtr> TModifierRegistry::CreateModifiers(const TModifiersConfig& modifiersConfig, const TFsPath& resourcesBasePath) {
    Y_ENSURE(modifiersConfig.ModifiersSize() == Producers_.size());

    auto modifiersMap = THashMap<TString, TBaseModifierPtr>(Producers_.size());
    for (const auto& producer : Producers_) {
        auto modifier = producer();
        Y_ENSURE(modifier);
        const auto modifierName = modifier->GetModifierType();
        const auto [it, inserted] = modifiersMap.emplace(modifierName, std::move(modifier));
        Y_ENSURE(inserted, "Duplicated modifier " << modifierName);
    }

    auto result = TVector<TBaseModifierPtr>(Reserve(modifiersConfig.ModifiersSize()));
    for (const auto& modifierConfig : modifiersConfig.GetModifiers()) {
        const auto& modifierName = modifierConfig.GetName();

        auto& modifier = modifiersMap[modifierName];
        Y_ENSURE(modifier, "Missing or duplicated modifier " << modifierName);

        modifier->SetEnabled(modifierConfig.GetEnabled());
        modifier->LoadResourcesFromPath(resourcesBasePath / TStringBuf("modifiers") / modifierName);

        result.emplace_back(nullptr).swap(modifier);
    }
    return result;
}

void TModifierRegistry::AddModifierProducer(const TModifierProducer& producer) {
    Producers_.push_back(producer);
}

TModifierRegistrator::TModifierRegistrator(const TModifierProducer& producer) {
    TModifierRegistry::Get().AddModifierProducer(producer);
}

} // NAlice::NHollywood::NModifiers
