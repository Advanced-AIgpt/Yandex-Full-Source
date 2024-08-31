#include "setup_add.h"
#include <alice/protos/data/tv/watch_list/wl_requests.pb.h>

namespace NAlice::NHollywood::NWatchList {
    TString TTvWatchListAddSetupHandle::Name() const {
        return "grpc/tv_watch_list/add/setup";
    }

    NAppHostHttp::THttpRequest_EMethod TTvWatchListAddSetupHandle::HttpMethod() const {
        return NAppHostHttp::THttpRequest_EMethod::THttpRequest_EMethod_Post;
    }

    TString TTvWatchListAddSetupHandle::ItemUuid(NAppHost::IServiceContext& serviceCtx) const {
        const auto& requestContainer = GetMaybeOnlyProto<google::protobuf::Any>(serviceCtx, "request");
        Y_ENSURE(requestContainer && requestContainer->Is<NAlice::NTv::TTvWatchListAddItemRequest>());
        NAlice::NTv::TTvWatchListAddItemRequest addRequest;
        requestContainer->UnpackTo(&addRequest);
        return addRequest.GetUuid();
    }
} // namespace NAlice::NHollywood::NWatchList
