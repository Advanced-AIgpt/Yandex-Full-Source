#pragma once

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/apphost_request/protos/begemot_response_parts.pb.h>
#include <alice/megamind/library/begemot/begemot.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/util/status.h>

#include <util/generic/string.h>

namespace NAlice::NMegamind {

namespace NImpl {

ELogPriority GetLogPolicy(bool logPolicyInfo);

}

inline constexpr TStringBuf AH_ITEM_BEGEMOT_NATIVE_REQUEST_NAME = "mm_begemot_native_request";
inline constexpr TStringBuf AH_ITEM_BEGEMOT_MERGER_REQUEST = "mm_begemot_merger_request";
inline constexpr TStringBuf AH_ITEM_BEGEMOT_NATIVE_BEGGINS_REQUEST_NAME = "mm_begemot_native_beggins_request";

inline constexpr TStringBuf AH_ITEM_POLYGLOT_BEGEMOT_NATIVE_REQUEST_NAME = "mm_polyglot_begemot_native_request";
inline constexpr TStringBuf AH_ITEM_POLYGLOT_BEGEMOT_MERGER_REQUEST = "mm_polyglot_begemot_merger_request";
inline constexpr TStringBuf AH_ITEM_POLYGLOT_BEGEMOT_NATIVE_BEGGINS_REQUEST_NAME = "mm_polyglot_begemot_native_beggins_request";

inline constexpr TStringBuf AH_ITEM_POLYGLOT_BEGEMOT_MERGER_MERGER_REQUEST = "mm_polyglot_begemot_merger_merger_request";

TStatus AppHostBegemotSetup(IAppHostCtx& ahCtx, const TString& utterance, const IContext& ctx);
TStatus AppHostPolyglotBegemotSetup(IAppHostCtx& ahCtx, const TString& utterance, const IContext& ctx);

TStatus AppHostBegemotPostSetup(IAppHostCtx& ahCtx, const IContext& ctx, TWizardResponse& wizardResponse);

void AppHostBegemotResponseRewrittenRequestSetup(IAppHostCtx& ahCtx, const TString& rewrittenRequest);
TStatus AppHostBegemotResponseRewrittenRequestPostSetup(
    IAppHostCtx& ahCtx,
    NMegamindAppHost::NBegemotResponseParts::TRewrittenRequest& rewrittenRequestProto
);

} // namespace NAlice::NMegamind
