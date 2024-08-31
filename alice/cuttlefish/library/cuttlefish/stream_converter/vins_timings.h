#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/library/json/json.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    void BuildLegacyVinsTimings(
        NJson::TJsonValue& payload,
        const NAliceProtocol::TRequestDebugInfo&,
        TLogContext* logContext = nullptr
    );
    void BuildLegacyTtsTimings(
        NJson::TJsonValue& payload,
        const NAliceProtocol::TRequestDebugInfo&,
        TLogContext* logContext = nullptr
    );
}
