#pragma once

#include <alice/cuttlefish/library/asr/base/protobuf.h>

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <voicetech/library/messages/message.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    void MessageToAsrInitRequest(
        const NVoicetech::NUniproxy2::TMessage&,
        NAlice::NAsr::NProtobuf::TInitRequest&,
        const NAliceProtocol::TSessionContext&,
        const NAliceProtocol::TRequestContext&,
        TLogContext* logContext = nullptr
    );

    void ApplyLocalExperimentsHacks(
        NAlice::NAsr::NProtobuf::TInitRequest&,
        const NAliceProtocol::TSessionContext&,
        const NVoicetech::NUniproxy2::TMessage&,
        TLogContext* logContext = nullptr
    );

    void ApplyExperiments(
        NAlice::NAsr::NProtobuf::TInitRequest&,
        const NAliceProtocol::TRequestContext&,
        TLogContext* logContext = nullptr
    );

    void ApplySettings(
        NAlice::NAsr::NProtobuf::TInitRequest&,
        const NAliceProtocol::TRequestContext&,
        TLogContext* logContext = nullptr
    );
}
