#include "apphost_http_requester.h"

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/library/network/common.h>
#include <library/cpp/uri/uri.h>
#include <apphost/lib/common/constants.h>

#include <google/protobuf/struct.pb.h>

#include <util/digest/sequence.h>
#include <util/string/join.h>

namespace NAlice::NHollywood {

namespace {

const TString HTTP_REQUEST_TYPE_BASE = "http_request_";
const TString HTTP_RESPONSE_TYPE_BASE = "http_response_";
const TString RTLOG_TOKEN_KEY_TYPE_BASE = "rtlog_token_";

const TString HTTP_PROXY_SOURCE_NAME_BASE = "HTTP_PROXY";
const TString MULTI_REQUEST_TYPE_BASE = "multi_request_";

TMaybe<TString> RetireHttpResponseMaybe(
    const NAppHost::IServiceContext& serviceCtx,
    TRTLogger& logger,
    TStringBuf responseKey,
    TStringBuf tokenKey,
    bool logBody = true
) {
    Y_UNUSED(tokenKey);
    const TBassRequestRTLogToken requestRTLogToken;

    auto maybeResponse = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(serviceCtx, responseKey);

    return RetireResponseMaybe(std::move(maybeResponse), requestRTLogToken, logger, logBody,
        /* throwOnFailure = */ false);
}

class TAppHostUrlRequester : public IHttpRequester {
public:
    TAppHostUrlRequester(
        NAppHost::IServiceContext& serviceCtx,
        const NScenarios::TRequestMeta& requestMeta,
        const NJson::TJsonValue& apphostParams,
        TRTLogger& logger,
        const TString& typePrefix,
        const TString& nodePrefix
    )
        : ServiceCtx(serviceCtx)
        , RequestMeta(requestMeta)
        , Logger(logger)
        , ApphostParams(apphostParams)
        , TypePrefix(typePrefix)
        , NodePrefix(nodePrefix)
    {
    }

    IHttpRequester& Add(
        const TString& requestId,
        EMethod method,
        const TString& url,
        const TString& body,
        const THashMap<TString, TString>& headers = {}
    ) override {
        const TMaybe<TString> response = RetireHttpResponseMaybe(ServiceCtx, Logger,
            TypePrefix + HTTP_RESPONSE_TYPE_BASE + requestId, TypePrefix + RTLOG_TOKEN_KEY_TYPE_BASE + requestId);
        if (response.Defined()) {
            LOG_INFO(Logger) << "Got response for '" << requestId << "'";
            RequestIdToResponse[requestId] = *response;
        } else {
            LOG_INFO(Logger) << "Preparing request for '" << requestId << "'";
            RequestIdToParams[requestId] = TRequestParams{method, url, body, {}};
            for (const auto& [name, value] : headers) {
                RequestIdToParams[requestId].Headers.AddHeader(name, value);
            }
        }
        return *this;
    }

    IHttpRequester& Start() override {
        Y_ASSERT(RequestIdToParams.empty() || RequestIdToResponse.empty());
        TVector<TString> requestIdList;
        for (const auto& [requestId, params] : RequestIdToParams) {
            NUri::TUri uri;
            if (!NNetwork::TryParseUri(params.Url, uri)) {
                ythrow yexception() << "Failed to parse uri" << params.Url;
            }

            const TString backendAddress = TStringBuilder{}
                << uri.GetField(NUri::TUri::TField::FieldScheme) << "://"
                << uri.GetField(NUri::TUri::TField::FieldHost)
                << ':' << uri.GetPort();
            NJson::TJsonValue subgraphAppHostParams = ApphostParams;
            subgraphAppHostParams["srcrwr"][NodePrefix + HTTP_PROXY_SOURCE_NAME_BASE] = backendAddress;
            ServiceCtx.AddItem(std::move(subgraphAppHostParams), TypePrefix + NAppHost::APP_HOST_PARAMS_TYPE + "_" + requestId);

            THttpProxyRequest request = PrepareHttpRequest(
                /* path= */ TStringBuilder{} << uri.GetField(NUri::TUri::TField::FieldPath) << '?' << uri.GetField(NUri::TField::FieldQuery),
                /* meta= */ RequestMeta,
                /* logger= */ Logger,
                /* name= */ requestId,
                /* body= */ params.Body,
                /* method= */ (params.Method == IHttpRequester::EMethod::Get)
                    ? NAppHostHttp::THttpRequest::Get
                    : NAppHostHttp::THttpRequest::Post,
                /* headers= */ params.Headers
            );
            if (uri.GetScheme() == NUri::TScheme::SchemeHTTPS) {
                request.Request.SetScheme(NAppHostHttp::THttpRequest::Https);
            }
            ServiceCtx.AddProtobufItem(request.Request, TypePrefix + HTTP_REQUEST_TYPE_BASE + requestId);
            ServiceCtx.AddProtobufItem(request.RTLogToken, TypePrefix + RTLOG_TOKEN_KEY_TYPE_BASE + requestId);
            requestIdList.push_back(requestId);
        }
        for (const auto& [requestId, _] : RequestIdToResponse) {
            requestIdList.push_back(requestId);
        }
        // NOTE(the0): Maybe it's not necessary to throw an exception below when `requestIdList` is empty.
        // However, such an optimization may lead to apphost graph complexity increase affecting it's readability.
        const TString multiRequestType = TypePrefix + MULTI_REQUEST_TYPE_BASE + ToString(TRangeHash()(requestIdList));
        if (!ServiceCtx.HasProtobufItem(multiRequestType)) {
            ServiceCtx.AddProtobufItem(::google::protobuf::Struct{}, multiRequestType);
            ythrow TAppHostNodeExecutionBreakException() << "Breaking node processing on http requests: [" << JoinSeq(", ", requestIdList) << "]";
        }
        return *this;
    }

    TString Fetch(const TString& requestId) override {
        if (const auto* result = RequestIdToResponse.FindPtr(requestId)) {
            return *result;
        }
        ythrow yexception() << "Failed to fetch '" << requestId << "'";
    }

private:
    struct TRequestParams {
        EMethod Method = EMethod::Get;
        TString Url;
        TString Body;
        THttpHeaders Headers;
    };

    NAppHost::IServiceContext& ServiceCtx;
    const NScenarios::TRequestMeta& RequestMeta;
    TRTLogger& Logger;
    const NJson::TJsonValue& ApphostParams;
    THashMap<TString, TRequestParams> RequestIdToParams;
    THashMap<TString, TString> RequestIdToResponse;
    TString TypePrefix;
    TString NodePrefix;
};

} // namespace

THolder<IHttpRequester> MakeApphostHttpRequester(NAppHost::IServiceContext& serviceCtx,
    const NScenarios::TRequestMeta& requestMeta, const NJson::TJsonValue& apphostParams, TRTLogger& logger,
    const TString& typePrefix, const TString& nodePrefix)
{
    return MakeHolder<TAppHostUrlRequester>(serviceCtx, requestMeta, apphostParams, logger, typePrefix, nodePrefix);
}

} // namespace NAlice::NHollywood
