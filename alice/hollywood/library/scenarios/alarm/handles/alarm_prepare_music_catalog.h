#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NReminders {

class TAlarmPrepareMusicCatalogHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "prepare_music_catalog";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NReminders
