#include <alice/cachalot/library/request.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/utils.h>

#include <alice/cachalot/events/cachalot.ev.pb.h>

#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/http_headers.h>
#include <library/cpp/neh/http2.h>

#include <contrib/libs/protobuf/src/google/protobuf/util/json_util.h>
#include <contrib/libs/protobuf/src/google/protobuf/json_util.h>

#include <util/generic/guid.h>


namespace NCachalot {


TRequest::TRequest(TRequestMetrics* requestMetrics)
    : RequestMetrics(requestMetrics)
    , RequestFormat(ERequestFormat::Unknown)
    , Status(EResponseStatus::PENDING)
    , ArrivalTime(TInstant::Now())
    , ReqId(CreateGuidAsString())
    , LogFrame(nullptr)
    , FinishPromise(NThreading::NewPromise<void>())
{
    Y_ASSERT(requestMetrics != nullptr);
}

TRequest::TRequest(const NNeh::IRequestRef& req, TRequestMetrics* requestMetrics)
    : TRequest(requestMetrics)
{
    NNeh::IHttpRequest* http = dynamic_cast<NNeh::IHttpRequest*>(req.Get());
    Y_ASSERT(http != nullptr);

    ArrivalTime = req->ArrivalTime();

    //  Extracting rtlog token
    if (const auto rtLogToken = NAlice::NCuttlefish::TryGetRtLogTokenFromHttpRequestHeaders(http->Headers())) {
        LogFrame = MakeIntrusive<TChronicler>(rtLogToken.GetRef());
    } else if (!LogFrame) {
        LogFrame = MakeIntrusive<TChronicler>();
    }

    //  Extracting req-id
    if (const THttpInputHeader* reqId = http->Headers().FindHeader("X-Ya-ReqId")) {
        ReqId = reqId->Value();
    } else {
        ReqId = CreateGuidAsString();
    }

    //  Extracting content type
    const THttpInputHeader* contentType = http->Headers().FindHeader("Content-Type");
    if (contentType) {
        LogFrame->LogEvent(NEvClass::RequestStarted(ReqId, contentType->Value()));
        if (contentType->Value() == "application/json") {
            RequestFormat = ERequestFormat::Json;
        } else if (contentType->Value() == "application/protobuf") {
            RequestFormat = ERequestFormat::Protobuf;
        } else if (contentType->Value() == "application/octet-stream") {
            RequestFormat = ERequestFormat::Protobuf;
        } else {
            LogFrame->LogEvent(NEvClass::HttpStatusUnknownContentType());
            RequestFormat = ERequestFormat::Unknown;
        }
    }


    //  Try to parse request body
    if (RequestFormat == ERequestFormat::Protobuf) {
        if (!Request.ParseFromString(TString(req->Data()))) {
            SetError(EResponseStatus::BAD_REQUEST, "Bad Request (Malformed Protobuf)");
            RequestMetrics->OnBadData(ArrivalTime);
        }
    } else if (RequestFormat == ERequestFormat::Json) {
        const auto parsingStatus = google::protobuf::util::JsonStringToMessage(TString(req->Data()), &Request);
        if (!parsingStatus.ok()) {
            SetError(EResponseStatus::BAD_REQUEST, "Bad Request (Malformed JSON)");
            RequestMetrics->OnBadData(ArrivalTime);
        }
    } else {
        SetError(EResponseStatus::BAD_REQUEST, "Bad Request (Invalid Content-Type)");
        RequestMetrics->OnBadData(ArrivalTime);
    }
}

TRequest::TRequest(NAppHost::TServiceContextPtr ctx, TRequestMetrics* requestMetrics)
    : TRequest(requestMetrics)
{
    Y_ASSERT(ctx != nullptr);

    RequestFormat = ERequestFormat::Protobuf;
    ArrivalTime = ctx->GetProcessingStartTime();
    ReqId = ctx->GetRequestID().AsGuidString();
    LogFrame = MakeLoggerWithRtLogTokenFromAppHostContext(*ctx);
}

TRequest::~TRequest() {
    FinishPromise.SetValue();
}

void TRequest::ReplyTo(const NNeh::IRequestRef& req) {
    Response.MutableStats()->SetServeTime(MillisecondsSince(ArrivalTime));
    LogFinalStatus();

    TString headers;
    TString payload;

    if (RequestFormat == ERequestFormat::Protobuf) {
        if (!Response.SerializeToString(&payload)) {
            req->SendError(NNeh::IRequest::InternalError, "Internal Error (Proto Serialize)");
            RequestMetrics->OnInternalError(ArrivalTime);
            LogFrame->LogEvent(NEvClass::HttpStatusFailedToSerializeProtoResponse());
            return;
        }
        headers = "\r\nContent-Type: application/protobuf";
    } else if (RequestFormat == ERequestFormat::Json) {
        const auto parsingStatus = google::protobuf::util::MessageToJsonString(Response, &payload);
        if (!parsingStatus.ok()) {
            req->SendError(NNeh::IRequest::InternalError, "Internal Error (JSON Serialize)");
            RequestMetrics->OnInternalError(ArrivalTime);
            LogFrame->LogEvent(NEvClass::HttpStatusFailedToSerializeJsonResponse());
            return;
        }
        headers = "\r\nContent-Type: application/json";
    } else {
        req->SendError(TStatus(Status), Response.GetStatusMessage());
        LogFrame->LogEvent(NEvClass::RequestFinished(ReqId, Status));
        return;
    }

    LogFrame->LogEvent(NEvClass::SendingReply());

    NNeh::IHttpRequest *r = dynamic_cast<NNeh::IHttpRequest*>(req.Get());
    NNeh::TData data(payload.data(), payload.data() + payload.size());
    r->SendReply(data, headers, Status);

    LogFrame->LogEvent(NEvClass::RequestFinished(ReqId, Status));
}

void TRequest::ReplyTo(NAppHost::TServiceContextPtr ctx, TString requestKey) {
    Response.MutableStats()->SetServeTime(MillisecondsSince(ArrivalTime));
    LogFinalStatus();

    if (bool(TStatus(Status))) {
        ReplyToApphostContextOnSuccess(ctx, TStringBuf(requestKey));
    } else {
        ReplyToApphostContextOnError(ctx, TStringBuf(requestKey));
    }
    LogFrame->LogEvent(NEvClass::RequestFinished(ReqId, Status));
}

void TRequest::AddBackendStats(const TString& backend, EResponseStatus status, const TStorageStats& stats) {
    ::NCachalotProtocol::TRequestStats *s = Response.MutableStats();
    ::NCachalotProtocol::TBackendStats *b = s->AddBackendStats();
    b->SetStatus(status);
    b->SetBackend(backend);
    b->SetSchedulingTime(stats.SchedulingTime);
    b->SetFetchingTime(stats.FetchingTime);
}

void TRequest::LogFinalStatus() const {
    switch (Status) {
        case EResponseStatus::OK:
        case EResponseStatus::CREATED:
            RequestMetrics->OnOk(ArrivalTime);
            LogFrame->LogEvent(NEvClass::HttpStatusOk());
            break;
        case EResponseStatus::NO_CONTENT:
        case EResponseStatus::NOT_FOUND:
            RequestMetrics->OnNotFound(ArrivalTime);
            LogFrame->LogEvent(NEvClass::HttpStatusNotFound());
            break;
        case EResponseStatus::BAD_GATEWAY:
        case EResponseStatus::GATEWAY_TIMEOUT:
            RequestMetrics->OnFailed(ArrivalTime);
            LogFrame->LogEvent(NEvClass::HttpStatusBadGateway());
            break;
        case EResponseStatus::TOO_MANY_REQUESTS:
            RequestMetrics->OnQueueFull(ArrivalTime);
            LogFrame->LogEvent(NEvClass::HttpStatusQueueFull());
            break;
        case EResponseStatus::SERVICE_UNAVAILABLE:
            RequestMetrics->OnServiceUnavailable(ArrivalTime);
            LogFrame->LogEvent(NEvClass::HttpStatusServiceUnavailable());
            break;
        case EResponseStatus::INTERNAL_ERROR:
        case EResponseStatus::NOT_IMPLEMENTED:
        case EResponseStatus::PENDING:
        case EResponseStatus::QUERY_PREPARE_FAILED:
        case EResponseStatus::QUERY_EXECUTE_FAILED:
            RequestMetrics->OnInternalError(ArrivalTime);
            LogFrame->LogEvent(NEvClass::HttpStatusInternalError());
            break;
        default:
            RequestMetrics->OnUnexpected();
            LogFrame->LogEvent(NEvClass::HttpStatusUnexpected());
            break;
    }
}

void TRequest::ReplyToApphostContextOnSuccess(NAppHost::TServiceContextPtr ctx, TStringBuf) {
    ReplyToApphostContextOnSuccess(ctx);
}

void TRequest::ReplyToApphostContextOnSuccess(NAppHost::TServiceContextPtr ctx) {
    Y_UNUSED(ctx);
    Y_ENSURE(false, "Not Implemented");
}

void TRequest::ReplyToApphostContextOnError(NAppHost::TServiceContextPtr ctx, TStringBuf) {
    ReplyToApphostContextOnError(ctx);
}

void TRequest::ReplyToApphostContextOnError(NAppHost::TServiceContextPtr ctx) {
    Y_UNUSED(ctx);
    Y_ENSURE(false, "Not Implemented");
}


}   // namespace NCachalot
