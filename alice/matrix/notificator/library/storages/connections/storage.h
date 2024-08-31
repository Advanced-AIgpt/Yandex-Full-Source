#pragma once

#include <alice/matrix/notificator/library/storages/connections/protos/device_info.pb.h>

#include <alice/matrix/library/ydb/storage.h>

#include <alice/protos/api/matrix/user_device.pb.h>


namespace NMatrix::NNotificator {

class TConnectionsStorage : public IYDBStorage {
public:
    struct TEndpoint {
        TString Ip;
        ui64 ShardId;
        ui32 Port;
        ui64 Monotonic;

        ui64 GetTableShardKey() const;
    };

    struct TUserDeviceInfo {
        TString Puid;
        TString DeviceId;
        TDeviceInfo DeviceInfo;
    };

    struct TConnectionsDiff {
        TEndpoint Endpoint;
        TVector<TUserDeviceInfo> ConnectedClients;
        TVector<TUserDeviceInfo> DisconnectedClients;
    };

    struct TConnectionsFullState {
        TEndpoint Endpoint;
        TVector<TUserDeviceInfo> ConnectedClients;
    };

    struct TUpdateConnectionsWithFullStateResult {
        ui64 AddedCount;
        ui64 RemovedCount;

        // WARNING: This list probably truncated
        TVector<TUserDeviceInfo> AddedClients;
        bool IsAddedClientsTruncated;
    };

    struct TListConnectionsResult {
        struct TRecord {
            TEndpoint Endpoint;
            TUserDeviceInfo UserDeviceInfo;
        };

        TVector<TRecord> Records;
    };

private:
    struct TUpdateConnectionsWithFullStateAddConnectedResult {
        ui64 AddedCount;
        TVector<TUserDeviceInfo> AddedClients;
        bool IsAddedClientsTruncated;
    };

public:
    TConnectionsStorage(
        const NYdb::TDriver& driver,
        const TYDBClientSettings& config
    );

    NThreading::TFuture<TExpected<void, TString>> UpdateConnectionsWithDiff(
        const TConnectionsDiff& connectionsDiff,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<TUpdateConnectionsWithFullStateResult, TString>> UpdateConnectionsWithFullState(
        const TConnectionsFullState& connectionsFullState,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> ListConnections(
        const TString& puid,
        const TMaybe<TString>& deviceId,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

private:
    NThreading::TFuture<TExpected<void, TString>> UpdateConnectionsWithDiffRemoveDisconnected(
        const TEndpoint& endpoint,
        const TVector<TUserDeviceInfo>& disconnectedClients,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> UpdateConnectionsWithDiffAddConnected(
        const TEndpoint& endpoint,
        const TVector<TUserDeviceInfo>& connectedClients,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<ui64, TString>> UpdateConnectionsWithFullStateRemoveAll(
        const TEndpoint& endpoint,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<ui64, TString>> UpdateConnectionsWithFullStateRemoveDisconnected(
        const TConnectionsFullState& connectionsFullState,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<TUpdateConnectionsWithFullStateAddConnectedResult, TString>> UpdateConnectionsWithFullStateAddConnected(
        const TConnectionsFullState& connectionsFullState,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> ListConnectionsByPuid(
        const TString& puid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> ListConnectionsByPuidAndDeviceId(
        const TString& puid,
        const TString& deviceId,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    static TExpected<TConnectionsStorage::TListConnectionsResult, TString> ParseListConnectionsResult(
        const NYdb::TResultSet& resultSet,
        TOperationContext& operationContext
    );

private:
    static inline constexpr TStringBuf NAME = "connections";

    // Hard limit for the duration of the connection is 12 hours (more info in ZION-287)
    static inline constexpr TDuration CONNECTION_TTL = TDuration::Hours(12);
    static inline constexpr ui32 MAX_ADDED_USER_DEVICES_IN_FULL_STATE_UPDATE_RESULT = 1000;
};

} // namespace NMatrix::NNotificator
