//
// HOLLYWOOD FRAMEWORK
// Common runtime data storage
//

#pragma once

#include <alice/library/logger/logger.h>

#include <google/protobuf/any.pb.h>

#include <util/generic/maybe.h>

//
// Forward declarations
//
namespace NAlice::NScenarios {
    class TScenarioBaseRequest;
    class TScenarioResponseBody;
    class TMementoChangeUserObjectsDirective;
}
namespace ru::yandex::alice::memento::proto {
    class TUserConfigs;
    class TSurfaceConfig;
    class TDeviceConfigs;
}

namespace NAlice::NHollywoodFw {

//
// Forward declarations
//
namespace NPrivate {
    class TNodeCaller;
} // namespace NPrivate

class TProtoHwFramework;
class TProtoHwScene;

//
// Default timeout for GetScenarioState
//
constexpr std::chrono::seconds DEFAULT_STORAGE_TIMEOUT = std::chrono::seconds(300);

//
// Generic storage functions
//
class TStorage {
friend class NPrivate::TNodeCaller;
public:
    ~TStorage();
    //
    // Interfaces for scenario state
    // =============================
    //

    // Result of GetScenarioState()
    enum class EScenarioStateResult {
        // Request doesn't contain any information about given scenario state
        Absent,
        // Request contains scenario state but this state was expired (see TScenario::SetScenarioStateTimeout())
        Expired,
        // Request contains scenario state but new session is started
        NewSession,
        // Request contains scenario state
        Present
    };

    /*
        Unpack scenario state from request
        @param [IN/OUT] arg protobuf struct to fill data
        @return EScenarioStateResult (see above)
    */
    template <class TScenarioStateProto>
    EScenarioStateResult GetScenarioState(TScenarioStateProto& arg, std::chrono::seconds timeout = DEFAULT_STORAGE_TIMEOUT) const {
        if (!ScenarioState_.Is<TScenarioStateProto>()) {
            return EScenarioStateResult::Absent;
        }
        if (!ScenarioState_.UnpackTo(&arg)) {
            return EScenarioStateResult::Absent;
        }
        if (IsNewSession_) {
            return EScenarioStateResult::NewSession;
        }
        if (ExpirationTime_ + timeout < CurrentServerTime_) {
            return EScenarioStateResult::Expired;
        }
        return EScenarioStateResult::Present;
    }

    template <class TScenarioStateProto>
    void SetScenarioState(const TScenarioStateProto& state) {
        if (!NewState_.PackFrom(state)) {
        }
        ApplyNewStateFlag_ = true;
    }

    bool HasChanges() const {
        return ApplyNewStateFlag_;
    }
    void DiscardChanges() {
        ApplyNewStateFlag_ = false;
    }

    //
    // Interfaces for TMemento storage
    // ===============================
    //

    // Read memento data
    const ru::yandex::alice::memento::proto::TUserConfigs& GetMementoUserConfig() const {
        return MementoUserConfig_;
    }
    const ru::yandex::alice::memento::proto::TSurfaceConfig& GetMementoSurfaceConfig() const {
        return MementoSurfaceConfig_;
    }
    template <class TProto>
    bool GetMementoScenarioData(TProto& proto) const {
        if (!MementoScenarioData_.Is<TProto>() || !MementoScenarioData_.UnpackTo(&proto)) {
            return false;
        }
        return true;
    }
    template <class TProto>
    bool GetMementoSurfaceScenarioData(TProto& proto) const {
        if (!MementoSurfaceScenarioData_.Is<TProto>() || !MementoSurfaceScenarioData_.UnpackTo(&proto)) {
            return false;
        }
        return true;
    }

    // Write memento data
    template <class TProto>
    bool AddMementoUserConfig(int configKey, const TProto& proto) {
        google::protobuf::Any any;
        if (!any.PackFrom(proto)) {
            LOG_ERROR(Logger_) << "PackFrom(): failed";
            return false;
        }
        AddMementoUserConfig(configKey, std::move(any));
        return true;
    }
    template <class TProto>
    bool AddMementoDeviceConfig(const TString& deviceId, int deviceConfigKey, const TProto& proto) {
        google::protobuf::Any any;
        if (!any.PackFrom(proto)) {
            LOG_ERROR(Logger_) << "PackFrom(): failed";
            return false;
        }
        AddMementoDeviceConfig(deviceId, deviceConfigKey, std::move(any));
        return true;
    }
    template <class TProto>
    bool AddMementoScenarioData(const TProto& proto) {
        google::protobuf::Any any;
        if (!any.PackFrom(proto)) {
            LOG_ERROR(Logger_) << "PackFrom(): failed";
            return false;
        }
        AddMementoScenarioData(std::move(any));
        return true;
    }
    template <class TProto>
    bool AddMementoSurfaceScenarioData(const TString& key, const TProto& proto) {
        google::protobuf::Any any;
        if (!any.PackFrom(proto)) {
            LOG_ERROR(Logger_) << "PackFrom(): failed";
            return false;
        }
        AddMementoSurfaceScenarioData(key, std::move(any));
        return true;
    }
    // Setup whole memento data
    // Note this function completely override all previous calls
    void SetMementoConfig(NScenarios::TMementoChangeUserObjectsDirective&& mementoData);

    //
    // Interfaces for Global Context
    //
    // TODO


    // Internal function
    void ToProto(TProtoHwScene& sceneResults) const;
    void BuildAnswer(NScenarios::TScenarioResponseBody* response, TProtoHwFramework& hwFrameworkState);

private:
    explicit TStorage(const NScenarios::TScenarioBaseRequest& baseRequest,
                      const TProtoHwFramework& hwFrameworkState,
                      const TProtoHwScene* protoScene,
                      TRTLogger& logger);

    void SetState(const google::protobuf::Any& newState);

    NScenarios::TMementoChangeUserObjectsDirective& CheckAndCreateMemento();
    void AddMementoUserConfig(int configKey, google::protobuf::Any&& any);
    void AddMementoDeviceConfig(const TString& deviceId, int deviceConfigKey, google::protobuf::Any&& any);
    void AddMementoScenarioData(google::protobuf::Any&& any);
    void AddMementoSurfaceScenarioData(const TString& key, google::protobuf::Any&& any);

private:
    TRTLogger& Logger_;
    const TProtoHwFramework& FrameworkState_;
    const google::protobuf::Any& ScenarioState_;
    bool IsNewSession_;
    std::chrono::milliseconds CurrentServerTime_;
    std::chrono::milliseconds ExpirationTime_;
    google::protobuf::Any NewState_;
    bool ApplyNewStateFlag_;

    // Read only memento configuration
    const ru::yandex::alice::memento::proto::TUserConfigs& MementoUserConfig_;
    const ru::yandex::alice::memento::proto::TSurfaceConfig& MementoSurfaceConfig_;
    const google::protobuf::Any& MementoScenarioData_;
    const google::protobuf::Any& MementoSurfaceScenarioData_;

    // Write memento configuration
    std::unique_ptr<NScenarios::TMementoChangeUserObjectsDirective> MementoWriter_;
};

} // namespace NAlice::NHollywoodFw
