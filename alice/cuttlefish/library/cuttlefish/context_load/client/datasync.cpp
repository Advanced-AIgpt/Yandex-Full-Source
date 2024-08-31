#include "datasync.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>


namespace NAlice::NCuttlefish::NAppHostServices {

    void SetupDatasyncForGuest(
        const NVoicetech::NUniproxy2::TMessage& /* message */,
        const NAliceProtocol::TSessionContext& /* sessionContext */,
        const NAliceProtocol::TRequestContext& /* requestContext */,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        appHostContext->AddFlag(EDGE_FLAG_LOAD_GUEST_CONTEXT_SOURCE_DATASYNC);
    }

    void SetupDatasyncForOwner(
        const NVoicetech::NUniproxy2::TMessage& /* message */,
        const NAliceProtocol::TSessionContext& /* sessionContext */,
        const NAliceProtocol::TRequestContext& /* requestContext */,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        appHostContext->AddFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_DATASYNC);
        appHostContext->AddFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_DATASYNC);
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
