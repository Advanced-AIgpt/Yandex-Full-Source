#pragma once

#include "node.h"

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/globalctx/fwd.h>
#include <alice/megamind/library/misspell/misspell.h>
#include <alice/megamind/library/registry/registry.h>
#include <alice/megamind/library/request_composite/client/client.h>
#include <alice/megamind/library/request_composite/event.h>
#include <alice/megamind/library/util/status.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf AH_ITEM_MISSPELL_HTTP_REQUEST_NAME = "mm_misspell_http_request";
inline constexpr TStringBuf AH_ITEM_MISSPELL_HTTP_RESPONSE_NAME = "mm_misspell_http_response";

TStatus AppHostMisspellSetup(IAppHostCtx& ahCtx, TRequestComponentsView<TClientComponent, TEventComponent> request,
                             TMaybe<TString>& utterance);

TStatus AppHostMisspellPostSetup(IAppHostCtx& ahCtx, TMaybe<TString>& utterance);

TStatus AppHostMisspellFromContext(IAppHostCtx& ahCtx, TMisspellProto& misspellProto);

} // namespace NAlice::NMegamind
