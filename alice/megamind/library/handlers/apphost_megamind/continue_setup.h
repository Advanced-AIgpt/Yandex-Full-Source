#pragma once

#include "node.h"

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/globalctx/fwd.h>
#include <alice/megamind/library/registry/registry.h>
#include <alice/megamind/library/util/status.h>

namespace NAlice::NMegamind {

void RegisterContinueSetupHandler(IGlobalCtx& globalCtx, TRegistry& registry);

class TContinueSetupNodeHandler : public TAppHostNodeHandler {
public:
    using TAppHostNodeHandler::TAppHostNodeHandler;

    TStatus Execute(IAppHostCtx& ahCtx) const override;
};

} // namespace NAlice::NMegaamind
