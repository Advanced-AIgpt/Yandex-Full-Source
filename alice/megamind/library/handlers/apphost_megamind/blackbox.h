#pragma once

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/globalctx/fwd.h>
#include <alice/megamind/library/registry/registry.h>
#include <alice/megamind/library/request_composite/client/client.h>
#include <alice/megamind/library/request_composite/view.h>
#include <alice/megamind/library/util/status.h>

#include <alice/library/blackbox/blackbox.h>

#include <util/generic/strbuf.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf AH_ITEM_BLACKBOX_HTTP_REQUEST_NAME = "mm_blackbox_http_request";
inline constexpr TStringBuf AH_ITEM_BLACKBOX_HTTP_RESPONSE_NAME = "mm_blackbox_http_response";

TStatus AppHostBlackBoxSetup(IAppHostCtx& ahCtx, TRequestComponentsView<TClientComponent> request);
TStatus AppHostBlackBoxPostSetup(IAppHostCtx& ahCtx, TBlackBoxFullUserInfoProto& fullInfo);

} // namespace NAlice::NMegaamind
