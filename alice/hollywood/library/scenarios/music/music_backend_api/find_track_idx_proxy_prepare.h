#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusic {

class TFindTrackIdxProxyPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "thin_find_track_idx_proxy_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic
