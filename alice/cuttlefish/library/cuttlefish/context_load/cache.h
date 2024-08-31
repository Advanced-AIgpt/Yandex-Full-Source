#pragma once

#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <alice/megamind/protos/common/iot.pb.h>

#include <alice/library/cachalot_cache/cachalot_cache.h>

#include <apphost/lib/proto_answers/http.pb.h>
#include <apphost/api/service/cpp/service.h>

#include <util/generic/strbuf.h>


namespace NAlice::NCuttlefish::NAppHostServices {

    namespace NPrivate {

        struct TCacheStorageConfig {
            const TString StorageTag;
            const TStringBuf ClearRequestItemType;
            const TStringBuf LoadRequestItemType;
            const TStringBuf LoadResponseItemType;
            const TStringBuf StoreRequestItemType;
            const TStringBuf SensorName;
        };

        class TCacheImpl {
        public:
            TCacheImpl(
                TString key,
                const TCacheStorageConfig& config,
                NAppHost::IServiceContext& ahContext,
                NAlice::NCuttlefish::TLogContext logContext,
                TSourceMetrics& metrics
            );

            void Load() const;
            void Store(TString data) const;
            void Store(TString data, uint64_t ttlSeconds) const;
            void Clear() const;
            TMaybe<TString> GetLoadedData() const;

        private:
            void Store(NCachalotProtocol::TSetRequest request) const;

        private:
            const TString Key;
            const TCacheStorageConfig& Config;
            NAppHost::IServiceContext& AhContext;
            NAlice::NCuttlefish::TLogContext LogContext;
            TSourceMetrics& Metrics;
        };

    }  // namespace NPrivate


    class TDatasyncCache {
    public:
        TDatasyncCache(
            const TString& authToken,
            NAppHost::IServiceContext& ahContext,
            NAlice::NCuttlefish::TLogContext logContext,
            TSourceMetrics& metrics
        );

        void Load() const;
        void Store(const NAppHostHttp::THttpResponse& cacheItem) const;
        void Clear() const;
        TMaybe<NAppHostHttp::THttpResponse> TryParseLoadedData() const;

        static bool IsEnabled(const NAliceProtocol::TRequestContext* requestCtx);

    private:
        static const NPrivate::TCacheStorageConfig Config;
        NPrivate::TCacheImpl Impl;
    };


    class TMementoCache {
    public:
        TMementoCache(
            const TString& authToken,
            NAppHost::IServiceContext& ahContext,
            NAlice::NCuttlefish::TLogContext logContext,
            TSourceMetrics& metrics
        );

        void Load() const;
        void Store(const NAppHostHttp::THttpResponse& cacheItem) const;
        TMaybe<NAppHostHttp::THttpResponse> TryParseLoadedData() const;

        static bool IsEnabled(const NAliceProtocol::TRequestContext* requestCtx);

    private:
        static const NPrivate::TCacheStorageConfig Config;
        NPrivate::TCacheImpl Impl;
    };


    class TIoTUserInfoCache {
    public:
        TIoTUserInfoCache(
            const TString& authToken,
            NAppHost::IServiceContext& ahContext,
            NAlice::NCuttlefish::TLogContext logContext,
            TSourceMetrics& metrics
        );

        void Load() const;
        void Store(const NAlice::TIoTUserInfo& cacheItem) const;
        TMaybe<NAlice::TIoTUserInfo> TryParseLoadedData() const;

        static bool IsEnabled(const NAliceProtocol::TRequestContext* requestCtx);

    private:
        static const NPrivate::TCacheStorageConfig Config;
        NPrivate::TCacheImpl Impl;
    };


    bool HasAuthToken(const NAliceProtocol::TSessionContext* sessionCtx);

}  // namespace NAlice::NCuttlefish::NAppHostServices
