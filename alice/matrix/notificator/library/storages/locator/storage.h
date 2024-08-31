#pragma once

#include <alice/matrix/notificator/library/storages/connections/storage.h>

#include <alice/uniproxy/library/protos/notificator.pb.h>

namespace NMatrix::NNotificator {

class TLocatorStorage : public IYDBStorage {
public:
    TLocatorStorage(
        const NYdb::TDriver& driver,
        const TYDBClientSettings& config
    );

    NThreading::TFuture<TExpected<void, TString>> Store(
        const ::NNotificator::TDeviceLocator& msg,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<void, TString>> Remove(
        const ::NNotificator::TDeviceLocator& msg,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> List(
        const TString& puid,
        const TString& deviceId,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

public:
    static inline constexpr ui32 SUBWAY_PORT = 80;

private:
    static inline constexpr TStringBuf NAME = "locator";
};

} // namespace NMatrix::NNotificator
