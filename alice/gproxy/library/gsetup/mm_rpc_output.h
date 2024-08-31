#pragma once

#include "handler.h"

namespace NGProxy {

class TGSetupMMRpcOutput : public TBasicHandler<TGSetupMMRpcOutput> {
public:
    static constexpr const char *Path = "/mm_rpc_output";

    using TBasicHandler::TBasicHandler;

    void Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext);
    // TODO (nkodosov) context_save
};

}   // namespace NGProxy
