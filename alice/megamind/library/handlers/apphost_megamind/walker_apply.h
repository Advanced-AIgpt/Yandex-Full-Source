#pragma once

#include "node.h"

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/library/walker/requestctx.h>
#include <alice/megamind/library/walker/scenario.h>

namespace NAlice::NMegamind {

class TAppHostWalkerApplyNodeHandler : public TAppHostNodeHandler {
public:
    TAppHostWalkerApplyNodeHandler(
        IGlobalCtx& globalCtx,
        TWalkerPtr walker,
        ILightWalkerRequestCtx::ERunStage runStage,
        bool useAppHostStreaming);

    TStatus Execute(IAppHostCtx& ahCtx) const override;

protected:
    TWalkerPtr Walker_;
    ILightWalkerRequestCtx::ERunStage RunStage_;
};

class TAppHostWalkerApplyModifiersNodeHandler : public TAppHostNodeHandler {
public:
    TAppHostWalkerApplyModifiersNodeHandler(IGlobalCtx& globalCtx, TWalkerPtr walker);

    TStatus Execute(IAppHostCtx& ahCtx) const override;

protected:
    TWalkerPtr Walker_;
};

} // namespace NAlice::NMegamind
