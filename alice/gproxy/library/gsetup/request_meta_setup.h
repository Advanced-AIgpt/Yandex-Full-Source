#pragma once

#include "handler.h"

namespace NGProxy {

    class TGSetupRequestMeta: public TBasicHandler<TGSetupRequestMeta> {
    public:
        static constexpr const char* Path = "/request_meta_setup";

        using TBasicHandler::TBasicHandler;

        void Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext);
    };

} // namespace NGProxy
