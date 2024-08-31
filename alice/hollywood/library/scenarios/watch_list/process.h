#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NWatchList {

    class TTvWatchListProcessHandle: public TScenario::THandleBase {
    public:
        TString Name() const override {
            return "grpc/tv_watch_list/process";
        }

        void Do(TScenarioHandleContext& ctx) const override;
    };
} // namespace NAlice::NHollywood::NWatchList
