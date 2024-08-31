#pragma once

#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/json/writer/json_value.h>

namespace NAlice::NCuttlefish::NAppHostServices {

class TFlagsJson {
public:
    static NAppHostHttp::THttpRequest GetExperiments(
        const NAliceProtocol::TSessionContext& ctx,
        const NAliceProtocol::TAbFlagsProviderOptions& options
    );

    static NJson::TJsonValue GetExperimentsJson(
        const NAliceProtocol::TSessionContext& ctx,
        const NAliceProtocol::TAbFlagsProviderOptions& options,
        NAlice::NCuttlefish::TSourceMetrics& metrics,
        NAlice::NCuttlefish::TLogContext& logContext
    );

    static void ParseResponse(
        NAliceProtocol::TExperimentsContext* dst,
        TString resp
    );

    static void CountFlags(
        const NAliceProtocol::TFlagsInfo& flagsInfo,
        const TMaybe<NAliceProtocol::TAbFlagsProviderOptions>& options,
        NAlice::NCuttlefish::TSourceMetrics* metrics,
        NAlice::NCuttlefish::TLogContext* logContext
    );
};


}  // namespace NAlice::NCuttlefish::NAppHostServices
