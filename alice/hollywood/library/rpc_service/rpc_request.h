#pragma once

#include <alice/cuttlefish/library/protos/context_load.pb.h>

#include <alice/hollywood/library/hw_service_context/context.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NHollywood::NRpc {

inline constexpr TStringBuf REQUEST_META_ITEM_NAME = "mm_scenario_request_meta";
inline constexpr TStringBuf CONTEXT_LOAD_ITEM_NAME = "context_load_response";
inline constexpr TStringBuf RPC_REQUEST_ITEM_NAME = "rpc_request";

template <typename TRequestType>
class TRpcRequestWrapper {
public:
    TRpcRequestWrapper(THwServiceContext& ctx) {
        RequestMeta = ctx.GetMaybeProto<NAlice::NScenarios::TRequestMeta>(REQUEST_META_ITEM_NAME);
        ContextLoadResponse = ctx.GetMaybeProto<NAliceProtocol::TContextLoadResponse>(CONTEXT_LOAD_ITEM_NAME);
        RpcRequest = ctx.GetProtoOrThrow<NAlice::NScenarios::TScenarioRpcRequest>(RPC_REQUEST_ITEM_NAME);
        Y_ENSURE(RpcRequest.GetRequest().UnpackTo(&Request), "Cannot unpack rpc request");
    }

    const TMaybe<NAlice::NScenarios::TRequestMeta>& GetRequestMeta() const {
        return RequestMeta;
    }

    const TMaybe<NAliceProtocol::TContextLoadResponse>& GetContextLoadResponse() const {
        return ContextLoadResponse;
    }

    const TRequestType& GetRequest() const {
        return Request;
    }

    const NAlice::NScenarios::TScenarioRpcRequest::TRpcBaseRequest GetBaseRequest() const {
        return RpcRequest.GetBaseRequest();
    }

private:
    TMaybe<NAlice::NScenarios::TRequestMeta> RequestMeta;
    TMaybe<NAliceProtocol::TContextLoadResponse> ContextLoadResponse; // TODO split into datasources
    NAlice::NScenarios::TScenarioRpcRequest RpcRequest;
    TRequestType Request;
};

} // namespace NAlice::NHollywood::NRpc
