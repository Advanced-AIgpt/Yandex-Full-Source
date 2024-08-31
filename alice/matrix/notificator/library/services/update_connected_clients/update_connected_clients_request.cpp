#include "update_connected_clients_request.h"

#include <alice/protos/api/matrix/technical_push.pb.h>
#include <alice/protos/api/matrix/user_device.pb.h>

namespace NMatrix::NNotificator {

namespace {

using std::placeholders::_1;

TDeviceInfo ConvertApiUserDeviceInfoToDeviceInfo(const NApi::TUserDeviceInfo& userDeviceInfo) {
    TDeviceInfo deviceInfo;
    deviceInfo.SetDeviceModel(userDeviceInfo.GetDeviceModel());
    deviceInfo.MutableSupportedFeatures()->CopyFrom(userDeviceInfo.GetSupportedFeatures());

    return deviceInfo;
}

} // namespace

void PatchUpdateConnectedClientsRequestDataEvent(NEvClass::TMatrixNotificatorUpdateConnectedClientsRequestData& event) {
    if (event.GetRequest().GetApiRequest().HasClientsFullInfo()) {
        event.MutableRequest()->MutableApiRequest()->MutableClientsFullInfo()->ClearClients();
    }
}

TUpdateConnectedClientsRequest::TUpdateConnectedClientsRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    NAppHost::TTypedServiceContextPtr ctx,
    const NServiceProtos::TUpdateConnectedClientsRequest& request,
    TConnectionsStorage& connectionsStorage,
    TDirectivesStorage& directivesStorage,
    const bool disableYDBOperationsForDiffUpdates,
    const bool disableYDBOperationsForFullStateUpdates,
    const bool disableYDBOperationsForDirectivesSelects
)
    : TTypedAppHostRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        ctx,
        request
    )
    , ApiRequest_(Request_.GetApiRequest())
    , ApiResponse_(*Response_.MutableApiResponse())
    , ConnectionsStorage_(connectionsStorage)
    , DirectivesStorage_(directivesStorage)
    , DisableYDBOperationsForDiffUpdates_(disableYDBOperationsForDiffUpdates)
    , DisableYDBOperationsForFullStateUpdates_(disableYDBOperationsForFullStateUpdates)
    , DisableYDBOperationsForDirectivesSelects_(disableYDBOperationsForDirectivesSelects)
    , Endpoint_({
        .Ip = ApiRequest_.GetUniproxyEndpoint().GetIp(),
        .ShardId = ApiRequest_.GetShardId(),
        .Port = ApiRequest_.GetUniproxyEndpoint().GetPort(),
        .Monotonic = ApiRequest_.GetMonotonicTimestamp(),
    })
    , ConnectionsDiff_(GetConnectionsDiff())
    , ConnectionsFullState_(GetConnectionsFullState())
{
    if (IsFinished()) {
        return;
    }

    if (!Request_.HasApiRequest()) {
        SetError("ApiRequest not found in request");
        Metrics_.SetError("api_request_not_found");
        IsFinished_ = true;
        return;
    }

    LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorUpdateConnectionsEndpoint>(
        Endpoint_.Ip,
        Endpoint_.ShardId,
        Endpoint_.Port,
        Endpoint_.Monotonic
    );
}

NThreading::TFuture<void> TUpdateConnectedClientsRequest::ServeAsync() {
    // Always apply diff
    return UpdateConnectionsWithDiff().Apply(
        std::bind(&TUpdateConnectedClientsRequest::OnUpdateConnectionsWithDiff, this, _1)
    );
}

TConnectionsStorage::TConnectionsDiff TUpdateConnectedClientsRequest::GetConnectionsDiff() const {
    TConnectionsStorage::TConnectionsDiff result;
    result.Endpoint = Endpoint_;

    TMap<std::pair<TString, TString>, TConnectionsStorage::TUserDeviceInfo> connectedClients;
    TMap<std::pair<TString, TString>, TConnectionsStorage::TUserDeviceInfo> disconnectedClients;

    for (const auto& clientStateChange : ApiRequest_.GetClientsDiff().GetClientStateChanges()) {
        const auto& puid = clientStateChange.GetUserDeviceInfo().GetUserDeviceIdentifier().GetPuid();
        const auto& deviceId = clientStateChange.GetUserDeviceInfo().GetUserDeviceIdentifier().GetDeviceId();

        if (puid.empty() || deviceId.empty()) {
            OnEmptyPuidOrDeviceId(puid, deviceId, "diff");
            continue;
        }

        std::pair<TString, TString> mapKey = {
            puid,
            deviceId
        };
        TConnectionsStorage::TUserDeviceInfo userDeviceInfo = {
            .Puid = puid,
            .DeviceId = deviceId,
            .DeviceInfo = ConvertApiUserDeviceInfoToDeviceInfo(clientStateChange.GetUserDeviceInfo()),
        };
        switch (clientStateChange.GetAction()) {
            case NApi::TClientStateChange::CONNECT: {
                if (auto it = disconnectedClients.find(mapKey); it != disconnectedClients.end()) {
                    disconnectedClients.erase(it);
                } else {
                    connectedClients[mapKey] = userDeviceInfo;
                }
                break;
            }
            case NApi::TClientStateChange::DISCONNECT: {
                if (auto it = connectedClients.find(mapKey); it != connectedClients.end()) {
                    connectedClients.erase(it);
                } else {
                    disconnectedClients[mapKey] = userDeviceInfo;
                }
                break;
            }
            default: {
                LogContext_.LogEventErrorCombo<NEvClass::TMatrixNotificatorUnknownClientStateChangeAction>(NApi::TClientStateChange::EAction_Name(clientStateChange.GetAction()));
                break;
            }
        }
    }

    result.ConnectedClients.reserve(connectedClients.size());
    for (auto& [_, userDeviceInfo] : connectedClients) {
        result.ConnectedClients.push_back(std::move(userDeviceInfo));
    }

    result.DisconnectedClients.reserve(disconnectedClients.size());
    for (auto& [_, userDeviceInfo] : disconnectedClients) {
        result.DisconnectedClients.push_back(std::move(userDeviceInfo));
    }

    return result;
}

TConnectionsStorage::TConnectionsFullState TUpdateConnectedClientsRequest::GetConnectionsFullState() const {
    TConnectionsStorage::TConnectionsFullState result;
    result.Endpoint = Endpoint_;

    if (ApiRequest_.GetAllConnectionsDroppedOnShutdown()) {
        return result;
    }

    result.ConnectedClients.reserve(ApiRequest_.GetClientsFullInfo().GetClients().size());
    for (const auto& client : ApiRequest_.GetClientsFullInfo().GetClients()) {
        const auto& puid = client.GetUserDeviceIdentifier().GetPuid();
        const auto& deviceId = client.GetUserDeviceIdentifier().GetDeviceId();

        if (puid.empty() || deviceId.empty()) {
            OnEmptyPuidOrDeviceId(puid, deviceId, "full_state");
            continue;
        }

        result.ConnectedClients.push_back({
            .Puid = puid,
            .DeviceId = deviceId,
            .DeviceInfo = ConvertApiUserDeviceInfoToDeviceInfo(client),
        });
    }

    return result;
}

NThreading::TFuture<TExpected<void, TString>> TUpdateConnectedClientsRequest::UpdateConnectionsWithDiff() {
    LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorUpdateConnectionsWithDiff>(
        ApiRequest_.GetClientsDiff().GetClientStateChanges().size(),
        ConnectionsDiff_.ConnectedClients.size(),
        ConnectionsDiff_.DisconnectedClients.size()
    );

    Metrics_.PushRate(ApiRequest_.GetClientsDiff().GetClientStateChanges().size(), "client_state_changes_in_diff");
    Metrics_.PushRate(ConnectionsDiff_.ConnectedClients.size(), "connected_clients_in_diff");
    Metrics_.PushRate(ConnectionsDiff_.DisconnectedClients.size(), "disconnected_clients_in_diff");
    Metrics_.PushRate(static_cast<ui64>(ApiRequest_.GetClientsDiff().GetClientStateChanges().empty()), "empty_client_state_changes");
    Metrics_.PushRate(
        static_cast<ui64>(ConnectionsDiff_.ConnectedClients.empty() && ConnectionsDiff_.DisconnectedClients.empty()),
        "empty_connected_and_disconnected_clients"
    );

    if ((ConnectionsDiff_.ConnectedClients.empty() && ConnectionsDiff_.DisconnectedClients.empty()) || DisableYDBOperationsForDiffUpdates_) {
        return NThreading::MakeFuture<TExpected<void, TString>>(TExpected<void, TString>::DefaultSuccess());
    }

    return ConnectionsStorage_.UpdateConnectionsWithDiff(
        ConnectionsDiff_,
        LogContext_,
        Metrics_
    );
}

