#pragma once

#include "node.h"

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/registry/registry.h>
#include <alice/megamind/library/sources/request.h>
#include <alice/megamind/library/util/status.h>

#include <util/generic/fwd.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf AH_ITEM_POLYGLOT_HTTP_REQUEST_NAME = "mm_polyglot_http_request";
inline constexpr TStringBuf AH_ITEM_POLYGLOT_HTTP_RESPONSE_NAME = "mm_polyglot_http_response";

TSourcePrepareStatus GetUtteranceFromEvent(TRequestComponentsView<TEventComponent> view, TString& utterance);

TStatus AppHostPolyglotSetup(IAppHostCtx& ahCtx, TRequestComponentsView<TClientComponent, TEventComponent> request,
                             TMaybe<TString>& utterance, TStringBuf languagePair);

TStatus AppHostPolyglotPostSetup(IAppHostCtx& ahCtx, TPolyglotTranslateUtteranceResponse& polyglotTranslateUtteranceResponse);

} // namespace NAlice::NMegamind
