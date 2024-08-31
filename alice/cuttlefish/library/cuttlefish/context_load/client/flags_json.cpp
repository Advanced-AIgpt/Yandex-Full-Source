#include "flags_json.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <voicetech/library/settings_manager/proto/settings.pb.h>

namespace {

    NAliceProtocol::TAbFlagsProviderOptions MakeAbExperimentsOptions(
        const NVoicetech::NUniproxy2::TMessage& message,
        const NAliceProtocol::TRequestContext& requestContext
    ) {
        NAliceProtocol::TAbFlagsProviderOptions options;

        if (const NJson::TJsonValue* request = message.Json.GetValueByPath("event.payload.request")) {
            if (const auto* uaasTests = request->GetMap().FindPtr("uaas_tests")) {
                for (const NJson::TJsonValue& uaasTestId : uaasTests->GetArray()) {
                    if (uaasTestId.IsString()) {
                        *(options.AddTestIds()) = uaasTestId.GetString();
                    } else if (uaasTestId.IsInteger()) {
                        *(options.AddTestIds()) = ToString(uaasTestId.GetInteger());
                    }
                }
            }
        }

        using namespace NAlice::NCuttlefish::NExpFlags;
        options.SetDisregardUaas(ExperimentFlagHasTrueValue(requestContext, DISREGARD_UAAS));
        options.SetOnly100PercentFlags(ExperimentFlagHasTrueValue(requestContext, ONLY_100_PERCENT_FLAGS));

        return options;
    }

}  // anonymous namespace


namespace NAlice::NCuttlefish::NAppHostServices {

    void SetupFlagsJsonForOwner(
        const NVoicetech::NUniproxy2::TMessage& message,
        const NAliceProtocol::TSessionContext& /* sessionContext */,
        const NAliceProtocol::TRequestContext& requestContext,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        appHostContext->AddProtobufItem(
            MakeAbExperimentsOptions(message, requestContext),
            ITEM_TYPE_AB_EXPERIMENTS_OPTIONS
        );
        appHostContext->AddFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_FLAGS_JSON);

        // Temporary condition (VOICESERV-4315)
        // @TODO remove after next release (athene-grey)
        if (requestContext.GetSettingsFromManager().GetUseIsolatedFlagsJsonService()) {
            appHostContext->AddFlag("use_isolated_flags_json_service");
        }
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
