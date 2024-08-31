#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NVideo {

class TVideoVhProxyPrepare : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "vh_proxy_prepare";
    }
    void Do(TScenarioHandleContext& ctx) const override;
};

class TVideoVhPlayerGetLastProcess : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "vh_player_get_last_process";
    }
    void Do(TScenarioHandleContext& ctx) const override;
};

class TVideoVhSeasonsProcess : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "vh_seasons_process";
    }
    void Do(TScenarioHandleContext& ctx) const override;
};

class TVideoVhEpisodesProcess : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "vh_episodes_process";
    }
    void Do(TScenarioHandleContext& ctx) const override;
};

class TVideoVhPlayerProcess : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "vh_player_process";
    }
    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NVideo
