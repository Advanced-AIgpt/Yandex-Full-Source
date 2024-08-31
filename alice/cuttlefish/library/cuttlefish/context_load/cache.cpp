#include "cache.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <voicetech/library/settings_manager/proto/settings.pb.h>
#include <library/cpp/openssl/crypto/sha.h>
#include <util/digest/city.h>
#include <util/string/hex.h>

using namespace NAlice::NCuttlefish;
using namespace NAlice::NAppHostServices;

namespace {

    constexpr TStringBuf LOAD_NODE_NAME = "CACHALOT_CONTEXT_CACHE";

    constexpr TStringBuf STORE_NODE_NAME_SAS = "CACHALOT_CONTEXT_CACHE_UPDATE_SAS";
    constexpr TStringBuf STORE_NODE_NAME_VLA = "CACHALOT_CONTEXT_CACHE_UPDATE_VLA";
    constexpr TStringBuf STORE_NODE_NAME_MAN = "CACHALOT_CONTEXT_CACHE_UPDATE_MAN";

    constexpr TStringBuf CLEAR_NODE_NAME_SAS = "INVALIDATE_CACHALOT_CONTEXT_CACHE_SAS";
    constexpr TStringBuf CLEAR_NODE_NAME_VLA = "INVALIDATE_CACHALOT_CONTEXT_CACHE_VLA";
    constexpr TStringBuf CLEAR_NODE_NAME_MAN = "INVALIDATE_CACHALOT_CONTEXT_CACHE_MAN";

    TString MakeCacheKey(const TString& authToken) {
        NOpenSsl::NSha256::TDigest hash = NOpenSsl::NSha256::Calc(authToken.data(), authToken.size());
        return HexEncode(hash.data(), hash.size());
    }
}

namespace NAlice::NCuttlefish::NAppHostServices::NPrivate {

    TCacheImpl::TCacheImpl(
        TString key,
        const TCacheStorageConfig& config,
        NAppHost::IServiceContext& ahContext,
        NAlice::NCuttlefish::TLogContext logContext,
        TSourceMetrics& metrics
    )
        : Key(std::move(key))
        , Config(config)
        , AhContext(ahContext)
        , LogContext(std::move(logContext))
        , Metrics(metrics)
    {
        Y_ASSERT(Key);
        Y_ASSERT(Config.StorageTag);
        Y_ASSERT(Config.ClearRequestItemType);
        Y_ASSERT(Config.LoadRequestItemType);
        Y_ASSERT(Config.LoadResponseItemType);
        Y_ASSERT(Config.StoreRequestItemType);
        Y_ASSERT(Config.SensorName);
    }

    void TCacheImpl::Load() const {
        NCachalotProtocol::TGetRequest request = TCachalotCache::MakeGetRequest(Key, Config.StorageTag);

        AhContext.AddProtobufItem(request, Config.LoadRequestItemType);
        AhContext.AddBalancingHint(LOAD_NODE_NAME, CityHash64(request.GetKey()));

        // TODO (paxakor): rename logged events
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostDatasyncCacheGetRequest>(request.ShortUtf8DebugString());
        Metrics.PushRate("cache_request", "load", Config.SensorName);
    }

    void TCacheImpl::Store(TString data) const {
        Store(TCachalotCache::MakeSetRequest(Key, std::move(data), Config.StorageTag));
    }

    void TCacheImpl::Store(TString data, uint64_t ttlSeconds) const {
        Store(TCachalotCache::MakeSetRequest(Key, std::move(data), Config.StorageTag, ttlSeconds));
    }

    void TCacheImpl::Store(NCachalotProtocol::TSetRequest request) const {
        AhContext.AddProtobufItem(request, Config.StoreRequestItemType);

        // Cache data is stored across 3 datacenters
        auto hash = CityHash64(Key);
        AhContext.AddBalancingHint(STORE_NODE_NAME_SAS, hash);
        AhContext.AddBalancingHint(STORE_NODE_NAME_VLA, hash);
        AhContext.AddBalancingHint(STORE_NODE_NAME_MAN, hash);

        request.ClearData();  // we do not need a lot of binary data in our logs.
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostDatasyncCacheSetRequest>(request.ShortUtf8DebugString());
        Metrics.PushRate("cache_request", "store", Config.SensorName);
    }

