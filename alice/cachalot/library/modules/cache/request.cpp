#include <alice/cachalot/library/modules/cache/request.h>
#include <alice/cachalot/library/modules/cache/storage.h>
#include <alice/cachalot/library/modules/cache/key_converter.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/utils.h>

#include <alice/cachalot/events/cachalot.ev.pb.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <util/random/random.h>


namespace NCachalot {

class TCacheStorageCallbacks : public IMegazordKvStorageCallbacks<TCacheStorage::TKey, TCacheStorage::TData> {
private:
    using TEmptyResponse = TCacheStorage::TEmptyResponse;
    using TSingleRowResponse = TCacheStorage::TSingleRowResponse;

public:
    TCacheStorageCallbacks(TRequestCache* request, TString localStorageTag, TString globalStorageTag)
        : RequestRef(request)
        , LocalStorageTag(std::move(localStorageTag))
        , GlobalStorageTag(std::move(globalStorageTag))
    {
    }

    void OnLocalSetResponse(const TEmptyResponse& rsp, TChroniclerPtr chronicler) override {
        OnSetResponse(rsp, chronicler, LocalStorageTag);
    }

    void OnGlobalSetResponse(const TEmptyResponse& rsp, TChroniclerPtr chronicler) override {
        OnSetResponse(rsp, chronicler, GlobalStorageTag);
    }

    void OnLocalGetSingleRowResponse(const TSingleRowResponse& rsp, TChroniclerPtr chronicler) override {
        OnGetSingleRowResponse(rsp, chronicler, LocalStorageTag);
    }

    void OnGlobalGetSingleRowResponse(const TSingleRowResponse& rsp, TChroniclerPtr chronicler) override {
        OnGetSingleRowResponse(rsp, chronicler, GlobalStorageTag);

        if (rsp.Status == EResponseStatus::OK) {
            RequestRef->AddBackgroundSetRequest(rsp.Key, rsp.Data.GetRef());
        }
    }

    void OnLocalDelResponse(const TEmptyResponse& rsp, TChroniclerPtr chronicler) override {
        OnDeleteResponse(rsp, chronicler, LocalStorageTag);
    }

    void OnGlobalDelResponse(const TEmptyResponse& rsp, TChroniclerPtr chronicler) override {
        OnDeleteResponse(rsp, chronicler, GlobalStorageTag);
    }

private:
    void OnGetSingleRowResponse(const TSingleRowResponse& rsp, TChroniclerPtr chronicler, TString tag) {
        Y_ASSERT(chronicler != nullptr);

        if (rsp.Status == EResponseStatus::OK) {
            chronicler->LogEvent(NEvClass::CacheHit(tag));
        } else if (rsp.Status == EResponseStatus::NO_CONTENT) {
            chronicler->LogEvent(NEvClass::CacheMiss(tag));
        } else {
            chronicler->LogEvent(NEvClass::CacheGetError(tag, rsp.Stats.ErrorMessage, rsp.Status));
        }
        RequestRef->AddBackendStats(tag, rsp.Status, rsp.Stats);
    }

    void OnSetResponse(const TEmptyResponse& rsp, TChroniclerPtr chronicler, TString tag) {
        Y_ASSERT(chronicler != nullptr);

        if (rsp.Status == EResponseStatus::OK || rsp.Status == EResponseStatus::CREATED) {
            chronicler->LogEvent(NEvClass::CacheSetOk(tag));
        } else {
            chronicler->LogEvent(NEvClass::CacheSetFail(tag, rsp.Stats.ErrorMessage, rsp.Status));
        }
        RequestRef->AddBackendStats(tag, rsp.Status, rsp.Stats);
    }

    void OnDeleteResponse(const TEmptyResponse& rsp, TChroniclerPtr chronicler, TString tag) {
        Y_ASSERT(chronicler != nullptr);

        if (rsp.Status == EResponseStatus::OK) {
            chronicler->LogEvent(NEvClass::CacheDelOk(tag));
        } else if (rsp.Status == EResponseStatus::NO_CONTENT) {
            chronicler->LogEvent(NEvClass::CacheDelNotFound(tag));
        } else {
            chronicler->LogEvent(NEvClass::CacheDelError(tag, rsp.Stats.ErrorMessage, rsp.Status));
        }
        RequestRef->AddBackendStats(tag, rsp.Status, rsp.Stats);
    }

private:
    // TRequestCache outlives any storage operations.
    TRequestCache* RequestRef;

    TString LocalStorageTag;
    TString GlobalStorageTag;
};

TRequestCache::TRequestCache(const NNeh::IRequestRef& req, TRequestCacheContextPtr context)
    : TRequest(req, &TMetrics::GetInstance().CacheServiceMetrics.Request)
    , Context(std::move(context))
{
}

TRequestCache::TRequestCache(NAppHost::TServiceContextPtr ahCtx, TRequestCacheContextPtr context)
    : TRequest(std::move(ahCtx), &TMetrics::GetInstance().CacheServiceMetrics.Request)
    , Context(std::move(context))
{
}

TAsyncStatus TRequestCache::ServeAsync() {
    RequestMetrics->OnServeStarted(ArrivalTime);
    Response.MutableStats()->SetWaitingTime(MillisecondsSince(ArrivalTime));
    const NCachalotProtocol::TCacheRequestOptions& opts = [&](){
        if (Request.HasGetReq()) {
            return Request.GetGetReq().GetOptions();
        }
        if (Request.HasSetReq()) {
            return Request.GetSetReq().GetOptions();
        }
        if (Request.HasDeleteReq()) {
            return Request.GetDeleteReq().GetOptions();
        }
        return NCachalotProtocol::TCacheRequestOptions();
    }();

    const TIntrusivePtr<TCacheStorage> storage = MakeStorageOrSetError(opts);

    if (storage == nullptr) {
        return NThreading::MakeFuture(TStatus(Status));
    }

    auto self = IntrusiveThis();
    NThreading::TPromise<TStatus> status = NThreading::NewPromise<TStatus>();

    if (Request.HasGetReq()) {
        const NCachalotProtocol::TGetRequest& getReq = Request.GetGetReq();
        LogFrame->LogEvent(getReq);

        ::NCachalotProtocol::TGetResponse *ptr = Response.MutableGetResp();
        ptr->SetKey(getReq.GetKey());

        storage->GetSingleRow(getReq.GetKey(), LogFrame)
            .Subscribe([this, self, status, storage, ptr](const auto& reply) mutable {
            const TSimpleSingleRowResponse& r = reply.GetValueSync();
            AddBackendStats(storage->GetStorageTag(), r.Status, r.Stats);
            SetStatus(r.Status);

            if (r && r.Status == EResponseStatus::OK) {
                const TString& data = r.Data.GetRef();
                Context->ServiceMetrics->OnCacheHit(ArrivalTime);
                Context->ServiceMetrics->LoadedSize(data.size());
                ptr->SetData(data);
            } else {
                Context->ServiceMetrics->OnCacheMiss(ArrivalTime);
                ptr->SetError(r.Stats.ErrorMessage);
            }
            status.SetValue(r);
        });

    } else if (Request.HasSetReq()) {
        const NCachalotProtocol::TSetRequest& setReq = Request.GetSetReq();
        {
            // TODO (paxakor): make it right when TLogContext supports field options
            NCachalotProtocol::TSetRequest logRecord;
            logRecord.SetKey(setReq.GetKey());
            logRecord.SetData(TStringBuilder() << "DataSize: " << setReq.GetData().size());
            logRecord.SetTTL(setReq.GetTTL());
            logRecord.SetStorageTag(setReq.GetStorageTag());
            logRecord.MutableOptions()->CopyFrom(setReq.GetOptions());
            LogFrame->LogEvent(logRecord);
        }

        Context->ServiceMetrics->StoredSize(setReq.GetData().size());

        ::NCachalotProtocol::TSetResponse* ptr = Response.MutableSetResp();
        ptr->SetKey(setReq.GetKey());

        storage->Set(setReq.GetKey(), setReq.GetData(), setReq.GetTTL(), LogFrame)
            .Subscribe([this, self, status, storage, ptr](const auto& reply) mutable {
            const TSimpleEmptyResponse& r = reply.GetValueSync();

            AddBackendStats(storage->GetStorageTag(), r.Status, r.Stats);
            SetStatus(r.Status);

            if (!r) {
                ptr->SetError(r.Stats.ErrorMessage);
            }

            status.SetValue(r);
        });
    } else if (Request.HasDeleteReq()) {
        const NCachalotProtocol::TDeleteRequest& deleteReq = Request.GetDeleteReq();
        LogFrame->LogEvent(deleteReq);

        ::NCachalotProtocol::TDeleteResponse* ptr = Response.MutableDeleteResp();
        ptr->SetKey(deleteReq.GetKey());

        storage->Del(deleteReq.GetKey(), LogFrame).Subscribe(
            [this, self, status, storage, ptr](const auto& reply) mutable {
                const TSimpleEmptyResponse& r = reply.GetValueSync();

                AddBackendStats(storage->GetStorageTag(), r.Status, r.Stats);
                SetStatus(r.Status);

                if (!r) {
                    ptr->SetError(r.Stats.ErrorMessage);
                }

                status.SetValue(r);
            }
        );
    }

    return status;
}

void TRequestCache::AddBackgroundSetRequest(TString key, TString data) {
    if (!Context->Settings.EnableInterCacheUpdates) {
        return;
    }

    if (LocalStorage == nullptr) {
        return;
    }

    const uint64_t ttl = ChooseTtl(
        LocalStorage->MutableConfig()->DefaultTtlSeconds.GetOrElse(
            Context->Settings.TtlSecondsForLocalStorage
        )
    );

    const bool added = Context->BackgroundThreadPool.AddFunc(
        [key=std::move(key), data=std::move(data), ttl, storage=LocalStorage] () mutable {
            storage->Set(key, data, ttl);
            TMetrics::GetInstance().CacheServiceMetrics.OnBackgroundTaskCompleted();
        }
    );

    if (added) {
        TMetrics::GetInstance().CacheServiceMetrics.OnBackgroundTaskAddOk();
    } else {
        // just ignore if queue is full
        TMetrics::GetInstance().CacheServiceMetrics.OnBackgroundTaskAddFail();
    }
}

uint64_t TRequestCache::ChooseTtl(uint64_t defaultTtl) const {
    if (Request.HasSetReq() && Request.GetSetReq().HasTTL()) {
        const uint64_t ttl = Request.GetSetReq().GetTTL();
        LogFrame->LogEvent(NEvClass::CacheManualTtl(ReqId, ttl));
        return ttl;
    }

    return defaultTtl + RandomNumber(Context->Settings.AllowedTtlVariationSeconds);
}

TIntrusivePtr<TCacheStorage> TRequestCache::MakeStorageOrSetError(
    const NCachalotProtocol::TCacheRequestOptions& opts
) {
    Y_ASSERT(Context != nullptr);

    TCacheStorage::TStoragePtr globalStorage = Context->Storage.Ydb;
    LocalStorage = Context->Storage.Imdb ? Context->Storage.Imdb : Context->Storage.Redis;

    if ((opts.GetForceInternalStorage() || opts.GetForceRedisStorage()) && opts.GetForceYdbStorage()) {
        SetError(EResponseStatus::BAD_REQUEST, "Multiple ForceStorage options cannot be set simultaneously");
        return nullptr;
    } else if (opts.GetForceInternalStorage() || opts.GetForceRedisStorage()) {
        if (!Context->Storage.Imdb) {
            SetError(EResponseStatus::BAD_REQUEST, "Imdb disabled");
            return nullptr;
        }
        globalStorage = nullptr;
    } else if (opts.GetForceYdbStorage()) {
        if (!Context->Storage.Ydb) {
            SetError(EResponseStatus::BAD_REQUEST, "Ydb disabled");
            return nullptr;
        }
        LocalStorage = nullptr;
    }

    return MakeIntrusive<TCacheStorage>(
        LocalStorage,
        std::move(globalStorage),
        ChooseTtl(Context->Settings.TtlSecondsForLocalStorage),  // TODO (paxakor): ttl for redis is ignored in imdb.
        ChooseTtl(Context->Settings.TtlSecondsForYdb),
        MakeIntrusive<TCacheStorageCallbacks>(
            this,
            LocalStorage ? LocalStorage->GetStorageTag() : "fake",
            globalStorage ? globalStorage->GetStorageTag() : "fake"
        )
    );
}


TRequestCacheGet::TRequestCacheGet(
    TAtomicSharedPtr<NCachalotProtocol::TGetRequest> protoReq,
    NAppHost::TServiceContextPtr ahCtx,
    TRequestCacheContextPtr context
)
    : TRequestCache(std::move(ahCtx), context)
{
    if (protoReq) {
        protoReq->Swap(Request.MutableGetReq());
    } else {
        SetError(EResponseStatus::BAD_REQUEST, "Bad Request (Empty Protobuf)");
        RequestMetrics->OnBadData(ArrivalTime);
    }
}

TRequestCacheSet::TRequestCacheSet(
    TAtomicSharedPtr<NCachalotProtocol::TSetRequest> protoReq,
    NAppHost::TServiceContextPtr ahCtx,
    TRequestCacheContextPtr context
)
    : TRequestCache(std::move(ahCtx), context)
{
    if (protoReq) {
        protoReq->Swap(Request.MutableSetReq());
    } else {
        SetError(EResponseStatus::BAD_REQUEST, "Bad Request (Empty Protobuf)");
        RequestMetrics->OnBadData(ArrivalTime);
    }
}

TRequestCacheDelete::TRequestCacheDelete(
    TAtomicSharedPtr<NCachalotProtocol::TDeleteRequest> protoReq,
    NAppHost::TServiceContextPtr ahCtx,
    TRequestCacheContextPtr context
)
    : TRequestCache(std::move(ahCtx), context)
{
    if (protoReq) {
        protoReq->Swap(Request.MutableDeleteReq());
    } else {
        SetError(EResponseStatus::BAD_REQUEST, "Bad Request (Empty Protobuf)");
        RequestMetrics->OnBadData(ArrivalTime);
    }
}

void TRequestCache::ReplyToApphostContextOnSuccess(NAppHost::TServiceContextPtr ctx, TStringBuf requestKey) {
    ctx->AddFlag(MakeFlagFromRequestKey(requestKey, Status));
    ctx->AddProtobufItem(Response, MakeResponseKeyFromRequest(requestKey));
}

void TRequestCache::ReplyToApphostContextOnError(NAppHost::TServiceContextPtr ctx, TStringBuf requestKey) {
    ctx->AddFlag(MakeFlagFromRequestKey(requestKey, Status != EResponseStatus::OK ? Status : EResponseStatus::INTERNAL_ERROR));
    ctx->AddProtobufItem(Response, MakeResponseKeyFromRequest(requestKey));
}

}   // namespace NCachalot
