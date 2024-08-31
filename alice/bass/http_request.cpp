#include "http_request.h"

#include "forms/request.h"
#include "ping_handler.h"
#include "push_handler.h"
#include "reopenlogs_handler.h"
#include "test_users.h"
#include "version_handler.h"

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/library/network/headers.h>

#include <alice/rtlog/client/client.h>

#include <kernel/reqid/reqid.h>

#include <library/cpp/unistat/raii.h>

using namespace std::chrono;

namespace {

class TRTLogScope {
public:
    TRTLogScope(NRTLog::TClient& client, const THttpInput& httpInput)
        : RequestLoggerPtr(nullptr)
    {
        const auto& rtlogToken = GetRTLogToken(httpInput.Headers());
        if (!rtlogToken.empty()) {
            RequestLoggerPtr = client.CreateRequestLogger(rtlogToken);
            TLogging::RequestLogger = RequestLoggerPtr.get();
        }
    }

    ~TRTLogScope() {
        if (RequestLoggerPtr) {
            RequestLoggerPtr->Finish();
            TLogging::RequestLogger = nullptr;
        }
    }

private:
    NRTLog::TRequestLoggerPtr RequestLoggerPtr;

private:
    TString GetRTLogToken(const THttpHeaders& headers) {
        const auto* reqidHeader = headers.FindHeader(NAlice::NNetwork::HEADER_X_APPHOST_REQUEST_REQID);
        const auto* ruidHeader = headers.FindHeader(NAlice::NNetwork::HEADER_X_APPHOST_REQUEST_RUID);

        if (reqidHeader != nullptr && ruidHeader != nullptr) {
            return TStringBuilder{} << reqidHeader->Value() << '-' << ruidHeader->Value();
        }

        const auto* rtlogTokenHeader = headers.FindHeader(NAlice::NNetwork::HEADER_X_RTLOG_TOKEN);
        if (rtlogTokenHeader == nullptr) {
            return "";
        }
        return rtlogTokenHeader->Value();
    }
};

} // namespace

THttpHandlersMap::THttpHandlersMap(NBASS::TGlobalContextPtr globalCtx) {
    TVersionHandler::RegisterHttpHandlers(this);
    TVersionJsonHandler::RegisterHttpHandlers(this);
    NBASS::TPingHandler::RegisterHttpHandlers(this);
    NBASS::TReopenLogsHandler::RegisterHttpHandlers(this);
    NBASS::TPushRequestHandler::RegisterHttpHandlers(this);
    NBASS::TBassRequestHandler::RegisterHttpHandlers(this, globalCtx);
    NBASS::TTestUsersRequestHandler::RegisterHttpHandlers(this, globalCtx);
}

const THttpHandlersMap::THandlerFactory* THttpHandlersMap::ByPath(TStringBuf path) const {
    return Handlers.FindPtr(path);
}

void THttpHandlersMap::Add(TStringBuf path, THttpHandlersMap::THandlerFactory factory) {
    Handlers.emplace(TString{path}, factory);
}

TString GetReqId(const THttpHeaders& httpHeaders, const TString& reqidClass) {
    // Because we are looking for the only one specific header just iterate all of them.
    // If more than one header require, build the map.
    for (const auto& header : httpHeaders) {
        if (header.Name() == TStringBuf("X-Req-Id") && CheckBalancerReqid(header.Value())) {
            return TStringBuilder() << header.Value() << '-' << ReqIdHostSuffix() << '-' << reqidClass;
        }

    }
    return ReqIdGenerate(reqidClass.c_str());
}

bool CheckBalancerReqid(TStringBuf reqId) {
    TStringBuf first, second;
    reqId.Split('-', first, second);

    ui64 dummy;
    return TryFromString(first, dummy) && TryFromString(second, dummy);
}

namespace {

void TryToSetReqIdFromHttpHeaders(const THttpHeaders& httpHeaders, const TString& reqidClass) {
    const TString reqid = GetReqId(httpHeaders, reqidClass);
    TLogging::BassReqId = reqid;
    // Make ReqId equal until we have VINSReqId from Context.Meta()
    // NB: We believe that at this point hypothesis_number is -1
    TLogging::ReqInfo.Get().Update(reqid);
}

} // namespace

bool TLoggingHttpRequestHandler::DoHttpReply(NBASS::TGlobalContextPtr globalCtx,
                                             const TParsedHttpFull& httpRequest,
                                             const TRequestReplier::TReplyParams& params)
{
    TRTLogScope rtlogScope(globalCtx->RTLogClient(), params.Input);

    const THttpHeaders& httpHeaders = params.Input.Headers();
    TryToSetReqIdFromHttpHeaders(httpHeaders, GetReqIdClass());

    TString requestText = params.Input.ReadAll();
    THttpResponse httpResponse = DoTextReply(globalCtx, requestText, httpRequest, httpHeaders);
    httpResponse.OutTo(params.Output);
    params.Output.Flush();
    return true;
}

