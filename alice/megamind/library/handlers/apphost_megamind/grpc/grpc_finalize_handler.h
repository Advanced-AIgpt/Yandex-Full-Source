#pragma once

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/handlers/apphost_megamind/node.h>
#include <alice/megamind/library/util/status.h>

namespace NAlice::NMegamind::NRpc {

class TRpcFinalizeNodeHandler : public TAppHostNodeHandler {
public:
    using TAppHostNodeHandler::TAppHostNodeHandler;

    TStatus Execute(IAppHostCtx& ahCtx) const override;
};

} // namespace NAlice::NMegaamind
