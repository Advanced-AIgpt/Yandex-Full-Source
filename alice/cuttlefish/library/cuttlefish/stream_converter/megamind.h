#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/protos/megamind.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/wsevent.pb.h>
#include <voicetech/library/messages/message.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    class TMegamindRequestConstructorError : public yexception {
    };

    void MessageToMegamindRequest(
        NAliceProtocol::TMegamindRequest&,
        NAliceProtocol::TRequestContext&,
        const NVoicetech::NUniproxy2::TMessage&,
        const NAliceProtocol::TSessionContext&,
        TLogContext* logContext = nullptr
    );

    void UpdateSpeechKitRequest(NAlice::TSpeechKitRequestProto&, const NAliceProtocol::TSessionContext&, const NAliceProtocol::TWsEvent&);
    void UpdateSpeechKitRequest(NAlice::TSpeechKitRequestProto&, const NAliceProtocol::TSessionContext&, const NJson::TJsonValue& messageJson);
}
