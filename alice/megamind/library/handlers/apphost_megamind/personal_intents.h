#pragma once

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/kv_saas/response.h>
#include <alice/megamind/library/request_composite/client/client.h>
#include <alice/megamind/library/request_composite/view.h>
#include <alice/megamind/library/util/status.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf AH_ITEM_PERS_INTENT_HTTP_REQUEST_NAME = "mm_pers_intents_http_request";
inline constexpr TStringBuf AH_ITEM_PERS_INTENT_HTTP_RESPONSE_NAME = "mm_pers_intents_http_response";

TStatus AppHostPersonalIntentsSetup(IAppHostCtx& ahCtx, TRequestComponentsView<TClientComponent> view);
TStatus AppHostPersonalIntentsPostSetup(IAppHostCtx& ahCtx, NKvSaaS::TPersonalIntentsResponse& protoResponse);

} // namespace NAlice::NMegamind
