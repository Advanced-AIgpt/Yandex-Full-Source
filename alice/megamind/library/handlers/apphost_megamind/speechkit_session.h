#pragma once


#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/session/protos/session.pb.h>

#include <alice/megamind/library/util/status.h>


namespace NAlice::NMegamind {

void AppHostSpeechKitSessionSetup(IAppHostCtx& ahCtx, const TSessionProto& sessionProto);
TStatus AppHostSpeechKitSessionPostSetup(IAppHostCtx& ahCtx, TSessionProto& rewrittenRequestProto);

} // namespace NAlice::NMegamind
