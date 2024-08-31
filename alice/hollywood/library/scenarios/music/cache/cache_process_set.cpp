#include "cache_process_set.h"
#include "common.h"

#include <alice/cachalot/api/protos/cachalot.pb.h>

namespace NAlice::NHollywood::NMusic::NCache {

namespace {

class TImpl {
public:
    TImpl(THwServiceContext& ctx)
        : Ctx_{ctx}
        , CacheResponse_{Ctx_.GetProtoOrThrow<NCachalotProtocol::TResponse>(CACHE_SET_RESPONSE_ITEM)}
    {}

    void Do() {
        TStringBuilder sb;
        sb << "Cachalot response status: " << EResponseStatus_Name(CacheResponse_.GetStatus());
        if (CacheResponse_.GetStatus() == NCachalotProtocol::OK) {
            sb << " (key " << CacheResponse_.GetSetResp().GetKey() << ")";
        }
        LOG_INFO(Ctx_.Logger()) << sb;
    }

private:
    THwServiceContext& Ctx_;
    const NCachalotProtocol::TResponse CacheResponse_;
};

} // namespace

const TString& TCacheProcessSetHandle::Name() const {
    const static TString name = "music/cache_process_set";
    return name;
}

void TCacheProcessSetHandle::Do(THwServiceContext& ctx) const {
    TImpl{ctx}.Do();
}

} // namespace NAlice::NHollywood::NMusic::NCache
