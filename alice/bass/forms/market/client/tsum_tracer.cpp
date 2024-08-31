#include "tsum_tracer.h"

#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>

#include <util/string/builder.h>

namespace NBASS::NMarket {

TTSUMTracer::TTSUMTracer(ERequestType type, TStringBuf sourceModule, TStringBuf targetModule)
    : Type(type)
    , SourceModule(sourceModule)
    , TargetModule(targetModule)
    , HttpCode(0)
    , RetryNum(1)
    , ResponseSize(0)
    , DurationMs(0)
{
}

TTSUMTracer& TTSUMTracer::operator<<(const NHttpFetcher::TResponse& response)
{
    auto header = response.Headers.FindHeader(NAlice::NNetwork::HEADER_X_MARKET_REQ_ID);
    DateTime = TInstant::Now().ToStringLocal();
    RequestId = ((header != nullptr) ? header->Value() : "");
    SourceHost = "";
    HttpCode = response.Code;
    DurationMs = response.Duration.MilliSeconds();
    ErrorCode = response.Result != NHttpFetcher::TResponse::EResult::Ok ? ::ToString(response.Result) : "";
    ResponseSize = response.Data.size();
    return *this;
}

TTSUMTracer& TTSUMTracer::operator<<(const NHttpFetcher::TRequest& request)
{
    NUri::TUri uri = NAlice::NNetwork::ParseUri(request.Url());
    TStringBuilder paramsBuilder;
    paramsBuilder << uri.GetField(NUri::TField::FieldPath) << "?" << uri.GetField(NUri::TField::FieldQuery);
    QueryParams = paramsBuilder;
    TargetHost = uri.GetHost();
    RequestMethod = uri.GetField(NUri::TField::FieldPath);
    RetryNum = 1;
    Protocol = uri.GetField(NUri::TField::FieldScheme);
    HttpMethod = request.GetMethod();
    return *this;
}

TString TTSUMTracer::ToString() const
{
    TStringStream out;
    out << "tskv"
        << "\ttskv_format=trace-log"
        << "\ttype=" << Type
        << "\tdate=" << DateTime
        << "\trequest_id=" << RequestId
        << "\tsource_module=" << SourceModule
        << "\tsource_host=" << SourceHost
        << "\ttarget_module=" << TargetModule
        << "\ttarget_host=" << TargetHost
        << "\trequest_method=" << RequestMethod
        << "\thttp_code=" << HttpCode
        << "\tretry_num=" << RetryNum
        << "\tduration_ms=" << DurationMs
        << "\terror_code=" << ErrorCode
        << "\tprotocol=" << Protocol
        << "\thttp_method=" << HttpMethod
        << "\tresponse_size_bytes=" << ResponseSize
        << "\tquery_params=" << QueryParams
        << "\n";
    return out.Str();
}

}
