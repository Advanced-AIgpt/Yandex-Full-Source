#pragma once

#include "node.h"

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/library/walker/walker.h>

namespace NAlice::NMegamind {

class TAppHostWalkerRunNodeHandler : public TAppHostNodeHandler {
public:
    TAppHostWalkerRunNodeHandler(
        IGlobalCtx& globalCtx,
        TWalkerPtr walker,
        ILightWalkerRequestCtx::ERunStage runStage,
        bool useAppHostStreaming);

protected:
    TWalkerPtr Walker_;
    ILightWalkerRequestCtx::ERunStage RunStage_;
};

class TAppHostPostClassifyNodeHandler final : public TAppHostWalkerRunNodeHandler {
public:
    using TAppHostWalkerRunNodeHandler::TAppHostWalkerRunNodeHandler;

    TStatus Execute(IAppHostCtx& ahCtx) const override;
};

class TAppHostPreClassifyNodeHandler final : public TAppHostWalkerRunNodeHandler {
public:
    using TAppHostWalkerRunNodeHandler::TAppHostWalkerRunNodeHandler;

    TStatus Execute(IAppHostCtx& ahCtx) const override;
};

class TAppHostWalkerRunProcessContinueHandler final : public TAppHostWalkerRunNodeHandler {
public:
    using TAppHostWalkerRunNodeHandler::TAppHostWalkerRunNodeHandler;

    TStatus Execute(IAppHostCtx& ahCtx) const override;
};

class TAppHostWalkerRunFinalizeNodeHandler final : public TAppHostWalkerRunNodeHandler {
public:
    using TAppHostWalkerRunNodeHandler::TAppHostWalkerRunNodeHandler;

    TStatus Execute(IAppHostCtx& ahCtx) const override;
};

class TAppHostWalkerRunClassifyWinnerNodeHandler final : public TAppHostWalkerRunNodeHandler {
public:
    using TAppHostWalkerRunNodeHandler::TAppHostWalkerRunNodeHandler;

    TStatus Execute(IAppHostCtx& ahCtx) const override;
};

class TAppHostWalkerRunProcessCombinatorContinueNodeHandler final : public TAppHostWalkerRunNodeHandler {
public:
    using TAppHostWalkerRunNodeHandler::TAppHostWalkerRunNodeHandler;

    TStatus Execute(IAppHostCtx& ahCtx) const override;
};

} // namespace NAlice::NMegamind
