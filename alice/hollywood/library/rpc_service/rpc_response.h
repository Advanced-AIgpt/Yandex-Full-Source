#pragma once

#include <alice/library/version/version.h>

#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/protos/api/rpc/status.pb.h>

namespace NAlice::NHollywood::NRpc {

template <typename TResponseType>
class TRpcResponseBuilder {
public:
    TRpcResponseBuilder() {
        Response.SetVersion(NAlice::VERSION_STRING);
    }

    TRpcResponseBuilder& SetAnalyticsInfo(const NAlice::NScenarios::TAnalyticsInfo& analyticsInfo) {
        Response.MutableAnalyticsInfo()->CopyFrom(analyticsInfo);
        return *this;
    }

    TRpcResponseBuilder& AddServerDirective(const NScenarios::TServerDirective& directive) {
        Response.AddServerDirectives()->CopyFrom(directive);
        return *this;
    }

    TRpcResponseBuilder& SetResponseBody(const TResponseType& response) {
        Response.MutableResponseBody()->PackFrom(response);
        return *this;
    }

    TRpcResponseBuilder& SetResponseError(const ::NAlice::NRpc::TStatus& error) {
        Response.MutableError()->CopyFrom(error);
        return *this;
    }

    NAlice::NScenarios::TScenarioRpcResponse Build() && {
        return std::move(Response);
    }

private:
    NAlice::NScenarios::TScenarioRpcResponse Response;
};

} // namespace NAlice::NMegamind
