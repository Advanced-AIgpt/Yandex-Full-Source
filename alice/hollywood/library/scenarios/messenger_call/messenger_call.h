#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/hollywood/library/scenarios/messenger_call/nlg/register.h>

#include <alice/protos/data/contacts.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

class TMessengerCallRunHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood
