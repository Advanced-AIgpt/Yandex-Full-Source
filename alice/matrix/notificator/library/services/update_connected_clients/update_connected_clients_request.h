#pragma once

#include <alice/matrix/notificator/library/services/update_connected_clients/protos/service.pb.h>
#include <alice/matrix/notificator/library/storages/connections/storage.h>
#include <alice/matrix/notificator/library/storages/directives/storage.h>

#include <alice/matrix/library/request/typed_apphost_request.h>

namespace NMatrix::NNotificator {

void PatchUpdateConnectedClientsRequestDataEvent(NEvClass::TMatrixNotificatorUpdateConnectedClientsRequestData& event);


class TUpdateConnectedClientsRequest : public TTypedAppHostRequest<
    NServiceProtos::TUpdateConnectedClientsRequest,
    NServiceProtos::TUpdateConnectedClientsResponse,
    NEvClass::TMatrixNotificatorUpdateConnectedClientsRequestData,
    NEvClass::TMatrixNotificatorUpdateConnectedClientsResponseData,
    PatchUpdateConnectedClientsRequestDataEvent
> {
public:
    TUpdateConnectedClientsRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        NAppHost::TTypedServiceContextPtr ctx,
        const NServiceProtos::TUpdateConnectedClientsRequest& request,
        TConnectionsStorage& connectionsStorage,
        TDirectivesStorage& directivesStorage,
        const bool disableYDBOperationsForDiffUpdates,
        const bool disableYDBOperationsForFullStateUpdates,
        const bool disableYDBOperationsForDirectivesSelects
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TConnectionsStorage::TConnectionsDiff GetConnectionsDiff() const;
    TConnectionsStorage::TConnectionsFullState GetConnectionsFullState() const;

    NThreading::TFuture<TExpected<void, TString>> UpdateConnectionsWithDiff();
    NThreading::TFuture<TExpected<TConnectionsStorage::TUpdateConnectionsWithFullStateResult, TString>> UpdateConnectionsWithFullState();
    NThreading::TFuture<TExpected<TDirectivesStorage::TGetDirectivesMultiUserDevicesResult, TString>> GetDirectivesForAddedClients(
        TMaybe<TConnectionsStorage::TUpdateConnectionsWithFullStateResult> updateConnectionsWithFullStateResult
    );

    NThreading::TFuture<void> OnUpdateConnectionsWithDiff(
        const NThreading::TFuture<TExpected<void, TString>>& updateConnectionsWithDiffResultFut
    );

    NThreading::TFuture<void> OnUpdateConnectionsWithFullState(
        const NThreading::TFuture<TExpected<TConnectionsStorage::TUpdateConnectionsWithFullStateResult, TString>>& updateConnectionsWithFullStateResultFut
    );

    NThreading::TFuture<void> OnGetDirectivesForAddedClients(
        const NThreading::TFuture<TExpected<TDirectivesStorage::TGetDirectivesMultiUserDevicesResult, TString>>& getDirectivesForAddedClientsResultFut
    );

    void OnEmptyPuidOrDeviceId(
        const TString& puid,
        const TString& deviceId,
        const TString& source
    ) const;

public:
    static inline constexpr TStringBuf NAME = "update_connected_clients";

private:
    const NApi::TUpdateConnectedClientsRequest& ApiRequest_;
    NApi::TUpdateConnectedClientsResponse& ApiResponse_;

    TConnectionsStorage& ConnectionsStorage_;
    TDirectivesStorage& DirectivesStorage_;

    const bool DisableYDBOperationsForDiffUpdates_;
    const bool DisableYDBOperationsForFullStateUpdates_;
    const bool DisableYDBOperationsForDirectivesSelects_;

    TConnectionsStorage::TEndpoint Endpoint_;
    TConnectionsStorage::TConnectionsDiff ConnectionsDiff_;
    TConnectionsStorage::TConnectionsFullState ConnectionsFullState_;
};

} // namespace NMatrix::NNotificator
