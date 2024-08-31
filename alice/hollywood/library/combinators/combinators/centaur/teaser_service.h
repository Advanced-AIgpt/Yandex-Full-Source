#pragma once

#include "defs.h"
#include "combinator_context_wrapper.h"

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/data/scenario/centaur/teasers/teaser_settings.pb.h>

namespace NAlice::NHollywood::NCombinators {

void UpdateMementoCentaurTeasersDeviceConfig(
    const NMemento::TCentaurTeasersDeviceConfig& centaurTeasersDeviceConfig,
    TCombinatorContextWrapper& CombinatorContextWrapper
);

void AddUserConfigs(
    NScenarios::TMementoChangeUserObjectsDirective& mementoDirective, 
    NMemento::EDeviceConfigKey key, 
    const NMemento::TCentaurTeasersDeviceConfig& value,
    TCombinatorContextWrapper& CombinatorContextWrapper
);

NMemento::TCentaurTeasersDeviceConfig GetCentaurTeasersDeviceConfig(TCombinatorContextWrapper& CombinatorContextWrapper);
bool CheckTeaserInSettings(NMemento::TCentaurTeasersDeviceConfig centaurTeasersDeviceConfig, NData::TCentaurTeaserConfigData teaserConfigData);
NMemento::TCentaurTeasersDeviceConfig PrepareDefaultCentaurTeaserDeviceConfig(TCombinatorContextWrapper& CombinatorContextWrapper);

}