bool TJsonHttpRequestHandler::DoHttpReply(NBASS::TGlobalContextPtr globalCtx,
                                           const TParsedHttpFull& httpRequest,
                                           const TRequestReplier::TReplyParams& params)
{
    TRTLogScope rtlogScope(globalCtx->RTLogClient(), params.Input);
    TryToSetReqIdFromHttpHeaders(params.Input.Headers(), GetReqIdClass());

    NSc::TValue request;
    TString requestText = params.Input.ReadAll();
    if (!NSc::TValue::FromJson(request, requestText) || !request.IsDict()) {
        LOG(ERR) << "invalid JSON in '" << httpRequest.Path << "' text: " << requestText << Endl;
        Y_STATS_INC_COUNTER("http_request_badjson");
        params.Output << THttpResponse(HTTP_BAD_REQUEST).SetContent("Invalid json");
        return true;
    }

    NSc::TValue response;
    THttpResponse httpResponse(DoJsonReply(globalCtx, request, httpRequest, params.Input.Headers(), &response));
    httpResponse.SetContentType("application/json; charset=utf-8");
    params.Output << httpResponse;
    response.ToJson(params.Output);
    params.Output.Flush();
    return true;
}

bool TMegamindProtocolHttpRequestHandler::DoHttpReply(NBASS::TGlobalContextPtr globalCtx,
                                                      const TParsedHttpFull& httpRequest,
                                                      const TRequestReplier::TReplyParams& params) {
    TRTLogScope rtlogScope(globalCtx->RTLogClient(), params.Input);
    TryToSetReqIdFromHttpHeaders(params.Input.Headers(), GetReqIdClass());

    // TODO Catch errors
    TString requestText = params.Input.ReadAll();
    TString response;
    THttpResponse httpResponse(DoJsonReply(globalCtx, requestText, httpRequest, params.Input.Headers(), response));

    httpResponse.SetContentType("application/protobuf");
    params.Output << httpResponse;
    params.Output << response;
    params.Output.Flush();
    return true;
}

THttpReplier::THttpReplier(NBASS::TGlobalContextPtr globalCtx, const THttpHandlersMap& handlers)
    : Handlers(handlers)
    , ConstructedAt(steady_clock::now())
    , GlobalCtx(globalCtx)
{
}

bool THttpReplier::DoReply(const TReplyParams& params) {
    {
        NMonitoring::THistogram& waitInQueue = NMonitoring::GetHistogram("http_request_wait_in_queue");
        waitInQueue.Record(duration_cast<milliseconds>(steady_clock::now() - ConstructedAt).count());
    }

    // Count number of parallel HTTP requests i.e. number of used threads.
    static NMonitoring::TIntGauge* counter =
        NMonitoring::GetSensors().IntGauge({{TStringBuf("sensor"), TStringBuf("http_request_parallel")}});
    struct ParallelRequestCounter {
        ParallelRequestCounter() {
            counter->Inc();
        }

        ~ParallelRequestCounter() {
            counter->Dec();
        }
    } parallelRequestCounter;

    try {
        TParsedHttpFull httpRequest(params.Input.FirstLine());
        const THttpHandlersMap::THandlerFactory* handler = Handlers.ByPath(httpRequest.Path);
        if (!handler) {
            LOG(ERR) << "path not found '" << httpRequest.Path << "'" << Endl;
            Y_STATS_INC_COUNTER("http_request_notfound");
            params.Output << THttpResponse(HTTP_NOT_FOUND);
            params.Output.Flush();
            return true;
        }

        const TStringBuf requestType = TStringBuf(httpRequest.Path).Skip(1);
        TString counterName = TString("http_request_") + requestType;
        Y_STATS_SCOPE_HISTOGRAM(counterName);
        Y_STATS_INC_COUNTER(counterName);

        TMaybe<TUnistatTimer> scope;
        if (requestType == TStringBuf("vins") || requestType == TStringBuf("setup")) {
            scope.ConstructInPlace(TUnistat::Instance(), counterName);
        }
        return (*handler)()->DoHttpReply(GlobalCtx, httpRequest, params);
    } catch(...) {
        Y_STATS_INC_COUNTER("http_request_exception");
        TString message = CurrentExceptionMessage();
        LOG(ERR) << "uncaught exception: " << message << Endl;
        params.Output << THttpResponse(HTTP_INTERNAL_SERVER_ERROR) << message;
        params.Output.Flush();
        return true;
    }
}
