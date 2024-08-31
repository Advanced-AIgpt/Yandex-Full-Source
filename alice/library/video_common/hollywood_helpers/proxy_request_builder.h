#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/framework/core/request.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/library/video_common/defs.h>

namespace NAlice::NVideoCommon {

class TProxyRequestBuilder {
public:
    TProxyRequestBuilder(const NHollywood::TScenarioHandleContext& ctx, const NHollywood::TScenarioRunRequestWrapper& request)
        : RequestMeta(ctx.RequestMeta)
        , Logger(ctx.Ctx.Logger())
        , DisableOauth(request.ExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_DISABLE_OAUTH).Defined())
    {}
    TProxyRequestBuilder(const NHollywoodFw::TRunRequest& request)
        : RequestMeta(request.GetRequestMeta())
        , Logger(request.Debug().Logger())
        , DisableOauth(request.Flags().IsExperimentEnabled(NAlice::NVideoCommon::FLAG_VIDEO_DISABLE_OAUTH))
    {}

    void AddCgiParam(const TStringBuf key, const TStringBuf value);

    void SetEndpoint(const TString& endpoint);

    void SetScheme(const NAppHostHttp::THttpRequest_EScheme scheme);

    void AddHeader(const TStringBuf key, const TStringBuf value);

    NHollywood::THttpProxyRequest Build() const;

private:
    const NScenarios::TRequestMeta& RequestMeta;
    TRTLogger& Logger;
    bool DisableOauth;
    TCgiParameters Cgi;
    TVector<std::pair<TString, TString>> Headers;
    TMaybe<TString> Endpoint;
    TMaybe<NAppHostHttp::THttpRequest_EScheme> Scheme;
};

} // namespace NAlice::NVideoCommon
