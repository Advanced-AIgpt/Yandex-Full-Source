#include "cache_prepare_get.h"
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
        , Request_{FindHttpRequestInfo(ctx)}
    {}

    void Do() {
        if (InputCacheMeta_ && InputCacheMeta_->GetUseCache()) {
            // send lookup request to cache server
            const size_t hash = CalculateHash(*InputCacheMeta_, Request_);
            LOG_INFO(Ctx_.Logger()) << "Add cache get-request with hash " << hash;
            NCachalotProtocol::TGetRequest cacheGetRequest =
                NAppHostServices::TCachalotCache::MakeGetRequest(ToString(hash), CACHE_STORAGE_TAG);
            Ctx_.AddProtobufItemToApphostContext(cacheGetRequest, CACHE_GET_REQUEST_ITEM);
            Ctx_.AddBalancingHint(CACHE_LOOKUP_NODE_NAME, hash);
        } else {
            // shouldn't use cache, fallback to common http proxy request
            Ctx_.AddProtobufItemToApphostContext(Request_.HttpRequest, Request_.InputItemName);
        }
    }

private:
    THwServiceContext& Ctx_;
    const TMaybe<TInputCacheMeta> InputCacheMeta_;
    const THttpRequestInfo Request_;
};

} // namespace

const TString& TCachePrepareGetHandle::Name() const {
    const static TString name = "music/cache_prepare_get";
    return name;
}

void TCachePrepareGetHandle::Do(THwServiceContext& ctx) const {
    TImpl{ctx}.Do();
}

} // namespace NAlice::NHollywood::NMusic::NCache
