#pragma once

#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/registry/registry.h>

namespace NAlice::NMegamind {

void RegisterAppHostWalkerHandlers(IGlobalCtx& globalCtx, TRegistry& registry);

} // namespace NAlice::NMegamind
