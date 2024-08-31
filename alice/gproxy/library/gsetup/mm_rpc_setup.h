#pragma once

#include "handler.h"

namespace NGProxy {

class TGSetupMMRpc : public TBasicHandler<TGSetupMMRpc> {
public:
    static constexpr const char *Path = "/mm_rpc_setup";

    using TBasicHandler::TBasicHandler;

    void Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext);

private:
    bool ProcessInputs(NAppHost::IServiceContext& ctx, NGProxy::TMetadata& meta,
                       NGProxy::GSetupRequestInfo& info, google::protobuf::Any& req,
                       TLogContext& logContext);
};

}   // namespace NGProxy