    void TCacheImpl::Clear() const {
        NCachalotProtocol::TDeleteRequest request = TCachalotCache::MakeDeleteRequest(Key, Config.StorageTag);

        AhContext.AddProtobufItem(request, Config.ClearRequestItemType);

        // Cache data is stored across 3 datacenters
        auto hash = CityHash64(Key);
        AhContext.AddBalancingHint(CLEAR_NODE_NAME_SAS, hash);
        AhContext.AddBalancingHint(CLEAR_NODE_NAME_VLA, hash);
        AhContext.AddBalancingHint(CLEAR_NODE_NAME_MAN, hash);

        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostDatasyncCacheDelRequest>(request.ShortUtf8DebugString());
        Metrics.PushRate("cache_request", "delete", Config.SensorName);
    }

    TMaybe<TString> TCacheImpl::GetLoadedData() const {
        if (AhContext.HasProtobufItem(Config.LoadResponseItemType)) {
            NCachalotProtocol::TResponse cacheResponse = AhContext.GetOnlyProtobufItem<NCachalotProtocol::TResponse>(
                Config.LoadResponseItemType
            );
            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostDatasyncCacheGetResponse>(
                cacheResponse.ShortUtf8DebugString()
            );

            if (cacheResponse.GetStatus() == NCachalotProtocol::OK) {
                Metrics.PushRate("response", "hit", Config.SensorName);
                return cacheResponse.GetGetResp().GetData();
            } else if (cacheResponse.GetStatus() == NCachalotProtocol::NO_CONTENT) { // TODO @aradzevich: Cachalot should return NOT_FOUND on cache miss, but not NO_CONTENT
                Metrics.PushRate("response", "miss", Config.SensorName);
            } else if (cacheResponse.GetStatus() == NCachalotProtocol::NOT_FOUND) {
                Metrics.PushRate("response", "miss", Config.SensorName);
            } else {
                Metrics.PushRate("response", "error", Config.SensorName);
            }
        }
        if (AhContext.HasProtobufItem(Config.LoadRequestItemType)) {
            Metrics.PushRate("response", "noans", Config.SensorName);
        }
        return Nothing();
    }

