#include "response_builder_impl.h"


namespace NGProxy {


TResponseBuilderImpl::TResponseBuilderImpl()
{
}

void TResponseBuilderImpl::SetResponse(NJson::TJsonValue content) {
    Response = std::move(content);
}

void TResponseBuilderImpl::SetRequestInfo(const NGProxy::TMetadata& meta, const NGProxy::GSetupRequestInfo& info) {
    Metadata = meta;
    RequestInfo = info;
}

NGProxy::GSetupResponse TResponseBuilderImpl::Build() const {
    NGProxy::GSetupResponse tmp;

    SetGrpcResponse(Response, Metadata, RequestInfo, tmp);

    return tmp;
}


}   // namespace NGProxy
