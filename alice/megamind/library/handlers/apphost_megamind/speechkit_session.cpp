#include "speechkit_session.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/util.h>


namespace NAlice::NMegamind {

void AppHostSpeechKitSessionSetup(IAppHostCtx& ahCtx, const TSessionProto& sessionProto) {
    ahCtx.ItemProxyAdapter().PutIntoContext(sessionProto, AH_ITEM_SPEECHKIT_SESSION);
}

TStatus AppHostSpeechKitSessionPostSetup(IAppHostCtx& ahCtx, TSessionProto& sessionProto) {
    if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_SPEECHKIT_SESSION, sessionProto)) {
        LOG_WARN(ahCtx.Log()) << "Unable to find item: " << AH_ITEM_SPEECHKIT_SESSION;
        return std::move(*err);
    }
    return Success();
}

} // namespace NAlice::NMegamind
