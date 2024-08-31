#pragma once

#include "handler.h"


namespace NGProxy {

class TGSetupHttpInit : public TBasicHandler<TGSetupHttpInit> {
public:
    static constexpr const char *Path = "/http_init";

    using TBasicHandler::TBasicHandler;

    TGSetupHttpInit() = default;

    void Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext);
};

}   // namespace NGProxy
