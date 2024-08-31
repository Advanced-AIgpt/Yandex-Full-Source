#pragma once

#include <alice/cachalot/library/modules/cache/storage.h>
#include <alice/cachalot/library/modules/cache/storage_tag_to_context.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/request.h>


namespace NCachalot {

class TRequestCache : public TRequest {
public:
    TRequestCache(const NNeh::IRequestRef& req, TRequestCacheContextPtr context);
    TRequestCache(NAppHost::TServiceContextPtr ahCtx, TRequestCacheContextPtr context);

    TAsyncStatus ServeAsync() override;

    void AddBackgroundSetRequest(TString key, TString data);

protected:
    void ReplyToApphostContextOnSuccess(NAppHost::TServiceContextPtr ctx, TStringBuf requestKey) override;
    void ReplyToApphostContextOnError(NAppHost::TServiceContextPtr ctx, TStringBuf requestKey) override;

private:
    TIntrusivePtr<TRequestCache> IntrusiveThis() {
        return this;
    }

    uint64_t ChooseTtl(uint64_t defaultTtl) const;

    TIntrusivePtr<TCacheStorage> MakeStorageOrSetError(const NCachalotProtocol::TCacheRequestOptions&);

private:
    TRequestCacheContextPtr Context;

    // We need to know concrete local storage for inter cache updates.
    TIntrusivePtr<TSimpleKvStorage> LocalStorage = nullptr;
};


class TRequestCacheGet : public TRequestCache {
public:
    TRequestCacheGet(
        TAtomicSharedPtr<NCachalotProtocol::TGetRequest> protoReq,
        NAppHost::TServiceContextPtr ahCtx,
        TRequestCacheContextPtr context
    );
};


class TRequestCacheSet : public TRequestCache {
public:
    TRequestCacheSet(
        TAtomicSharedPtr<NCachalotProtocol::TSetRequest> protoReq,
        NAppHost::TServiceContextPtr ahCtx,
        TRequestCacheContextPtr context
    );
};


class TRequestCacheDelete : public TRequestCache {
public:
    TRequestCacheDelete(
        TAtomicSharedPtr<NCachalotProtocol::TDeleteRequest> protoReq,
        NAppHost::TServiceContextPtr ahCtx,
        TRequestCacheContextPtr context
    );
};

}   // namespace NCachalot
