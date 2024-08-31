#include "cache_process_get.h"
#include "common.h"

#include <alice/hollywood/library/scenarios/music/proto/cache_data.pb.h>

#include <alice/cachalot/api/protos/cachalot.pb.h>

namespace NAlice::NHollywood::NMusic::NCache {

namespace {

class TImpl {
public:
    TImpl(THwServiceContext& ctx)
        : Ctx_{ctx}
        , CacheResponse_{Ctx_.GetProtoOrThrow<NCachalotProtocol::TResponse>(CACHE_GET_RESPONSE_ITEM)}
        , Request_{FindHttpRequestInfo(ctx)}
    {}

    void Do() {
        LOG_INFO(Ctx_.Logger()) << "Cachalot response status: " << EResponseStatus_Name(CacheResponse_.GetStatus());

        TOutputCacheMeta outputCacheMeta;
        if (CacheResponse_.GetStatus() == NCachalotProtocol::OK) {
            // CACHE HIT, return cached response
            outputCacheMeta.SetCacheHit(true);
            NAppHostHttp::THttpResponse httpResponse;
            httpResponse.SetStatusCode(200);
            httpResponse.SetContent(CacheResponse_.GetGetResp().GetData());
            Ctx_.AddProtobufItemToApphostContext(httpResponse, Request_.OutputItemName);
        } else {
            // CACHE MISS, prepare request
            outputCacheMeta.SetCacheHit(false);
            Ctx_.AddProtobufItemToApphostContext(Request_.HttpRequest, Request_.InputItemName);
        }
        Ctx_.AddProtobufItemToApphostContext(outputCacheMeta, OUTPUT_CACHE_META_ITEM);
    }

private:
    THwServiceContext& Ctx_;
    const NCachalotProtocol::TResponse CacheResponse_;
    const THttpRequestInfo Request_;
};

} // namespace

const TString& TCacheProcessGetHandle::Name() const {
    const static TString name = "music/cache_process_get";
    return name;
}

void TCacheProcessGetHandle::Do(THwServiceContext& ctx) const {
    TImpl{ctx}.Do();
}

} // namespace NAlice::NHollywood::NMusic::NCache
