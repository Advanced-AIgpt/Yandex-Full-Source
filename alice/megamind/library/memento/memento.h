#pragma once

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/speechkit/request.h>

#include <alice/memento/proto/api.pb.h>

#include <alice/library/logger/logger.h>

#include <google/protobuf/any.pb.h>

namespace NAlice::NMegamind {

namespace NMementoApi = ru::yandex::alice::memento::proto;

class TMementoData {
public:
    TMementoData();

    explicit TMementoData(NMementoApi::TRespGetAllObjects&& getAllObjectsResponse);
    explicit TMementoData(const NMementoApi::TRespGetAllObjects& getAllObjectsResponse);

    const google::protobuf::Any* GetScenarioData(const TString& scenarioName) const;
    const google::protobuf::Any* GetSurfaceScenarioData(const TString& scenarioName,
                                                        const TString& deviceId,
                                                        const TString& uuid) const;

    const NMementoApi::TUserConfigs& GetUserConfigs() const;

    void PutDataIntoContext(TItemProxyAdapter& itemProxyAdapter) const;

    // TODO(g-kostin): Implement UserConfigs filtering (MEGAMIND-2128)
    // TODO(g-kostin): Implement SurfaceConfig filtering (MEGAMIND-2129)

private:
    NMementoApi::TRespGetAllObjects GetAllObjectsResponse;
};

class TMementoDataView {
public:
    TMementoDataView(const TMementoData& mementoData, const TString& scenarioName,
                     const TString& deviceId, const TString& uuid)
        : MementoData(mementoData)
        , ScenarioName(scenarioName)
        , DeviceId(deviceId)
        , Uuid(uuid)
    {}

    const google::protobuf::Any* GetScenarioData() const {
        return MementoData.GetScenarioData(ScenarioName);
    }

    const google::protobuf::Any* GetSurfaceScenarioData() const {
        return MementoData.GetSurfaceScenarioData(ScenarioName, DeviceId, Uuid);
    }

    const NMementoApi::TUserConfigs& GetUserConfigs() const {
        return MementoData.GetUserConfigs();
    }

private:
    const TMementoData& MementoData;
    const TString& ScenarioName;
    const TString& DeviceId;
    const TString& Uuid;
};

TMementoData DeserializeMementoData(const TString& encodedString);
TMementoData GetMementoFromSpeechKitRequest(const TSpeechKitRequest& skr, TRTLogger& logger);

} // namespace NAlice::NMegamind
