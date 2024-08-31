#pragma once

#include "node.h"

#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/library/walker/walker.h>

namespace NAlice::NMegamind {

class TAppHostWalkerPrepareNodeHandler : public TAppHostNodeHandler {
public:
    TAppHostWalkerPrepareNodeHandler(IGlobalCtx& globalCtx, TWalkerPtr walker);

    TStatus Execute(IAppHostCtx& ahCtx) const override;

protected:
    TWalkerPtr Walker_;
};

} // namespace NAlice::NMegamind
