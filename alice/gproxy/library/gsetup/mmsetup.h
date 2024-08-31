#pragma once


#include "handler.h"


namespace NGProxy {

class TGSetupMM : public TBasicHandler<TGSetupMM> {
public:
    static constexpr const char *Path = "/mm_setup";

    using TBasicHandler::TBasicHandler;

    void Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext);

private:
    bool ProcessInputs(NAppHost::IServiceContext& ctx, NGProxy::TMetadata& meta, NGProxy::GSetupRequestInfo& info, NGProxy::GSetupRequest& req, TLogContext& logContext);
};

}   // namespace NGProxy
