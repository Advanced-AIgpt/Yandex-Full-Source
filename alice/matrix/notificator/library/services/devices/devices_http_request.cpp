#include "devices_http_request.h"

#include <alice/matrix/notificator/library/storages/utils/utils.h>


namespace NMatrix::NNotificator {

TDevicesHttpRequest::TDevicesHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TLocatorStorage& locatorStorage,
    TConnectionsStorage& connectionsStorage,
    const bool useOldConnectionsStorage
)
    : THttpRequestWithProtoResponse(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Post == method;
        }
    )
    , LocatorStorage_(locatorStorage)
    , ConnectionsStorage_(connectionsStorage)
    , UseOldConnectionsStorage_(useOldConnectionsStorage)
{
    if (IsFinished()) {
        return;
    }

    if (Request_->GetPuid().empty()) {
        SetError("Puid is empty", 400);
        IsFinished_ = true;
        return;
    }

    NeededSupportedFeatures_.reserve(Request_->GetSupportedFeatures().size());
    for (const auto& supportedFeature : Request_->GetSupportedFeatures()) {
        if (const auto parsedSupportedFeature = TryParseSupportedFeatureFromString(supportedFeature)) {
            NeededSupportedFeatures_.push_back(*parsedSupportedFeature);
        } else {
            SetError(TString::Join("Supported feature '", supportedFeature, "' is not allowed (more info in ZION-284)"), 400);
            IsFinished_ = true;
            return;
        }
    }
}

NThreading::TFuture<void> TDevicesHttpRequest::ServeAsync() {
    return ListConnections().Apply(
        [this](const NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>>& connectionsFut) {
            const auto connectionsRes = connectionsFut.GetValueSync();
            if (!connectionsRes) {
                SetError(connectionsRes.Error(), 500);
                return;
            }

            NAlice::NNotificator::TGetDevicesResponse res;
            for (const auto& connectionRecord : connectionsRes.Success().Records) {
                // Check that supportedFeatures is subset of featuresSet.
                THashSet<NApi::TUserDeviceInfo::ESupportedFeature> supportedFeaturesSet;
                for (const auto& supportedFeature : connectionRecord.UserDeviceInfo.DeviceInfo.GetSupportedFeatures()) {
                    supportedFeaturesSet.insert(static_cast<NApi::TUserDeviceInfo::ESupportedFeature>(supportedFeature));
                }

                bool ok = true;
                for (const auto& supportedFeature : NeededSupportedFeatures_) {
                    if (!supportedFeaturesSet.contains(supportedFeature)) {
                        ok = false;
                        break;
                    }
                }

                if (ok) {
                    Response_.AddDevices()->SetDeviceId(connectionRecord.UserDeviceInfo.DeviceId);
                }
            }
        }
    );
}

NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> TDevicesHttpRequest::ListConnections() {
    if (UseOldConnectionsStorage_) {
        return LocatorStorage_.List(
            Request_->GetPuid(),
            /* deviceId = */ "",
            LogContext_,
            Metrics_
        );
    } else {
        return ConnectionsStorage_.ListConnections(
            Request_->GetPuid(),
            /* deviceId = */ Nothing(),
            LogContext_,
            Metrics_
        );
    }
}

} // namespace NMatrix::NNotificator
