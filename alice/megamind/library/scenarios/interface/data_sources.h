#pragma once

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/context/responses.h>
#include <alice/megamind/library/session/dialog_history.h>

#include <alice/megamind/protos/scenarios/request.pb.h>  // TODO(g-kostin): move data sources to separate proto file
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/common/smart_home.pb.h>
#include <alice/megamind/protos/quasar/auxiliary_config.pb.h>

#include <alice/library/geo/user_location.h>

#include <alice/protos/data/contacts.pb.h>

namespace NAlice::NMegamind {

struct IDataSources {
    virtual const NScenarios::TDataSource& GetDataSource(EDataSourceType type) = 0;
    virtual ~IDataSources() = default;
};

// Simple cache for scenarios' data sources to prepare each only once and only if requested.
class TDataSources : public IDataSources {
public:
    using TVideoViewState = google::protobuf::Struct;

public:
    TDataSources(const IResponses* responses,
                 const TUserLocation* userLocation, const TDialogHistory* dialogHistory,
                 const google::protobuf::Map<TString, NScenarios::TFrameAction>* actions,
                 const NScenarios::TLayout* layout, const TSmartHomeInfo* smartHomeInfo,
                 const TVideoViewState* videoViewState, const TNotificationState* notificationState,
                 const NScenarios::TSkillDiscoverySaasCandidates* skillDiscoverySaasCandidates,
                 const NQuasarAuxiliaryConfig::TAuxiliaryConfig* auxiliaryConfig, TRTLogger& logger,
                 const TDeviceState& deviceState, const TIoTUserInfo* ioTUserInfo,
                 const NScenarios::TAppInfo* appInfo, TItemProxyAdapter& itemProxyAdapter, TString rawPersonalData,
                 const NAlice::TDeviceState_TVideo_TCurrentlyPlaying* videoCurrentlyPlaying,
                 const NAlice::NData::TContactsList* contactsList,
                 const TEnvironmentState* environmentState,
                 const TTandemEnvironmentState* tandemEnvironmentState,
                 const TString& webSearchQuery,
                 const TMaybe<TRequest::TWhisperInfo>& whisperInfo,
                 const TMaybe<TGuestData>& guestData,
                 const TMaybe<TGuestOptions>& guestOptions)
        : Responses(responses)
        , UserLocation(userLocation)
        , DialogHistory(dialogHistory)
        , Actions(actions)
        , Layout(layout)
        , SmartHomeInfo(smartHomeInfo)
        , VideoViewState(videoViewState)
        , NotificationState(notificationState)
        , SkillDiscoverySaasCandidates(skillDiscoverySaasCandidates)
        , AuxiliaryConfig(auxiliaryConfig)
        , Logger(logger)
        , DeviceState(deviceState)
        , IoTUserInfo(ioTUserInfo)
        , AppInfo(appInfo)
        , ItemProxyAdapter(itemProxyAdapter)
        , RawPersonalData(rawPersonalData)
        , VideoCurrentlyPlaying(videoCurrentlyPlaying)
        , ContactsList(contactsList)
        , EnvironmentState(environmentState)
        , TandemEnvironmentState(tandemEnvironmentState)
        , WebSearchQuery(webSearchQuery)
        , WhisperInfo(whisperInfo)
        , GuestData(guestData)
        , GuestOptions(guestOptions)
    {
    }

    const NScenarios::TDataSource& GetDataSource(EDataSourceType type) override;

private:
    struct TGuardedDataSource {
        TMaybe<NScenarios::TDataSource> DataSource;
        std::once_flag Flag;
    };

private:
    void CreateDataSource(EDataSourceType type);

private:
    std::array<TGuardedDataSource, EDataSourceType_ARRAYSIZE> DataSources;
    const IResponses* Responses;
    const TUserLocation* UserLocation;
    const TDialogHistory* DialogHistory;
    const google::protobuf::Map<TString, NScenarios::TFrameAction>* Actions;
    const NScenarios::TLayout* Layout;
    const TSmartHomeInfo* SmartHomeInfo;
    const TVideoViewState* VideoViewState;
    const TNotificationState* NotificationState;
    const NScenarios::TSkillDiscoverySaasCandidates* SkillDiscoverySaasCandidates;
    const NQuasarAuxiliaryConfig::TAuxiliaryConfig* AuxiliaryConfig;
    TRTLogger& Logger;
    const TDeviceState& DeviceState;
    const TIoTUserInfo* IoTUserInfo;
    const NScenarios::TAppInfo* AppInfo;
    TItemProxyAdapter& ItemProxyAdapter;
    TString RawPersonalData;
    const NAlice::TDeviceState_TVideo_TCurrentlyPlaying* VideoCurrentlyPlaying;
    const NAlice::NData::TContactsList* ContactsList;
    const TEnvironmentState* EnvironmentState;
    const TTandemEnvironmentState* TandemEnvironmentState;
    const TString WebSearchQuery;
    const TMaybe<TRequest::TWhisperInfo>& WhisperInfo;
    const TMaybe<TGuestData>& GuestData;
    const TMaybe<TGuestOptions>& GuestOptions;
};

inline bool IsDataSourceFilled(const NScenarios::TDataSource& dataSource) {
    return dataSource.GetTypeCase() != NScenarios::TDataSource::TYPE_NOT_SET;
}

inline bool IsDeviceStateDataSource(const EDataSourceType dataSourceType) {
    switch (dataSourceType) {
        case EDataSourceType::DEVICE_STATE_NAVIGATOR:
            [[fallthrough]];
        case EDataSourceType::EMPTY_DEVICE_STATE:
            return true;
        default:
            return false;
    }
}

} // namespace NAlice::NMegamind
