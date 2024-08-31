#pragma once

#include "node.h"

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/registry/registry.h>
#include <alice/megamind/library/util/status.h>

namespace NAlice::NMegamind {

class TAppHostFallbackResponseNodeHandler : public TAppHostNodeHandler {
public:
    TAppHostFallbackResponseNodeHandler(IGlobalCtx& globalCtx);

    TStatus Execute(IAppHostCtx& ahCtx) const override;

public:
    static void Register(IGlobalCtx& globalCtx, TRegistry& registry);
};

} // namespace NAlice::NMegamind
