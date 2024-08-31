#pragma once

#include "processor.h"

namespace NAlice::NHollywoodFw::NVins {

class TProcessorShowRoute : public IDivCardProcessor {
public:
    TProcessorShowRoute()
        : IDivCardProcessor(
            TSet<TString>{
                "personal_assistant.scenarios.show_route",
                "personal_assistant.scenarios.show_route__ellipsis"
            },"show.route.div.card")
    {
    }

    NData::TScenarioData Process(const NProtoVins::TVinsRunResponse& response) const override;
};

}  // namespace NAlice::NHollywoodFw::NVins
