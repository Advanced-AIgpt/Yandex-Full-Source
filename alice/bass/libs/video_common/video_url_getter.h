#pragma once

#include <alice/bass/libs/fetcher/neh.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/system/types.h>

namespace NVideoCommon {
class TVideoUrlGetter {
public:
    struct TException : public yexception {};

    struct TParams {
        TDuration RetryPeriod = TDuration::MilliSeconds(5000);
        TDuration Timeout = TDuration::MilliSeconds(2000);
        ui32 MaxAttempts = 2;
        bool UseZora = false;
    };

    struct TRequest {
        TMaybe<TString> ProviderName;
        TMaybe<TString> ProviderItemId;
        TMaybe<TString> Type;
        TMaybe<TString> TvShowItemId;
        TMaybe<TString> KinopoiskId;
    };

public:
    explicit TVideoUrlGetter(const TParams& params);

    TMaybe<TString> Get(const TRequest& request) const;

private:
    NHttpFetcher::TResponse::TRef MakeRequest(TStringBuf url) const;

private:
    NHttpFetcher::TRequestOptions Options;
    bool UseZora;
};
} // namespace NVideoCommon
