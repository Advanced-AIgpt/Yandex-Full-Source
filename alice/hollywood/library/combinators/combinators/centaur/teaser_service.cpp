#include "teaser_service.h"
#include "alice/protos/data/scenario/centaur/teasers/teaser_settings.pb.h"
#include "combinator_context_wrapper.h"

#include <alice/memento/proto/api.pb.h>
#include <alice/memento/proto/device_configs.pb.h>

namespace NAlice::NHollywood::NCombinators {

using namespace NAlice::NScenarios;

void UpdateMementoCentaurTeasersDeviceConfig(
    const NMemento::TCentaurTeasersDeviceConfig& centaurTeasersDeviceConfig,
    TCombinatorContextWrapper& CombinatorContextWrapper
) {
    LOG_INFO(CombinatorContextWrapper.Logger()) << "Got config to update " << centaurTeasersDeviceConfig.AsJSON();
    auto& mementoDirective = *CombinatorContextWrapper.ResponseForRenderer().MutableResponseBody()->AddServerDirectives()
                                        ->MutableMementoChangeUserObjectsDirective();
    AddUserConfigs(mementoDirective, NMemento::EDeviceConfigKey::DCK_CENTAUR_TEASERS, centaurTeasersDeviceConfig, CombinatorContextWrapper);
}

void AddUserConfigs(
    TMementoChangeUserObjectsDirective& mementoDirective, 
    NMemento::EDeviceConfigKey key, 
    const NMemento::TCentaurTeasersDeviceConfig& value, 
    TCombinatorContextWrapper& CombinatorContextWrapper
) {
    NMemento::TDeviceConfigs deviceConfigs;
    deviceConfigs.SetDeviceId(CombinatorContextWrapper.Request().BaseRequestProto().GetClientInfo().GetDeviceId());
    auto& deviceConfigsKeyValuePair = *deviceConfigs.AddDeviceConfigs();
    deviceConfigsKeyValuePair.SetKey(key);
;
    if (deviceConfigsKeyValuePair.MutableValue()->PackFrom(value)) {
        *mementoDirective.MutableUserObjects()->AddDevicesConfigs() = std::move(deviceConfigs);
    } else {
        LOG_ERROR(CombinatorContextWrapper.Logger()) << "PackFrom failed for user config";
    }
}

NMemento::TCentaurTeasersDeviceConfig GetCentaurTeasersDeviceConfig(TCombinatorContextWrapper& CombinatorContextWrapper) {

    const auto fullMementoData = CombinatorContextWrapper.Ctx().GetProtoOrThrow<NMemento::TRespGetAllObjects>(AH_ITEM_FULL_MEMENTO_DATA);
    const auto& deviceId = CombinatorContextWrapper.Request().BaseRequestProto().GetClientInfo().GetDeviceId();
    const auto& surfaceConfigs = fullMementoData.GetSurfaceConfigs();
    const auto& deviceSurfaceConfigs = surfaceConfigs.find(deviceId);
    bool validConfigExists = false;

    NMemento::TCentaurTeasersDeviceConfig centaurTeasersDeviceConfig;
    if (deviceSurfaceConfigs != surfaceConfigs.end() && deviceSurfaceConfigs->second.HasCentaurTeasersDeviceConfig()) {
        centaurTeasersDeviceConfig = deviceSurfaceConfigs->second.GetCentaurTeasersDeviceConfig();
        if (centaurTeasersDeviceConfig.GetTeaserConfigs().size() != 0) {
            validConfigExists = true;
        }
    }
    if(validConfigExists) {
         LOG_DEBUG(CombinatorContextWrapper.Logger()) << "Has valid centaur surface config for teasers";
    } 
    return validConfigExists ? centaurTeasersDeviceConfig : PrepareDefaultCentaurTeaserDeviceConfig(CombinatorContextWrapper);
    
}


bool CheckTeaserInSettings(
    NMemento::TCentaurTeasersDeviceConfig centaurTeasersDeviceConfig, 
    NData::TCentaurTeaserConfigData teaserConfigData
) {
    for(auto config : centaurTeasersDeviceConfig.GetTeaserConfigs()) {
        if(config.GetTeaserType() == teaserConfigData.GetTeaserType() && config.GetTeaserId() == teaserConfigData.GetTeaserId()) {
            return true;
        }
    }
    return false;
}

NMemento::TCentaurTeasersDeviceConfig PrepareDefaultCentaurTeaserDeviceConfig(TCombinatorContextWrapper& CombinatorContextWrapper){ 
    NMemento::TCentaurTeasersDeviceConfig centaurTeasersDeviceConfigNew;
    NAlice::NData::TCentaurTeaserConfigData photoframeConfigData;
    photoframeConfigData.SetTeaserType("PhotoFrame");
    *centaurTeasersDeviceConfigNew.AddTeaserConfigs() = std::move(photoframeConfigData);
    UpdateMementoCentaurTeasersDeviceConfig(centaurTeasersDeviceConfigNew, CombinatorContextWrapper);
    return centaurTeasersDeviceConfigNew;
}

}
