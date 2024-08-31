#pragma once

#include "processor.h"

namespace NAlice::NHollywoodFw::NVins {

class TProcessorShowTraffic : public IDivCardProcessor {
public:
    TProcessorShowTraffic()
        : IDivCardProcessor("personal_assistant.scenarios.show_traffic", "show.traffic.div.card")
    {
    }

    NData::TScenarioData Process(const NProtoVins::TVinsRunResponse& response) const override;
};

}  // namespace NAlice::NHollywoodFw::NVins
