#pragma once

#include <alice/megamind/library/models/directives/uniproxy_directive_model.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/memento/proto/api.pb.h>

#include <google/protobuf/any.pb.h>

namespace NAlice::NMegamind {

class TMementoChangeUserObjectsDirectiveModel final : public TUniproxyDirectiveModel {
public:
    using TRecChangeUserObjects = ru::yandex::alice::memento::proto::TReqChangeUserObjects;

    TMementoChangeUserObjectsDirectiveModel(const TString& scenarioName,
                                            const NScenarios::TMementoChangeUserObjectsDirective& directive)
        : TUniproxyDirectiveModel(TString{ModelName})
    {
        const auto& userObjects = directive.GetUserObjects();

        UserObjects.MutableUserConfigs()->CopyFrom(userObjects.GetUserConfigs());
        UserObjects.MutableDevicesConfigs()->CopyFrom(userObjects.GetDevicesConfigs());
        if (userObjects.HasScenarioData()) {
            (*UserObjects.MutableScenarioData())[scenarioName] = userObjects.GetScenarioData();
        }
        if (userObjects.HasSurfaceScenarioData()) {
            (*UserObjects.MutableSurfaceScenarioData())[scenarioName] = userObjects.GetSurfaceScenarioData();
        }
    }

    TMementoChangeUserObjectsDirectiveModel(const TString& scenarioName, const google::protobuf::Message& data)
        : TUniproxyDirectiveModel(TString{ModelName})
    {
        google::protobuf::Any anyData;
        anyData.PackFrom(data);
        (*UserObjects.MutableScenarioData())[scenarioName] = std::move(anyData);
    }

    TMementoChangeUserObjectsDirectiveModel(ru::yandex::alice::memento::proto::EConfigKey userConfigKey, const google::protobuf::Message& data)
        : TUniproxyDirectiveModel(TString{ModelName})
    {
        google::protobuf::Any anyData;
        anyData.PackFrom(data);
        auto& userConfig = *(UserObjects.MutableUserConfigs()->Add());
        userConfig.SetKey(userConfigKey);
        (*userConfig.MutableValue()) = std::move(anyData);
    }

    void Accept(IModelSerializer& serializer) const final {
        return serializer.Visit(*this);
    }

    [[nodiscard]] const TRecChangeUserObjects& GetUserObjects() const {
        return UserObjects;
    }

private:
    constexpr static TStringBuf ModelName = "update_memento";
    TRecChangeUserObjects UserObjects;
};

} // namespace NAlice::NMegamind
