#pragma once

#include <alice/library/version/version.h>

#include <alice/megamind/protos/grpc_request/analytics_info.pb.h>
#include <alice/megamind/protos/grpc_request/response.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <alice/protos/api/rpc/status.pb.h>

namespace NAlice::NMegamind::NRpc {

class TMegamindRpcResponseBuilder {
public:
    TMegamindRpcResponseBuilder() {
        Response.SetVersion(NAlice::VERSION_STRING);
    }

    TMegamindRpcResponseBuilder& SetAnalyticsInfo(const NAlice::NRpc::TRpcAnalyticsInfo& analyticsInfo) {
        Response.MutableAnalyticsInfo()->CopyFrom(analyticsInfo);
        return *this;
    }

    TMegamindRpcResponseBuilder&
    SetServerDirectives(const google::protobuf::RepeatedPtrField<NScenarios::TServerDirective>& serverDirectives) {
        Response.MutableServerDirectives()->CopyFrom(serverDirectives);
        return *this;
    }

    TMegamindRpcResponseBuilder& SetResponseBody(const google::protobuf::Any& response) {
        Response.MutableResponseBody()->CopyFrom(response);
        return *this;
    }

    TMegamindRpcResponseBuilder& SetResponseError(const NAlice::NRpc::TStatus& error) {
        Response.MutableError()->CopyFrom(error);
        return *this;
    }

    NAlice::NRpc::TRpcResponseProto Build() && {
        return std::move(Response);
    }

private:
    NAlice::NRpc::TRpcResponseProto Response;
};

} // namespace NAlice::NMegamind
