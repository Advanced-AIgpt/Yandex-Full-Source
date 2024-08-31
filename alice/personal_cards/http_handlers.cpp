#include "http_handlers.h"
#include "server.h"

#include <alice/bass/libs/logging/logger.h>

namespace NPersonalCards {

namespace {

constexpr TStringBuf HEADER_FORWARDED_FOR = "X-Forwarded-For";
constexpr TStringBuf HEADER_START_TIME = "X-Req-StartTime";
constexpr TStringBuf HEADER_TVM_SRV = "X-Ya-Service-Ticket";
constexpr TStringBuf HEADER_TVM_USR = "X-Ya-User-Ticket";

TString GetFirstHeader(const THttpHeaders& httpHeaders, TStringBuf header) {
    for (const auto& i : httpHeaders) {
        if (i.Name() == header)
            return i.Value();
    }
    return TString();
}

const TString REQID_CLASS = "pcards";

// Without TVM
template <TServerHandler F>
class TCardsRequestHandlerDeprecated : public TJsonHttpRequestHandler {
public:
    HttpCodes DoJsonReply(NJson::TJsonMap&& request, NJson::TJsonMap* response,
                           const TParsedHttpFull&, const THttpHeaders& headers) override {
        TRequestContext requestContext(std::move(request), GetFirstHeader(headers, HEADER_FORWARDED_FOR), GetFirstHeader(headers, HEADER_START_TIME));
        return (TApplication::GetInstance()->GetCardsServer()->*F)(requestContext, response) ? HTTP_OK : HTTP_BAD_REQUEST;
    }

protected:
    const TString& GetReqIdClass() const override { return REQID_CLASS; }
};

// With TVM
template <TServerHandler F>
class TCardsRequestHandler : public TJsonHttpRequestHandler {
public:
    HttpCodes DoJsonReply(NJson::TJsonMap&& request, NJson::TJsonMap* response,
                           const TParsedHttpFull&, const THttpHeaders& headers) override {
        const auto tvmClient = TApplication::GetInstance()->GetTvmClient();
        const auto serviceTicket = tvmClient->CheckServiceTicket(GetFirstHeader(headers, HEADER_TVM_SRV));
        if (!serviceTicket) {
            LOG(WARNING) << "Can't parse service ticket. Status: " << serviceTicket.GetStatus() << Endl;
            return HTTP_UNAUTHORIZED;
        }
        const auto& trustedServices = TApplication::GetInstance()->GetTrustedServices();
        if (!trustedServices.contains(serviceTicket.GetSrc())) {
            LOG(WARNING) << "Untrusted service ticket " << serviceTicket.DebugInfo() << Endl;
            return HTTP_UNAUTHORIZED;
        }

        const auto userTicket = tvmClient->CheckUserTicket(GetFirstHeader(headers, HEADER_TVM_USR));
        if (!userTicket) {
            LOG(WARNING) << "Can't parse user ticket. Status: " << userTicket.GetStatus() << Endl;
            return HTTP_UNAUTHORIZED;
        }
        if (!userTicket.GetDefaultUid()) {
            LOG(WARNING) << "User ticket has no default uid  " << userTicket.DebugInfo() << Endl;
            return HTTP_UNAUTHORIZED;
        }

        TRequestContext requestContext(
            std::move(request),
            GetFirstHeader(headers, HEADER_FORWARDED_FOR),
            GetFirstHeader(headers, HEADER_START_TIME),
            userTicket.GetDefaultUid()
        );
        return (TApplication::GetInstance()->GetCardsServer()->*F)(requestContext, response) ? HTTP_OK : HTTP_BAD_REQUEST;
    }

protected:
    const TString& GetReqIdClass() const override { return REQID_CLASS; }
};

template<bool UseTvm, TServerHandler F>
IHttpRequestHandler::TPtr CreateCardsRequestHandler() {
    if constexpr (UseTvm) {
        static IHttpRequestHandler::TPtr handler = new TCardsRequestHandler<F>;
        return handler;
    } else {
        static IHttpRequestHandler::TPtr handler = new TCardsRequestHandlerDeprecated<F>;
        return handler;
    }
}

using TServerRawHandler = bool (TServer::*)(const TParsedHttpFull&, const TRequestReplier::TReplyParams&);

template <TServerRawHandler F>
class TCardsRawRequestHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(const TParsedHttpFull& httpRequest, const TRequestReplier::TReplyParams& params) override {
        return (TApplication::GetInstance()->GetCardsServer()->*F)(httpRequest, params);
    }
};

} // namespace

void RegisterCardsHttpHandlers(THttpHandlersMap* handlers) {
    // With TVM
    handlers->insert(std::make_pair(
        TStringBuf("/getPushCards"), CreateCardsRequestHandler<true, &TServer::GetCards>));
    handlers->insert(std::make_pair(
        TStringBuf("/removePushCard"), CreateCardsRequestHandler<true, &TServer::DismissCard>));
    handlers->insert(std::make_pair(
        TStringBuf("/addPushCard"), CreateCardsRequestHandler<true, &TServer::AddPushCard>));

    // Without TVM (deprecated, DO NOT USE!!!)
    handlers->insert(std::make_pair(
        TStringBuf("/cards"), CreateCardsRequestHandler<false, &TServer::GetCards>));
    handlers->insert(std::make_pair(
        TStringBuf("/dismiss"), CreateCardsRequestHandler<false, &TServer::DismissCard>));
    handlers->insert(std::make_pair(
        TStringBuf("/addPushCards"), CreateCardsRequestHandler<false, &TServer::AddPushCard>));

    // Sensors
    handlers->insert(std::make_pair(
        TStringBuf("/sensors"), CreateCardsRequestHandler<false, &TServer::Sensors>));


    // Deprecated, unsupported, useless handlers
    static const THashSet<TString> deprecatedRoutes = {
        "/addCards",
        "/personalCards",
        "/updateCards",
        "/userContext",
        "/loadCards",
        "/updateCardsData",
        "/userInfo",
        "/getStoriesInfo",
        "/addStoriesWatch"
    };
    for (const auto& route : deprecatedRoutes) {
        handlers->insert(
            std::make_pair(
                route,
                CreateCardsRequestHandler<false, &TServer::LogDeprecatedRequestAndReturnNothing>
            )
        );
    }
}

} // namespace NPersonalCards
