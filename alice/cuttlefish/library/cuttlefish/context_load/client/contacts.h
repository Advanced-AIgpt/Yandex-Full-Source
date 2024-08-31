#pragma once

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <voicetech/library/messages/message.h>
#include <apphost/api/service/cpp/service_context.h>


namespace NAlice::NCuttlefish::NAppHostServices {

    void SetupContactsJsonForOwner(
        const NVoicetech::NUniproxy2::TMessage& message,
        const NAliceProtocol::TSessionContext& sessionContext,
        const NAliceProtocol::TRequestContext& requestContext,
        NAppHost::TServiceContextPtr appHostContext
    );

    void SetupContactsProtoForOwner(
        const NVoicetech::NUniproxy2::TMessage& message,
        const NAliceProtocol::TSessionContext& sessionContext,
        const NAliceProtocol::TRequestContext& requestContext,
        NAppHost::TServiceContextPtr appHostContext
    );

}  // namespace NAlice::NCuttlefish::NAppHostServices
