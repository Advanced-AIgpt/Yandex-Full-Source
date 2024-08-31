#pragma once

#include <util/generic/strbuf.h>

namespace google::protobuf {
    class Message;
} // namespace google::protobuf

namespace NAlice {
    class TEndpoint;
    class TEnvironmentState;
    enum TEndpoint_EEndpointType : int;
} // namespace NAlice

namespace NAlice::NHollywood {

const TEndpoint* FindEndpoint(const TEnvironmentState& environmentState, TEndpoint_EEndpointType type);
const TEndpoint* FindEndpoint(const TEnvironmentState& environmentState, TStringBuf deviceId);
bool ParseTypedCapability(google::protobuf::Message& typedCapability, const TEndpoint& endpoint);

} // namespace NAlice::NHollywood
