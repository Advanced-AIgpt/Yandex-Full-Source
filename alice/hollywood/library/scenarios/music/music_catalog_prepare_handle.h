#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusic {

class TMusicCatalogPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "music_catalog_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic
