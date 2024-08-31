#pragma once

#include "handler.h"


namespace NGProxy {

class TGSetupHttpOutput : public TBasicHandler<TGSetupHttpOutput> {
public:
    static constexpr const char *Path = "/http_output";

    using TBasicHandler::TBasicHandler;

    TGSetupHttpOutput() = default;

    void Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext);
};

}   // namespace NGProxy
