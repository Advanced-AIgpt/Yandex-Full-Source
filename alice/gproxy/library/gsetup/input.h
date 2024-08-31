#pragma once

#include "handler.h"


namespace NGProxy {

class TGSetupInput : public TBasicHandler<TGSetupInput> {
public:
    static constexpr const char *Path = "/input";

    using TBasicHandler::TBasicHandler;

    TGSetupInput() = default;

    void Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext);

private:
    bool ProcessInputs(NAppHost::IServiceContext& ctx, NGProxy::TMetadata& metadata, NGProxy::GSetupRequestInfo& request, TLogContext& logContext);
};

}   // namespace NGProxy