    TMaybe<NAppHostHttp::THttpResponse> TryParseHttpResponse(TMaybe<TString> data) {
        if (data.Defined()) {
            NAppHostHttp::THttpResponse response;
            response.SetContent(std::move(data.GetRef()));
            response.SetStatusCode(HttpCodes::HTTP_OK);
            return response;
        }
        return Nothing();
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices::NPrivate


namespace NAlice::NCuttlefish::NAppHostServices {

    TDatasyncCache::TDatasyncCache(
        const TString& authToken,
        NAppHost::IServiceContext& ahContext,
        TLogContext logContext,
        TSourceMetrics& metrics
    )
        : Impl(
            MakeCacheKey(authToken),
            Config,
            ahContext,
            logContext,
            metrics
        )
    {
    }

    void TDatasyncCache::Load() const {
        Impl.Load();
    }

    void TDatasyncCache::Store(const NAppHostHttp::THttpResponse& cacheItem) const {
        Impl.Store(cacheItem.GetContent());
    }

    void TDatasyncCache::Clear() const {
        Impl.Clear();
    }

    TMaybe<NAppHostHttp::THttpResponse> TDatasyncCache::TryParseLoadedData() const {
        return NPrivate::TryParseHttpResponse(Impl.GetLoadedData());
    }

    bool TDatasyncCache::IsEnabled(const NAliceProtocol::TRequestContext* requestCtx) {
        return requestCtx
            && requestCtx->HasSettingsFromManager()
            && requestCtx->GetSettingsFromManager().GetUseDatasyncCache();
    }

    const NPrivate::TCacheStorageConfig TDatasyncCache::Config{
        .StorageTag = "Datasync",
        .ClearRequestItemType = ITEM_TYPE_DATASYNC_CACHE_DELETE_REQUEST,
        .LoadRequestItemType = ITEM_TYPE_DATASYNC_CACHE_GET_REQUEST,
        .LoadResponseItemType = ITEM_TYPE_DATASYNC_CACHE_GET_RESPONSE,
        .StoreRequestItemType = ITEM_TYPE_DATASYNC_CACHE_SET_REQUEST,
        .SensorName = "datasync-cache",
    };


    TMementoCache::TMementoCache(
        const TString& authToken,
        NAppHost::IServiceContext& ahContext,
        TLogContext logContext,
        TSourceMetrics& metrics
    )
        : Impl(
            MakeCacheKey(authToken),
            Config,
            ahContext,
            logContext,
            metrics
        )
    {
    }

    void TMementoCache::Load() const {
        Impl.Load();
    }

    void TMementoCache::Store(const NAppHostHttp::THttpResponse& cacheItem) const {
        Impl.Store(cacheItem.GetContent());
    }

    TMaybe<NAppHostHttp::THttpResponse> TMementoCache::TryParseLoadedData() const {
        return NPrivate::TryParseHttpResponse(Impl.GetLoadedData());
    }

    bool TMementoCache::IsEnabled(const NAliceProtocol::TRequestContext* requestCtx) {
        return requestCtx
            && requestCtx->HasSettingsFromManager()
            && requestCtx->GetSettingsFromManager().GetUseMementoCache();
    }

    const NPrivate::TCacheStorageConfig TMementoCache::Config{
        .StorageTag = "Memento",
        .ClearRequestItemType = "not_implemented",
        .LoadRequestItemType = ITEM_TYPE_MEMENTO_CACHE_GET_REQUEST,
        .LoadResponseItemType = ITEM_TYPE_MEMENTO_CACHE_GET_RESPONSE,
        .StoreRequestItemType = ITEM_TYPE_MEMENTO_CACHE_SET_REQUEST,
        .SensorName = "memento-cache",
    };


    TIoTUserInfoCache::TIoTUserInfoCache(
        const TString& authToken,
        NAppHost::IServiceContext& ahContext,
        TLogContext logContext,
        TSourceMetrics& metrics
    )
        : Impl(
            MakeCacheKey(authToken),
            Config,
            ahContext,
            logContext,
            metrics
        )
    {
    }

    void TIoTUserInfoCache::Load() const {
        Impl.Load();
    }

    void TIoTUserInfoCache::Store(const NAlice::TIoTUserInfo& cacheItem) const {
        Impl.Store(cacheItem.SerializeAsString());
    }

    TMaybe<NAlice::TIoTUserInfo> TIoTUserInfoCache::TryParseLoadedData() const {
        if (TMaybe<TString> data = Impl.GetLoadedData()) {
            if (NAlice::TIoTUserInfo response; response.ParseFromString(data.GetRef())) {
                return response;
            }
        }
        return Nothing();
    }

    bool TIoTUserInfoCache::IsEnabled(const NAliceProtocol::TRequestContext* requestCtx) {
        return requestCtx
            && requestCtx->HasSettingsFromManager()
            && requestCtx->GetSettingsFromManager().GetUseIoTCache();
    }

    const NPrivate::TCacheStorageConfig TIoTUserInfoCache::Config{
        .StorageTag = "IoT",
        .ClearRequestItemType = "not_implemented",
        .LoadRequestItemType = ITEM_TYPE_QUASARIOT_CACHE_GET_REQUEST,
        .LoadResponseItemType = ITEM_TYPE_QUASARIOT_CACHE_GET_RESPONSE,
        .StoreRequestItemType = ITEM_TYPE_QUASARIOT_CACHE_SET_REQUEST,
        .SensorName = "iot-cache",
    };


    bool HasAuthToken(const NAliceProtocol::TSessionContext* sessionCtx) {
        return sessionCtx
            && sessionCtx->HasUserInfo()
            && sessionCtx->GetUserInfo().HasAuthToken();
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
