#include "neh.h"

#include "fwd.h"
#include "neh_detail.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/neh.h>

#include <util/string/builder.h>

namespace NHttpFetcher {
namespace {

class THttpCodeStatusCallback : public NHttpFetcher::IStatusCallback {
public:
    NHttpFetcher::TFetchStatus OnResponse(const NHttpFetcher::TResponse& response) override {
        if (IsUserError(response.Code) && response.Code != 407) { //XXX: 407 is a temporary hack over Zora-related issues
            return NHttpFetcher::TFetchStatus::Failure(response.Code);
        }
        if (response.IsError()) {
            return NHttpFetcher::TFetchStatus::Error(response.Code);
        }
        return NHttpFetcher::TFetchStatus::Success();
    }
};

class TNehMultiRequest : public IMultiRequest {
public:
    TNehMultiRequest(IEventLoggerPtr logger, bool weak, NHttpFetcher::IRequestEventListener* listener)
        : MultiFetcher(new TNehMultiFetcher(logger, listener))
        , Weak(weak)
    {
    }

    THolder<TRequest> AddRequest(const NUri::TUri& uri, const TRequestOptions& options) override {
        TRequestOptions patchedOptions = options;
        if (Weak) {
            patchedOptions.IsRequired = false;
        }
        return AttachedNehRequest(uri, patchedOptions, MultiFetcher);
    }

    void WaitAll(TInstant deadline) override {
        MultiFetcher->WaitAll(deadline);
    }

private:
    TNehMultiFetcher::TRef MultiFetcher;
    const bool Weak;
};

} // namespace anonymous

const TString TFetchStatus::HTTP_0 = "0";
const TString TFetchStatus::HTTP_1XX = "1xx";
const TString TFetchStatus::HTTP_2XX = "2xx";
const TString TFetchStatus::HTTP_3XX = "3xx";
const TString TFetchStatus::HTTP_4XX = "4xx";
const TString TFetchStatus::HTTP_5XX = "5xx";

TFetchStatus TFetchStatus::Success() {
    return {EStatus::StatusSuccess, TFetchStatus::HTTP_2XX};
}

TFetchStatus TFetchStatus::Error(ui32 httpCode) {
    return TFetchStatus::Error(ClassifyHttpCode(httpCode));
}
TFetchStatus TFetchStatus::Failure(ui32 httpCode) {
    return TFetchStatus::Failure(ClassifyHttpCode(httpCode));
}

TFetchStatus TFetchStatus::Error(TString reason) {
    return {EStatus::StatusError, reason};
}
TFetchStatus TFetchStatus::Failure(TString reason) {
    return {EStatus::StatusFailure, reason};
}

bool TFetchStatus::IsSuccess() const {
    return Status == EStatus::StatusSuccess;
}

bool TFetchStatus::IsError() const {
    return Status == EStatus::StatusError;
}

bool TFetchStatus::IsFailure() const {
    return Status == EStatus::StatusFailure;
}

TString TFetchStatus::ClassifyHttpCode(ui32 code) {
    if (code == 0) {
        return TFetchStatus::HTTP_0;
    }
    if (code < 200) {
        return TFetchStatus::HTTP_1XX;
    }
    if (code >= 200 && code < 300) {
        return TFetchStatus::HTTP_2XX;
    }
    if (code >= 300 && code < 400) {
        return TFetchStatus::HTTP_3XX;
    }
    if (code >= 400 && code < 500) {
        return TFetchStatus::HTTP_4XX;
    }
    return TFetchStatus::HTTP_5XX;
}

IStatusCallback::TRef DefaultStatusCallback() {
    static IStatusCallback::TRef def = MakeIntrusive<THttpCodeStatusCallback>();
    return def;
}

THolder<TRequest> IMultiRequest::AddRequest(TStringBuf url, const TRequestOptions& options) {
    return AddRequest(ParseUri(url), options);
}

// TRequestAPI -----------------------------------------------------------------
TRequestPtr TRequestAPI::Request(TStringBuf url, const TRequestOptions& options, IRequestEventListener* listener) {
    return Request(ParseUri(url), options, listener);
}

TRequestPtr TRequestAPI::Request(const NUri::TUri& uri, const TRequestOptions& options,
                                 IRequestEventListener* listener) {
    return RetryableNehRequest(uri, options, Logger, listener);
}

IMultiRequest::TRef TRequestAPI::MultiRequest(IRequestEventListener* listener) {
    return new TNehMultiRequest(Logger, /*weak*/ false, listener);
}

IMultiRequest::TRef TRequestAPI::WeakMultiRequest(IRequestEventListener* listener) {
    return new TNehMultiRequest(Logger, /*weak*/ true, listener);
}

// -----------------------------------------------------------------------------
TRequestPtr Request(TStringBuf url, const TRequestOptions& options, IRequestEventListener* listener) {
    return BassRequestAPI().Request(url, options, listener);
}

TRequestPtr Request(const NUri::TUri& uri, const TRequestOptions& options, IRequestEventListener* listener) {
    return BassRequestAPI().Request(uri, options, listener);
}

IMultiRequest::TRef MultiRequest() {
    return BassRequestAPI().MultiRequest();
}

IMultiRequest::TRef WeakMultiRequest() {
    return BassRequestAPI().WeakMultiRequest();
}

} // namespace NHttpFetcher

// just for debug
template <>
void Out<NHttpFetcher::TRequestOptions>(IOutputStream& out, const NHttpFetcher::TRequestOptions& options) {
    out << "MA: " << options.MaxAttempts << ", TO: " << options.Timeout << ", RP: " << options.RetryPeriod;
}

