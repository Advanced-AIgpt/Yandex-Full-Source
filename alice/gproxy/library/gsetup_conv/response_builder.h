#pragma once

#include "response_builder_impl.h"
#include <alice/cuttlefish/library/logging/event_log.h>


namespace NGProxy {

class TResponseBuilder : public TResponseBuilderImpl {
protected:
    /**
     *  @brief extract grpc response from json megamind response
     *  @param[in]  input   megamind response
     *  @param[in]  meta    grpc original request metadata
     *  @param[in]  info    grpc original request information
     *  @param[out] resp    extracted grpc response
     */
    virtual void SetGrpcResponse(const NJson::TJsonValue& input, const NGProxy::TMetadata& meta, const NGProxy::GSetupRequestInfo& info, NGProxy::GSetupResponse& resp) const override;
};  // class TRequestBuilder

}   // namespace NGProxy
