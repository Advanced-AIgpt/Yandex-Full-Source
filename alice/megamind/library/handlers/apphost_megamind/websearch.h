#pragma once

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/search/request.h>
#include <alice/megamind/library/util/status.h>

#include <util/generic/variant.h>
#include <util/system/mutex.h>

namespace NAlice::NMegamind {

TStatus AppHostWebSearchSetup(IAppHostCtx& ahCtx, const TSpeechKitRequest& skr, const IEvent& event,
                              TWebSearchRequestBuilder& builder);

TSearchResponse AppHostWebSearchPostSetup(IAppHostCtx& ahCtx);

} // namespace NAlice::NMegaamind