NThreading::TFuture<TExpected<TConnectionsStorage::TUpdateConnectionsWithFullStateResult, TString>> TUpdateConnectedClientsRequest::UpdateConnectionsWithFullState() {
    LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorUpdateConnectionsWithFullState>(
        ConnectionsFullState_.ConnectedClients.size(),
        ApiRequest_.GetAllConnectionsDroppedOnShutdown()
    );

    Metrics_.PushRate(ConnectionsFullState_.ConnectedClients.size(), "full_state_clients_count");
    Metrics_.PushRate(static_cast<ui64>(ApiRequest_.GetAllConnectionsDroppedOnShutdown()), "all_connections_dropped_on_shutdown");

    if (DisableYDBOperationsForFullStateUpdates_) {
        return NThreading::MakeFuture<TExpected<TConnectionsStorage::TUpdateConnectionsWithFullStateResult, TString>>(
            TConnectionsStorage::TUpdateConnectionsWithFullStateResult({
                .AddedCount = 0,
                .RemovedCount = 0,
            })
        );
    }

    return ConnectionsStorage_.UpdateConnectionsWithFullState(
        ConnectionsFullState_,
        LogContext_,
        Metrics_
    );
}

NThreading::TFuture<TExpected<TDirectivesStorage::TGetDirectivesMultiUserDevicesResult, TString>> TUpdateConnectedClientsRequest::GetDirectivesForAddedClients(
    TMaybe<TConnectionsStorage::TUpdateConnectionsWithFullStateResult> updateConnectionsWithFullStateResult
) {
    TVector<TDirectivesStorage::TUserDevice> userDevices;

    for (const auto& connectedClient : ConnectionsDiff_.ConnectedClients) {
        userDevices.push_back({
            .Puid = connectedClient.Puid,
            .DeviceId = connectedClient.DeviceId,
        });
    }

    if (updateConnectionsWithFullStateResult.Defined()) {
        for (const auto& connectedClient : updateConnectionsWithFullStateResult->AddedClients) {
            userDevices.push_back({
                .Puid = connectedClient.Puid,
                .DeviceId = connectedClient.DeviceId,
            });
        }
    }

    Metrics_.PushRate(static_cast<ui64>(userDevices.empty()), "empty_user_devices_list_for_get_directives");

    if (userDevices.empty() || DisableYDBOperationsForDirectivesSelects_) {
        return NThreading::MakeFuture<TExpected<TDirectivesStorage::TGetDirectivesMultiUserDevicesResult, TString>>(
            TDirectivesStorage::TGetDirectivesMultiUserDevicesResult({
                .UserDirectives = {},
                .IsTruncated = false,
            })
        );
    }

    return DirectivesStorage_.GetDirectivesMultiUserDevices(
        userDevices,
        LogContext_,
        Metrics_
    );
}

NThreading::TFuture<void> TUpdateConnectedClientsRequest::OnUpdateConnectionsWithDiff(
    const NThreading::TFuture<TExpected<void, TString>>& updateConnectionsWithDiffResultFut
) {
    auto updateConnectionsWithDiffResult = updateConnectionsWithDiffResultFut.GetValueSync();
    if (!updateConnectionsWithDiffResult) {
        SetError(updateConnectionsWithDiffResult.Error());
        return NThreading::MakeFuture();
    }

    if (ApiRequest_.HasClientsFullInfo() || ApiRequest_.GetAllConnectionsDroppedOnShutdown()) {
        return UpdateConnectionsWithFullState().Apply(
            std::bind(&TUpdateConnectedClientsRequest::OnUpdateConnectionsWithFullState, this, _1)
        );
    } else {
        return GetDirectivesForAddedClients(/* updateConnectionsWithFullStateResult = */ Nothing()).Apply(
            std::bind(&TUpdateConnectedClientsRequest::OnGetDirectivesForAddedClients, this, _1)
        );
    }
}

