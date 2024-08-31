#include "endpoint.h"

#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>

#include <google/protobuf/any.pb.h>

#include <util/generic/algorithm.h>

namespace NAlice::NHollywood {

const TEndpoint* FindEndpoint(const TEnvironmentState& environmentState, TEndpoint_EEndpointType type) {
    return FindIfPtr(environmentState.GetEndpoints(), [type](const auto& endpoint) {
        return endpoint.GetMeta().GetType() == type;
    });
}

const TEndpoint* FindEndpoint(const TEnvironmentState& environmentState, TStringBuf deviceId) {
    return FindIfPtr(environmentState.GetEndpoints(), [deviceId](const auto& endpoint) {
        return endpoint.GetId() == deviceId;
    });
}

bool ParseTypedCapability(google::protobuf::Message& typedCapability, const TEndpoint& endpoint) {
    for (const auto& capability : endpoint.GetCapabilities()) {
        if (capability.UnpackTo(&typedCapability)) {
            return true;
        }
    }
    return false;
}

} // namespace NAlice::NHollywood
