#include "memento.h"

#include <alice/library/proto/protobuf.h>

#include <util/generic/mapfindptr.h>

namespace NAlice::NMegamind {

TMementoData::TMementoData()
    : GetAllObjectsResponse(NMementoApi::TRespGetAllObjects::default_instance())
    {}

TMementoData::TMementoData(NMementoApi::TRespGetAllObjects&& getAllObjectsResponse)
    : GetAllObjectsResponse(std::move(getAllObjectsResponse))
    {}

TMementoData::TMementoData(const NMementoApi::TRespGetAllObjects& getAllObjectsResponse)
    : GetAllObjectsResponse(getAllObjectsResponse)
    {}

const google::protobuf::Any* TMementoData::GetScenarioData(const TString& scenarioName) const {
    return MapFindPtr(GetAllObjectsResponse.GetScenarioData(), scenarioName);
}

const google::protobuf::Any* TMementoData::GetSurfaceScenarioData(const TString& scenarioName,
                                                                  const TString& deviceId,
                                                                  const TString& uuid) const {
    const auto* surfaceScenarioDataMap =
        deviceId.empty()
            ? MapFindPtr(GetAllObjectsResponse.GetSurfaceScenarioData(), uuid)
            : MapFindPtr(GetAllObjectsResponse.GetSurfaceScenarioData(), deviceId);
    if (surfaceScenarioDataMap) {
        return MapFindPtr(surfaceScenarioDataMap->GetScenarioData(), scenarioName);
    }
    return nullptr;
}

const NMementoApi::TUserConfigs& TMementoData::GetUserConfigs() const {
    return GetAllObjectsResponse.GetUserConfigs();
}

void TMementoData::PutDataIntoContext(TItemProxyAdapter& itemProxyAdapter) const {
    itemProxyAdapter.PutIntoContext(GetAllObjectsResponse, AH_ITEM_FULL_MEMENTO_DATA);
}

TMementoData DeserializeMementoData(const TString& encodedString) {
    NMementoApi::TRespGetAllObjects objects;
    ProtoFromBase64String(encodedString, objects);
    return TMementoData(std::move(objects));
}

TMementoData GetMementoFromSpeechKitRequest(const TSpeechKitRequest& skr, TRTLogger& logger) {
    if (skr->HasMementoData()) {
        try {
            return DeserializeMementoData(skr->GetMementoData());
        } catch (...) {
            LOG_ERR(logger) << "Failed to deserialize memento data: " << CurrentExceptionMessage();
        }
    }
    return {};
}

} // namespace NAlice::NMegamind
