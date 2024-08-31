#pragma once

#include <alice/matrix/notificator/library/storages/connections/storage.h>
#include <alice/matrix/notificator/library/storages/locator/storage.h>

#include <alice/matrix/library/request/http_request.h>


namespace NMatrix::NNotificator {

class TDevicesHttpRequest : public TProtoHttpRequestWithProtoResponse<
    NAlice::NNotificator::TGetDevicesRequest,
    NAlice::NNotificator::TGetDevicesResponse,
    NEvClass::TMatrixNotificatorDevicesHttpRequestData,
    NEvClass::TMatrixNotificatorDevicesHttpResponseData
> {
public:
    TDevicesHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TLocatorStorage& locatorStorage,
        TConnectionsStorage& connectionsStorage,
        const bool useOldConnectionsStorage
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> ListConnections();

public:
    static inline constexpr TStringBuf NAME = "devices";
    static inline constexpr TStringBuf PATH = "/devices";

private:
    TLocatorStorage& LocatorStorage_;
    TConnectionsStorage& ConnectionsStorage_;

    const bool UseOldConnectionsStorage_;

    TVector<NApi::TUserDeviceInfo::ESupportedFeature> NeededSupportedFeatures_;
};

} // namespace NMatrix::NNotificator
