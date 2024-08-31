#include "http_request.h"

#include "application.h"
#include "http_handlers.h"
#include "ping_handler.h"
#include "rotate_logs_handler.h"
#include "sensors.h"
#include "version_handler.h"

#include <alice/bass/libs/logging/logger.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <kernel/reqid/reqid.h>
#include <util/system/hostname.h>

namespace NPersonalCards {

namespace {

constexpr TStringBuf FAKE_ROUTE_PATH = "/fake_route";

THttpHandlersMap CreateHttpHandlers() {
    THttpHandlersMap handlers;

    RegisterCardsHttpHandlers(&handlers);
    TRotateLogsHandler::RegisterHttpHandlers(&handlers);
    TVersionHandler::RegisterHttpHandlers(&handlers);
    TPingHandler::RegisterHttpHandlers(&handlers);

    return handlers;
}

bool CheckBalancerReqid(TStringBuf reqId) {
    TStringBuf first, second;
    reqId.Split('-', first, second);

    ui64 dummy;
    return TryFromString(first, dummy) && TryFromString(second, dummy);
}

TString GetReqId(const THttpHeaders& httpHeaders, const TString& reqidClass) {
    // Because we are looking for the only one specific header just iterate all of them.
    // If more than one header require, build the map.
    for (auto i : httpHeaders) {
        if (i.Name() == "X-Req-Id"sv && CheckBalancerReqid(i.Value())) {
            return TStringBuilder() << i.Value() << '-' << ReqIdHostSuffix() << '-' << reqidClass;
        }

    }
    return ReqIdGenerate(reqidClass.c_str());
}

/* Inheritance from 'THttpResponse' is not used for simplicity (for not to implement 'Out<>' function).
 * Also 'THttpResponse' has no virtual destructor.
 */
THttpResponse CreateHttpResponseAndCount(HttpCodes code, TStringBuf path) {
    if (code != HttpCodes::HTTP_NOT_FOUND) {
        NInfra::TRateSensor(SENSOR_GROUP, "request_result", {
            {"code", TStringBuilder() << code / 100 << "XX"},
            {"route", path}
        }).Inc();
    }

    return THttpResponse(code);
}

} // namespace

const THttpHandlersMap HttpRequestHandlers = CreateHttpHandlers();

bool TJsonHttpRequestHandler::DoHttpReply(const TParsedHttpFull& httpRequest,
                                           const TRequestReplier::TReplyParams& params) {
    TLogging::ReqId = GetReqId(params.Input.Headers(), GetReqIdClass());

    NJson::TJsonMap request;
    TString requestText = params.Input.ReadAll();
    if (!NJson::ReadJsonFastTree(requestText, &request, false /* throwOnError */)) {
        LOG(ERR) << "invalid JSON in '" << httpRequest.Path << "' text: " << requestText << Endl;

        static NInfra::TRateSensor rateSensor(SENSOR_GROUP, "http_request_badjson");
        rateSensor.Inc();

        params.Output << CreateHttpResponseAndCount(HTTP_BAD_REQUEST, httpRequest.Path).SetContent("Invalid json");
        params.Output.Flush();
        return true;
    }

    NJson::TJsonMap response;
    auto httpResponse = CreateHttpResponseAndCount(
        DoJsonReply(std::move(request), &response, httpRequest, params.Input.Headers()),
        httpRequest.Path
    );
    httpResponse.SetContentType("application/json");
    httpResponse.AddHeader("X-PCards-Host", HostName());
    params.Output << httpResponse << NJson::WriteJson(
        response,
        false, // formatOutput
        false, // sortkeys
        false  // validateUtf8
    );
    params.Output.Flush();
    return true;
}

bool THttpReplier::DoReply(const TReplyParams& params) {
    struct ParallelRequestCounter {
        ParallelRequestCounter()
            : StartTime(TInstant::Now())
            , Path(FAKE_ROUTE_PATH)
        {
            // Update only in constructor because update function just updates maximum
            TApplication::GetInstance()->UpdateMaxParallelHttpRequestCountInWindow(
                ++TApplication::GetInstance()->GetParallelHttpRequestCountRef()
            );
        }

        void SetPath(const TStringBuf path) {
            Path = path;
        }

        ~ParallelRequestCounter() {
            --TApplication::GetInstance()->GetParallelHttpRequestCountRef();

            if (TInstant::Now() - StartTime > TApplication::GetInstance()->GetRequestTimeLimit()) {
                NInfra::TRateSensor(SENSOR_GROUP, "request_time_limit_exceeded", {{"route", Path}}).Inc();
            }
        }

        TInstant StartTime;
        TString Path;
    } parallelRequestCounter;

    try {
        TParsedHttpFull httpRequest(params.Input.FirstLine());
        parallelRequestCounter.SetPath(httpRequest.Path);
        auto it = HttpRequestHandlers.find(httpRequest.Path);
        if (it == HttpRequestHandlers.end()) {
            LOG(ERR) << "path not found '" << httpRequest.Path << Endl;
            static NInfra::TRateSensor rateSensor(SENSOR_GROUP, "http_request_notfound");
            rateSensor.Inc();

            params.Output << CreateHttpResponseAndCount(HTTP_NOT_FOUND, httpRequest.Path);
            params.Output.Flush();
            return true;
        } else {
            NInfra::TRateSensor(SENSOR_GROUP, "http_request_count", {{"route", httpRequest.Path}}).Inc();
            THistogramTimer("http_request_time", {{"route", httpRequest.Path}});
            return it->second()->DoHttpReply(httpRequest, params);
        }
    } catch(...) {
        static NInfra::TRateSensor rateSensor(SENSOR_GROUP, "http_request_exception");
        rateSensor.Inc();
        TString message = CurrentExceptionMessage();
        LOG(ERR) << "uncaught exception: " << message << Endl;
        params.Output << CreateHttpResponseAndCount(HTTP_INTERNAL_SERVER_ERROR, FAKE_ROUTE_PATH) << message;
        params.Output.Flush();
        return true;
    }
}

} // namespace NPersonalCards
