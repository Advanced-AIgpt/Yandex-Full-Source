#pragma once

#include "node.h"

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/globalctx/fwd.h>
#include <alice/megamind/library/registry/registry.h>
#include <alice/megamind/library/util/status.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf AH_ITEM_COMBINATOR_REQUEST_PREFIX = "combinator_request_apphost_type_";
inline constexpr TStringBuf AH_ITEM_COMBINATOR_CONTINUE_REQUEST_PREFIX = "combinator_continue_request_apphost_type_";
inline constexpr TStringBuf AH_ITEM_COMBINATOR_RESPONSE_PREFIX = "combinator_response_apphost_type_";
inline constexpr TStringBuf AH_ITEM_COMBINATOR_CONTINUE_RESPONSE_PREFIX = "combinator_continue_response_apphost_type_";

void RegisterCombinatorHandlers(IGlobalCtx& globalCtx, TRegistry& registry);

class TCombinatorSetupNodeHandler : public TAppHostNodeHandler {
public:
    using TAppHostNodeHandler::TAppHostNodeHandler;

    TStatus Execute(IAppHostCtx& ahCtx) const override;
};

} // namespace NAlice::NMegaamind
