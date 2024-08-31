#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/video_common/hollywood_helpers/proxy_request_builder.h>

#include <util/generic/string.h>

namespace NAlice::NVideoCommon {

TString MakeEntref(const TString& intoId);

void AddIrrelevantResponse(NAlice::NHollywood::TScenarioHandleContext& ctx);
void FillAnalyticsInfo(NAlice::NHollywood::TResponseBodyBuilder& bodyBuilder,
                       TStringBuf intent,
                       TStringBuf productScenarioName);

TCgiParameters GetDefaultWebviewCgiParams(const NHollywood::TScenarioRunRequestWrapper& request);
void AddCodecHeadersIntoRequest(TProxyRequestBuilder& requestBuilder, const NHollywood::TScenarioBaseRequestWrapper& scenarioRequest);
void AddCodecHeadersIntoRequest(TProxyRequestBuilder& requestBuilder, const NScenarios::TScenarioBaseRequest& baseRequestProto);

void AddIpregParam(TProxyRequestBuilder& requestBuilder, const NHollywood::TScenarioRunRequestWrapper& request);
void AddIpregParam(TCgiParameters& cgi, const NHollywood::TScenarioRunRequestWrapper& request);

} // namespace NAlice::NVideoCommon
