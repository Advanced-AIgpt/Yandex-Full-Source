#include "grpc_setup_handler.h"

#include "common.h"

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/grpc_request/scenario_request_builder.h>
#include <alice/megamind/library/request/request.h>

#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/grpc_request/request.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <alice/library/geo/protos/user_location.pb.h>

#include <google/protobuf/struct.pb.h>

namespace NAlice::NMegamind::NRpc {

namespace {

TString ConstructRequestItemName(const TString& handlerName) {
    return SCENARIO_RPC_REQUEST_ITEM_NAME_PREFIX + handlerName;
}

} // namespace

TStatus TRpcSetupNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    TItemProxyAdapter& itemProxyAdapter = ahCtx.ItemProxyAdapter();

    const auto errorOrRequest = itemProxyAdapter.GetFromContext<NAlice::NRpc::TRpcRequestProto>(MM_RPC_REQUEST_ITEM_NAME);
    if (errorOrRequest.Error()) {
        return TError{TError::EType::Critical} << "Cannot get mm_rpc_request: " << *errorOrRequest.Error();
    }
    const auto& request = errorOrRequest.Value();

    const auto& handlerName = request.GetHandler();
    // TODO check handler in configs
    if (handlerName.empty()) {
        return TError{TError::EType::Critical} << "Got empty rpc handler name" << Endl;
    }

    TMaybe<TLocation> deviceLocation; // TODO get from context
    const auto location = ParseLocation(deviceLocation, request.GetMeta().GetLaasRegion());
    const auto userLocation = ParseUserLocation(ahCtx.GlobalCtx().GeobaseLookup(), location,
                                                request.GetMeta().GetApplication().GetTimezone(), ahCtx.Log());
    if (userLocation.Defined()) {
        ahCtx.ItemProxyAdapter().PutIntoContext(userLocation->BuildProto(), USER_LOCATION_DATASOURCE_ITEM_NAME);
    }

    const auto scenarioRpcRequest = CreateScenarioRpcRequest(request, location, ahCtx.Log());

    ahCtx.ItemProxyAdapter().PutIntoContext(scenarioRpcRequest, ConstructRequestItemName(handlerName));
    return Success();
}

} // namespace NAlice::NMegaamind
