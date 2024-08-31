#include "setup_delete.h"
#include <alice/protos/data/tv/watch_list/wl_requests.pb.h>

namespace NAlice::NHollywood::NWatchList {
    TString TTvWatchListDeleteSetupHandle::Name() const {
        return "grpc/tv_watch_list/delete/setup";
    }

    NAppHostHttp::THttpRequest_EMethod TTvWatchListDeleteSetupHandle::HttpMethod() const {
        return NAppHostHttp::THttpRequest_EMethod::THttpRequest_EMethod_Delete;
    }

    TString TTvWatchListDeleteSetupHandle::ItemUuid(NAppHost::IServiceContext& serviceCtx) const {
        const auto& requestContainer = GetMaybeOnlyProto<google::protobuf::Any>(serviceCtx, "request");
        Y_ENSURE(requestContainer && requestContainer->Is<NAlice::NTv::TTvWatchListDeleteItemRequest>());
        NAlice::NTv::TTvWatchListDeleteItemRequest delRequest;
        requestContainer->UnpackTo(&delRequest);
        return delRequest.GetUuid();
    }
} // namespace NAlice::NHollywood::NWatchList
