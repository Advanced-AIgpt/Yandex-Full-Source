#pragma once

#include <alice/library/logger/logger.h>

#include <alice/megamind/library/request/request.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/grpc_request/request.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <google/protobuf/struct.pb.h>

namespace NAlice::NMegamind::NRpc {

NScenarios::TScenarioRpcRequest CreateScenarioRpcRequest(const NAlice::NRpc::TRpcRequestProto& request,
                                                         const TMaybe<TRequest::TLocation>& location,
                                                         TRTLogger& logger);

} // namespace NAlice::NMegamind
