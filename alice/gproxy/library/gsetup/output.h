#pragma once

#include "handler.h"


namespace NGProxy {

class TGSetupOutput : public TBasicHandler<TGSetupOutput> {
public:
    static constexpr const char *Path = "/output";

    using TBasicHandler::TBasicHandler;

    void Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext);

private:
    void SetupContextSaveRequest(
        NAppHost::IServiceContext& ctx,
        const NGProxy::TMetadata& meta,
        const NJson::TJsonValue& mmResponse,
        TLogContext& logContext
    ) const;

    void BuildGSetupResponse(
        NAppHost::IServiceContext& ctx,
        const NGProxy::TMetadata& meta,
        const NGProxy::GSetupRequestInfo& info,
        NJson::TJsonValue mmResponse,
        TLogContext& logContext
    );
};

}   // namespace NGProxy
