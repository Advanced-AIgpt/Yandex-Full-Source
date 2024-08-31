#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood {

class TOpenAppsFixlistHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TSearchPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TSearchRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TSearchApplyHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "apply";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TSearchCommitPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TSearchCommitRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit_render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TEntitySearchGoodwinCallbackHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "entity_search_goodwin_callback";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood
