#include "cache_prepare_set.h"
#include "common.h"

#include <alice/hollywood/library/scenarios/music/proto/cache_data.pb.h>

#include <alice/library/cachalot_cache/cachalot_cache.h>

namespace NAlice::NHollywood::NMusic::NCache {

namespace {

class TImpl {
public:
    TImpl(THwServiceContext& ctx)
        : Ctx_{ctx}
        , InputCacheMeta_{ctx.GetMaybeProto<TInputCacheMeta>(INPUT_CACHE_META_ITEM)}
        , OutputCacheMeta_{ctx.GetMaybeProto<TOutputCacheMeta>(OUTPUT_CACHE_META_ITEM)}
        , Request_{FindHttpRequestInfo(ctx)}
    {}

    void Do() {
        if (!InputCacheMeta_ || !InputCacheMeta_->GetUseCache()) {
            return;
        }

        if (!OutputCacheMeta_ || OutputCacheMeta_->GetCacheHit()) {
            return;
        }

        // send put request to cache server
        const size_t hash = CalculateHash(*InputCacheMeta_, Request_);
        LOG_INFO(Ctx_.Logger()) << "Add cache set-request with hash " << hash;

        const NAppHostHttp::THttpResponse response = Ctx_.GetProtoOrThrow<NAppHostHttp::THttpResponse>(Request_.OutputItemName);
        NCachalotProtocol::TSetRequest cacheSetRequest =
            NAppHostServices::TCachalotCache::MakeSetRequest(ToString(hash), response.GetContent(), CACHE_STORAGE_TAG);

        Ctx_.AddProtobufItemToApphostContext(cacheSetRequest, CACHE_SET_REQUEST_ITEM);
        Ctx_.AddBalancingHint(CACHE_LOOKUP_NODE_NAME, hash);
    }

private:
    THwServiceContext& Ctx_;
    const TMaybe<TInputCacheMeta> InputCacheMeta_;
    const TMaybe<TOutputCacheMeta> OutputCacheMeta_;
    const THttpRequestInfo Request_;
};

} // namespace

const TString& TCachePrepareSetHandle::Name() const {
    const static TString name = "music/cache_prepare_set";
    return name;
}

void TCachePrepareSetHandle::Do(THwServiceContext& ctx) const {
    TImpl{ctx}.Do();
}

} // namespace NAlice::NHollywood::NMusic::NCache
