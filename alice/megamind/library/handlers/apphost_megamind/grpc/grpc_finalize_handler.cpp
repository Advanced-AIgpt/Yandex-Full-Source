#include "grpc_finalize_handler.h"

#include "common.h"

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/grpc_request/response_builder.h>

#include <alice/megamind/protos/grpc_request/analytics_info.pb.h>
#include <alice/megamind/protos/grpc_request/request.pb.h>
#include <alice/megamind/protos/grpc_request/response.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/version/version.h>

namespace NAlice::NMegamind::NRpc {

TStatus TRpcFinalizeNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    TItemProxyAdapter& itemProxyAdapter = ahCtx.ItemProxyAdapter();

    const auto errorOrRequest = itemProxyAdapter.GetFromContext<NAlice::NRpc::TRpcRequestProto>(MM_RPC_REQUEST_ITEM_NAME);
    if (errorOrRequest.Error()) {
        return TError{TError::EType::Critical} << "Cannot get mm_rpc_request: " << *errorOrRequest.Error();
    }
    const auto& request = errorOrRequest.Value();

    const auto errorOrResponse = itemProxyAdapter.GetFromContext<NScenarios::TScenarioRpcResponse>(SCENARIO_RPC_RESPONSE_ITEM_NAME);
    if (errorOrResponse.Error()) {
        return TError{TError::EType::Critical} << "Cannot get mm_rpc_response: " << *errorOrResponse.Error();
    }
    const auto& response = errorOrResponse.Value();

    NAlice::NRpc::TRpcAnalyticsInfo analyticsInfo;
    analyticsInfo.MutableAnalyticsInfo()->CopyFrom(response.GetAnalyticsInfo());

    auto responseBuilder =
        TMegamindRpcResponseBuilder()
            .SetAnalyticsInfo(analyticsInfo)
            .SetServerDirectives(response.GetServerDirectives());

    if (response.HasError()) {
        // TODO add sensor
        LOG_INFO(ahCtx.Log()) << "Got error from rpc request to " << request.GetHandler() << ": "
                              << JsonFromProto(response.GetError()) << Endl;
        responseBuilder.SetResponseError(response.GetError());
    } else {
        responseBuilder.SetResponseBody(response.GetResponseBody());
    }

    ahCtx.ItemProxyAdapter().PutIntoContext(std::move(responseBuilder).Build(), MM_RPC_RESPONSE_ITEM_NAME);
    return Success();
}

} // namespace NAlice::NMegaamind
