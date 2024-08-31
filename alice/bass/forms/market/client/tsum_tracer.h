#pragma once

#include <alice/bass/libs/fetcher/request.h>

namespace NBASS::NMarket {

class TTSUMTracer {
public:
    enum class ERequestType {
        IN /* "IN" */,
        OUT /* "OUT" */,
        PROXY /* "PROXY" */
    };

    TTSUMTracer(ERequestType type = ERequestType::OUT, TStringBuf sourceModule = "", TStringBuf targetModule = "");

    TTSUMTracer& operator<<(const NHttpFetcher::TResponse& response);
    TTSUMTracer& operator<<(const NHttpFetcher::TRequest& request);
    TString ToString() const ;

private:
    ERequestType Type;
    TString SourceModule;
    TString SourceHost;
    TString TargetModule;
    TString TargetHost;
    TString DateTime;
    TString RequestId;
    TString RequestMethod;
    TString Protocol;
    TString HttpMethod;
    ui32 HttpCode;
    ui32 RetryNum;
    ui32 ResponseSize;
    ui64 DurationMs;
    TString ErrorCode;
    TString QueryParams;
};

inline IOutputStream& operator<<(IOutputStream& strm, const TTSUMTracer& tracer) {
    return strm << tracer.ToString();
}

}
