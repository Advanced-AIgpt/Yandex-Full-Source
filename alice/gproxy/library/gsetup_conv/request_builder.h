#pragma once

#include "request_builder_impl.h"


namespace NGProxy {

class TRequestBuilder : public TRequestBuilderImpl {
protected:
    virtual void SetGrpcRequest(NJson::TJsonValue& output, const NGProxy::TMetadata& meta, const NGProxy::GSetupRequestInfo& info, const NGProxy::GSetupRequest& req) override;
};  // class TRequestBuilder

}   // namespace NGProxy
