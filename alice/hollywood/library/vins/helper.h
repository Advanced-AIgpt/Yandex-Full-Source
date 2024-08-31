#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NAlice::NHollywoodFw {

void EnrichRequestFromApphostItems(const TRunRequest& runRequest, NScenarios::TScenarioRunRequest& requestProto);

void AddExpFlag(const TString& name, NScenarios::TScenarioBaseRequest& requestProto);

void AddExpFlagRenderVinsNlgInHollywood(NScenarios::TScenarioBaseRequest& requestProto);

TCgiParameters BuildCgiParameters(const NJson::TJsonValue& params);

NHollywood::THttpProxyRequestBuilder CreateHttpRequestBuilder(const TRequest& request, const TCgiParameters& cgiParameters);

void AddHeaders(NHollywood::THttpProxyRequestBuilder& builder, const TRequest& request);

template <class TProto>
void AddBody(NHollywood::THttpProxyRequestBuilder& builder, const TProto& requestProto) {
    TString data;
    Y_ENSURE(requestProto.SerializeToString(&data),
        "Failed to serialize proto " << requestProto.GetTypeName() << " to string");
    builder.SetBody(data, NContentTypes::APPLICATION_PROTOBUF);
}

} // namespace NAlice::NHollywood::NVins
