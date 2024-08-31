#pragma once

#include "node.h"

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/globalctx/fwd.h>
#include <alice/megamind/library/registry/registry.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/util/status.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <util/generic/yexception.h>

#include <functional>

namespace NAlice::NMegamind {

void RegisterAppHostMegamindHandlers(IGlobalCtx& globalCtx, TRegistry& registry);

class TAppHostSkrNodeHandler final : public TAppHostNodeHandler {
public:
    using TAppHostNodeHandler::TAppHostNodeHandler;

public:
    TStatus Execute(IAppHostCtx& ahCtx) const override;
    TRTLogger CreateLogger(NAppHost::IServiceContext& ctx) const override;
};

class TAppHostInitSkr {
public:
    struct TSkrCreationError : public yexception {
        TSkrCreationError(TError&& error);

        const TError Error;
    };

public:
    TStatus ParseHttp(TRequestCtx& requestCtx);

protected:
    virtual IAppHostCtx& AhCtx() = 0;
    virtual TStatus OnSuccess(const IContext& ctx) = 0;
    virtual TSpeechKitRequest CreateSkr(TSpeechKitInitContext& initCtx) = 0;
};

} // namespace NAlice::NMegamind
