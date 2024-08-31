#include "laas.h"

#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <alice/cuttlefish/library/protos/context_load.pb.h>


namespace {

    NAliceProtocol::TContextLoadLaasRequestOptions MakeLaasRequestOptions(
        const NAliceProtocol::TRequestContext& requestContext
    ) {
        NAliceProtocol::TContextLoadLaasRequestOptions options;
        options.SetUseCoordinatesFromIoT(
            NAlice::NCuttlefish::NExpFlags::ExperimentFlagHasTrueValue(
                requestContext,
                "use_coordinates_from_iot_in_laas_request"
            )
        );
        return options;
    }

}  // anonymous namespace


namespace NAlice::NCuttlefish::NAppHostServices {

    void SetupLaasForOwner(
        const NVoicetech::NUniproxy2::TMessage& /* message */,
        const NAliceProtocol::TSessionContext& /* sessionContext */,
        const NAliceProtocol::TRequestContext& requestContext,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        appHostContext->AddProtobufItem(MakeLaasRequestOptions(requestContext), ITEM_TYPE_LAAS_REQUEST_OPTIONS);
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
