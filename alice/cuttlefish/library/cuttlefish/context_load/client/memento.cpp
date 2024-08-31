#include "memento.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>


namespace NAlice::NCuttlefish::NAppHostServices {

    void SetupMementoForOwner(
        const NVoicetech::NUniproxy2::TMessage& /* message */,
        const NAliceProtocol::TSessionContext& /* sessionContext */,
        const NAliceProtocol::TRequestContext& /* requestContext */,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        appHostContext->AddFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_MEMENTO);
        appHostContext->AddFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_MEMENTO);
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
