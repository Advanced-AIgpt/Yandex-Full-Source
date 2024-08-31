//
// HOLLYWOOD FRAMEWORK
// Common runtime data storage
//

#include "storage.h"

#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

#include <alice/library/json/json.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/memento/proto/api.pb.h>

namespace NAlice::NHollywoodFw {

/*
    TStorage ctor
    Internal function, called automatically
*/
TStorage::TStorage(const NScenarios::TScenarioBaseRequest& baseRequest,
                   const TProtoHwFramework& hwFrameworkState,
                   const TProtoHwScene* protoScene,
                   TRTLogger& logger)
    : Logger_(logger)
    , FrameworkState_(hwFrameworkState)
    , ScenarioState_(hwFrameworkState.GetScenarioState())
    , IsNewSession_(baseRequest.GetIsNewSession())
    , CurrentServerTime_(std::chrono::milliseconds(baseRequest.GetServerTimeMs()))
    , ExpirationTime_(std::chrono::milliseconds(hwFrameworkState.GetLastTimeUpdateStorage()))
    , ApplyNewStateFlag_(false)
    , MementoUserConfig_(baseRequest.GetMemento().GetUserConfigs())
    , MementoSurfaceConfig_(baseRequest.GetMemento().GetSurfaceConfig())
    , MementoScenarioData_(baseRequest.GetMemento().GetScenarioData())
    , MementoSurfaceScenarioData_(baseRequest.GetMemento().GetSurfaceScenarioData())
{
    if (protoScene != nullptr && protoScene->HasMementoDirective()) {
        auto& writer = CheckAndCreateMemento();
        writer.CopyFrom(protoScene->GetMementoDirective());
    }
}

TStorage::~TStorage() = default;

void TStorage::SetMementoConfig(NScenarios::TMementoChangeUserObjectsDirective&& mementoData) {
    MementoWriter_.reset(new NScenarios::TMementoChangeUserObjectsDirective(std::move(mementoData)));
}

/*
    Creates new object if it doesn't exist
    OR
    Returns currently existing memento writer object
*/
NScenarios::TMementoChangeUserObjectsDirective& TStorage::CheckAndCreateMemento() {
    if (!MementoWriter_) {
        MementoWriter_ = std::make_unique<NScenarios::TMementoChangeUserObjectsDirective>();
    }
    return *MementoWriter_.get();
}

void TStorage::AddMementoUserConfig(int configKey, google::protobuf::Any&& any) {
    auto& writer = CheckAndCreateMemento();
    ru::yandex::alice::memento::proto::TConfigKeyAnyPair cfg;
    cfg.SetKey(static_cast<ru::yandex::alice::memento::proto::EConfigKey>(configKey));
    *cfg.MutableValue() = std::move(any);
    *writer.MutableUserObjects()->AddUserConfigs() = std::move(cfg);
}

void TStorage::AddMementoDeviceConfig(const TString& deviceId, int deviceConfigKey, google::protobuf::Any&& any) {
    auto& writer = CheckAndCreateMemento();
    ru::yandex::alice::memento::proto::TDeviceConfigsKeyAnyPair keyvalue;
    keyvalue.SetKey(static_cast<ru::yandex::alice::memento::proto::EDeviceConfigKey>(deviceConfigKey));
    *keyvalue.MutableValue() = std::move(any);
    // Looking for this key in array
    for (int i = 0; i < writer.GetUserObjects().GetDevicesConfigs().size(); ++i) {
        if (writer.GetUserObjects().GetDevicesConfigs()[i].GetDeviceId() == deviceId) {
            // Will use this object to add confuguration data
            *writer.MutableUserObjects()->MutableDevicesConfigs()->at(i).AddDeviceConfigs() = std::move(keyvalue);
            return;
        }
    }
    // Not found, add new
    ru::yandex::alice::memento::proto::TDeviceConfigs dcfg;
    dcfg.SetDeviceId(deviceId);
    *dcfg.AddDeviceConfigs() = std::move(keyvalue);
    *writer.MutableUserObjects()->AddDevicesConfigs() = std::move(dcfg);
}

void TStorage::AddMementoScenarioData(google::protobuf::Any&& any) {
    auto& writer = CheckAndCreateMemento();
    *writer.MutableUserObjects()->MutableScenarioData() = std::move(any);
}

void TStorage::AddMementoSurfaceScenarioData(const TString& key, google::protobuf::Any&& any) {
    auto& writer = CheckAndCreateMemento();
    (*writer.MutableUserObjects()->MutableSurfaceScenarioData()->MutableScenarioData())[key] = std::move(any);
}

void TStorage::ToProto(TProtoHwScene& sceneResults) const {
    if (MementoWriter_) {
        sceneResults.MutableMementoDirective()->CopyFrom(*MementoWriter_);
    }
}

/*
    Attach TProtoHwFramework and Scenario State into response
*/
void TStorage::BuildAnswer(NAlice::NScenarios::TScenarioResponseBody* response, TProtoHwFramework& hwFrameworkState) {
    if (ApplyNewStateFlag_) {
        hwFrameworkState.SetLastTimeUpdateStorage(CurrentServerTime_.count());
        *hwFrameworkState.MutableScenarioState() = NewState_;
    }
    if (response) {
        response->MutableState()->PackFrom(hwFrameworkState);
    }
    if (MementoWriter_) {
        NScenarios::TServerDirective directive;
        auto* directive2 = directive.MutableMementoChangeUserObjectsDirective();
        *directive2 = std::move(*MementoWriter_);
        *response->AddServerDirectives() = std::move(directive);
    }
}

} // namespace NAlice::NHollywoodFw
