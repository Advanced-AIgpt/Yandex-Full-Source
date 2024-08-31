#pragma once

#include "node.h"

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/registry/registry.h>
#include <alice/megamind/library/util/status.h>

namespace NAlice::NMegamind {

class TAppHostUtterancePostSetupNodeHandler : public TAppHostNodeHandler {
public:
    using TAppHostNodeHandler::TAppHostNodeHandler;

    TStatus Execute(IAppHostCtx& ahCtx) const override;
};

void RegisterAppHostUtteranceHandlers(IGlobalCtx& globalCtx, TRegistry& registry);

TStatus AppHostOnUtteranceReadySetup(IAppHostCtx& ahCtx, TString utterance, const IContext& ctx);

} // namespace NAlice::NMegaamind
