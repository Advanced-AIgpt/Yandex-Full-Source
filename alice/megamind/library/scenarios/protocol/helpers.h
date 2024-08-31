#pragma once

#include <alice/megamind/library/context/context.h>

#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>

namespace NAlice {

constexpr TStringBuf TAG_RUN = "run";
constexpr TStringBuf TAG_APPLY = "apply";
constexpr TStringBuf TAG_COMMIT = "commit";
constexpr TStringBuf TAG_CONTINUE = "continue";

constexpr TStringBuf HTTP_PROXY_REQUEST_SUFFIX = "http_proxy_request";
constexpr TStringBuf HTTP_PROXY_RESPONSE_SUFFIX = "http_proxy_response";
constexpr TStringBuf PURE_REQUEST_SUFFIX = "pure_request";
constexpr TStringBuf PURE_RESPONSE_SUFFIX = "pure_response";

constexpr TStringBuf BASE_REQUEST_SUFFIX = "_base_request";
constexpr TStringBuf REQUEST_INPUT_SUFFIX = "_input";

struct TAppHostItemNames {
    TAppHostItemNames(const TString& scenarioName, const TStringBuf itemRequestSuffix, const TStringBuf itemResponseSuffix);

    const TString ApplyRequest;
    const TString ApplyResponse;
    const TString CommitRequest;
    const TString CommitResponse;
    const TString ContinueRequest;
    const TString ContinueResponse;
    const TString RunRequest;
    const TString RunResponse;
    const TString RequestMeta;
    const TString BaseRequest;
    const TString RequestInput;
};

template <typename TProto, typename TScenarioRequest>
[[nodiscard]] TStatus FillHttpProxyRequest(const IContext& ctx,
                                           const TProto& proto,
                                           TScenarioRequest& request,
                                           bool enableOAuth) {
    TString data;
    if (!proto.SerializeToString(&data)) {
        return TError{TError::EType::Parse} << "Failed to serialize proto " << proto.GetTypeName() << " to string";
    }
    if (const auto& ticket = ctx.Responses().BlackBoxResponse().GetUserTicket(); !ticket.Empty()) {
        request.AddHeader(NNetwork::HEADER_X_YA_USER_TICKET, ticket);
    } else {
        LOG_DEBUG(ctx.Logger()) << "No user ticket";
    }

    if (enableOAuth) {
        if (const auto& oAuthToken = ctx.AuthToken(); oAuthToken.Defined()) {
            request.AddHeader(NNetwork::HEADER_X_OAUTH_TOKEN, *oAuthToken);
        } else {
            LOG_DEBUG(ctx.Logger()) << "No user OAuthToken";
        }
    }

    if (const auto& appInfo = ctx.AppInfo(); appInfo.Defined()) {
        request.AddHeader(NNetwork::HEADER_X_YANDEX_APP_INFO, *appInfo);
    }

    request.AddHeader(NNetwork::HEADER_ACCEPT, NContentTypes::APPLICATION_PROTOBUF);
    request.SetBody(data, NHttpMethods::POST);
    request.SetContentType(NContentTypes::APPLICATION_PROTOBUF);

    return Success();
}

} // namespace NAlice
