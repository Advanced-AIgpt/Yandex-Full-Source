#pragma once

#include <alice/megamind/library/globalctx/fwd.h>
#include <alice/megamind/library/registry/registry.h>

namespace NAlice::NMegamind::NRpc {

void RegisterRpcHandlers(IGlobalCtx& globalCtx, TRegistry& registry);

} // namespace NAlice::NMegamind
