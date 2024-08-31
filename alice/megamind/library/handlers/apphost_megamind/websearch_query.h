#pragma once

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/apphost_request/protos/web_search_query.pb.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/util/status.h>


namespace NAlice::NMegamind {

TStatus AppHostWebSearchQuerySetup(IAppHostCtx& ahCtx, const IContext& ctx, const IEvent& event);

TStatus AppHostWebSearchQueryPostSetup(IAppHostCtx& ahCtx, NMegamindAppHost::TWebSearchQueryProto& webSearchQueryProto);

} // namespace NAlice::NMegaamind
