#include "blackbox.h"


namespace NAlice::NCuttlefish::NAppHostServices {

    void SetupBlackboxForGuest(
        const NVoicetech::NUniproxy2::TMessage& /* message */,
        const NAliceProtocol::TSessionContext& /* sessionContext */,
        const NAliceProtocol::TRequestContext& /* requestContext */,
        NAppHost::TServiceContextPtr /* appHostContext */
    ) {
        // TODO: make me here instead of TWsStreamToProtobuf::StartGuestContextLoad
    }

    void SetupBlackboxForOwner(
        const NVoicetech::NUniproxy2::TMessage& /* message */,
        const NAliceProtocol::TSessionContext& /* sessionContext */,
        const NAliceProtocol::TRequestContext& /* requestContext */,
        NAppHost::TServiceContextPtr /* appHostContext */
    ) {
        // TODO: make me here instead of context_load_pre
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
