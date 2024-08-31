#include "helper.h"

#include <alice/hollywood/library/framework/helpers/beggins/beggins.h>
#include <alice/hollywood/library/request/experiments.h>

#include <alice/library/network/headers.h>

#include <util/generic/variant.h>
#include <util/string/cast.h>

using namespace NAlice::NScenarios;
using namespace NAlice::NHollywood;

namespace NAlice::NHollywoodFw {

inline constexpr uint64_t VINS_HINT_BLUR_RATIO = 5;

const TString SHOW_ROUTE_INTENT = "personal_assistant.scenarios.show_route";

TMaybe<NAlice::TSemanticFrame> LookupShowRouteFrame(const TMaybe<NBg::NProto::TAliceResponseResult>& response) {
    if (!response.Defined()) {
        return Nothing();
    }
    const auto& parsedFrames = response->GetAliceParsedFrames();
    const int framesCount = parsedFrames.FramesSize();
    for (int i = 0; i < framesCount; ++i) {
        const auto& frame = parsedFrames.GetFrames(i);
        if (frame.GetName() == SHOW_ROUTE_INTENT && parsedFrames.GetConfidences(i) == 1.0) {
            return frame;
        }
    }
    return Nothing();
}

void EnrichRequestFromApphostItems(const TRunRequest& runRequest, TScenarioRunRequest& requestProto) {
    for (int dsId = 1; dsId < EDataSourceType_ARRAYSIZE; ++dsId) {
        if (!EDataSourceType_IsValid(dsId)) {
            continue;
        }
        if (const auto* ds = runRequest.GetDataSource(static_cast<NAlice::EDataSourceType>(dsId), /*logError*/ false)) {
            requestProto.MutableDataSources()->insert({dsId, *ds});
        }
    }

    const auto begginsResponse = GetBegginsResponse(runRequest);
    if (auto showRouteFrame = LookupShowRouteFrame(begginsResponse)) {
        LOG_INFO(runRequest.Debug().Logger()) << "Added intent from Beggins: " << *showRouteFrame;
        *requestProto.MutableInput()->AddSemanticFrames() = std::move(*showRouteFrame);
    }
}

TCgiParameters BuildCgiParameters(const NJson::TJsonValue& params) {
    TCgiParameters cgiParameters;
    if (const auto* srcrwr = params.GetValueByPath("srcrwr")) {
        for (const auto& [k, v]: srcrwr->GetMap()) {
            if (k.StartsWith("BASS")) {
                cgiParameters.InsertUnescaped("srcrwr", TStringBuilder() << k << ':' << v.GetString());
            }
        }
    }
    return cgiParameters;
}

TString ToPath(const ENodeType requestType) {
    switch(requestType) {
        case ENodeType::Run: {
            return "run";
        }
        case ENodeType::Continue: {
            return "continue";
        }
        case ENodeType::Commit: {
            return "commit";
        }
        case ENodeType::Apply: {
            return "apply";
        }
        default:
            return "";
    }
}

TString BuildHttpRequestPath(const ENodeType requestType, const TCgiParameters& cgiParameters) {
    auto path = TStringBuilder() << "/" << ToPath(requestType);
    if (auto cgiParametersStr = cgiParameters.Print()) {
        path << '?' << std::move(cgiParametersStr);
    }
    return path;
}

THttpProxyRequestBuilder CreateHttpRequestBuilder(const TRequest& request, const TCgiParameters& cgiParameters) {
    auto apphostInfo = request.GetApphostInfo();
    auto path = BuildHttpRequestPath(apphostInfo.NodeType, cgiParameters);
    return THttpProxyRequestBuilder(path, request.GetRequestMeta(), request.Debug().Logger(), apphostInfo.NodeName)
        .SetMethod(NAppHostHttp::THttpRequest::Post);
}

void AddHeaders(THttpProxyRequestBuilder& builder, const TRequest& request) {
    const auto uuid = request.Client().GetClientInfo().Uuid;
    const auto utterance = request.Input().GetUtterance();
    const auto randomSeed = request.GetRequestMeta().GetRandomSeed();

    const ui64 hint = NNetwork::GetVinsBalancerHint(utterance, uuid, randomSeed, VINS_HINT_BLUR_RATIO);
    builder.AddHeader(NNetwork::HEADER_X_VINS_REQUEST_HINT, ToString(hint));

    if (const auto userTicket = request.GetRequestMeta().GetUserTicket()) {
        builder.AddHeader(NNetwork::HEADER_X_YA_USER_TICKET, userTicket);
    } else {
        LOG_DEBUG(request.Debug().Logger()) << "no UserTicket";
    }

    if (const auto oauth = request.GetRequestMeta().GetOAuthToken()) {
        builder.AddHeader(NNetwork::HEADER_X_OAUTH_TOKEN, TStringBuilder() << TStringBuf("OAuth ") << oauth);
    } else {
        LOG_DEBUG(request.Debug().Logger()) << "no user OAuthToken";
    }

    builder.AddHeader(NNetwork::HEADER_ACCEPT, NContentTypes::APPLICATION_PROTOBUF);
}

void AddExpFlagRenderVinsNlgInHollywood(NScenarios::TScenarioBaseRequest& requestProto) {
    static const auto expFlagHwRenderVinsNlg = TString(EXP_HW_RENDER_VINS_NLG);
    AddExpFlag(expFlagHwRenderVinsNlg, requestProto);
}

void AddExpFlag(const TString& name, NScenarios::TScenarioBaseRequest& requestProto) {
    static const auto enabledExpValue = []() {
        auto result = ::google::protobuf::Value();
        result.set_string_value("1");
        return result;
    }();

    auto& experiments = *requestProto.MutableExperiments()->mutable_fields();
    experiments.insert({name, enabledExpValue});
}

} // namespace NAlice::NHollywood::NVins