NThreading::TFuture<void> TUpdateConnectedClientsRequest::OnUpdateConnectionsWithFullState(
    const NThreading::TFuture<TExpected<TConnectionsStorage::TUpdateConnectionsWithFullStateResult, TString>>& updateConnectionsWithFullStateResultFut
) {
    auto updateConnectionsWithFullStateResult = updateConnectionsWithFullStateResultFut.GetValueSync();
    if (!updateConnectionsWithFullStateResult) {
        SetError(updateConnectionsWithFullStateResult.Error());
        return NThreading::MakeFuture();
    }
    auto updateConnectionsWithFullState = updateConnectionsWithFullStateResult.Success();

    LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorUpdateConnectionsWithFullStateResult>(
        ApiRequest_.GetAllConnectionsDroppedOnShutdown(),
        updateConnectionsWithFullState.AddedCount,
        updateConnectionsWithFullState.RemovedCount,
        updateConnectionsWithFullState.AddedClients.size(),
        updateConnectionsWithFullState.IsAddedClientsTruncated
    );

    if (ApiRequest_.GetAllConnectionsDroppedOnShutdown()) {
        Metrics_.PushRate(updateConnectionsWithFullState.RemovedCount, "disconnected_clients_by_drop_on_shutdown");
    } else {
        Metrics_.PushRate(updateConnectionsWithFullState.AddedCount, "connected_clients_by_full_state");
        Metrics_.PushRate(updateConnectionsWithFullState.RemovedCount, "disconnected_clients_by_full_state");
    }

    Metrics_.PushRate(updateConnectionsWithFullState.AddedClients.size(), "added_clients_from_full_state_size");
    Metrics_.PushRate(static_cast<ui64>(updateConnectionsWithFullState.IsAddedClientsTruncated), "is_added_clients_from_full_state_truncated");

    return GetDirectivesForAddedClients(updateConnectionsWithFullState).Apply(
        std::bind(&TUpdateConnectedClientsRequest::OnGetDirectivesForAddedClients, this, _1)
    );
}

NThreading::TFuture<void> TUpdateConnectedClientsRequest::OnGetDirectivesForAddedClients(
    const NThreading::TFuture<TExpected<TDirectivesStorage::TGetDirectivesMultiUserDevicesResult, TString>>& getDirectivesForAddedClientsResultFut
) {
    auto getDirectivesForAddedClientsResult = getDirectivesForAddedClientsResultFut.GetValueSync();
    if (!getDirectivesForAddedClientsResult) {
        SetError(getDirectivesForAddedClientsResult.Error());
        return NThreading::MakeFuture();
    }
    auto getDirectivesForAddedClients = getDirectivesForAddedClientsResult.Success();

    LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorGetDirectivesMultiUserDevicesResult>(
        getDirectivesForAddedClients.UserDirectives.size(),
        getDirectivesForAddedClients.IsTruncated
    );

    Metrics_.PushRate(getDirectivesForAddedClients.UserDirectives.size(), "user_directives_for_added_clients_size");
    Metrics_.PushRate(static_cast<ui64>(getDirectivesForAddedClients.IsTruncated), "is_user_directives_for_added_clients_truncated");

    TMap<std::pair<TString, TString>, TVector<std::reference_wrapper<const TDirectivesStorage::TUserDirective>>> userDeviceToDirectives;
    TString endpointStr = TString::Join(Endpoint_.Ip, ':', ToString(Endpoint_.Port));
    for (const auto& userDirective : getDirectivesForAddedClients.UserDirectives) {
        // Important log for analytics
        LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorSendDirectiveToUpdateConnectedClientsResponse>(
            userDirective.Directive.PushId,
            endpointStr,
            userDirective.UserDevice.Puid,
            userDirective.UserDevice.DeviceId
        );
        userDeviceToDirectives[std::make_pair(userDirective.UserDevice.Puid, userDirective.UserDevice.DeviceId)].emplace_back(userDirective);
    }
    for (const auto& [userDevice, userDirectives] : userDeviceToDirectives) {
        const auto& [puid, deviceId] = userDevice;

        auto& technicalPushesForUserDevice = *ApiResponse_.AddTechnicalPushesForUserDevices();
        technicalPushesForUserDevice.MutableUserDeviceIdentifier()->SetPuid(puid);
        technicalPushesForUserDevice.MutableUserDeviceIdentifier()->SetDeviceId(deviceId);

        for (const auto& userDirective : userDirectives) {
            auto& technicalPush = *technicalPushesForUserDevice.AddTechnicalPushes();
            technicalPush.SetTechnicalPushId(userDirective.get().Directive.PushId);
            technicalPush.MutableSpeechKitDirective()->PackFrom(userDirective.get().Directive.SpeechKitDirective);
        }
    }

    return NThreading::MakeFuture();
}

void TUpdateConnectedClientsRequest::OnEmptyPuidOrDeviceId(
    const TString& puid,
    const TString& deviceId,
    const TString& source
) const {
    if (puid.empty() && deviceId.empty()) {
        Metrics_.PushRate("empty_puid_and_device_id", "error", source);
    } else if (puid.empty()) {
        Metrics_.PushRate("empty_puid", "error", source);
    } else { // deviceId.empty()
        Metrics_.PushRate("empty_device_id", "error", source);
    }
}

} // namespace NMatrix::NNotificator
