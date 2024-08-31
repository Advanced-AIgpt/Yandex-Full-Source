#pragma once

#include <utility>

#include "tsum_tracer.h"

#include <alice/bass/forms/market/checkout_user_info.h>
#include <alice/bass/forms/market/market_exception.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/source_request/source_request.h>
#include <alice/library/network/headers.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/scheme/util/scheme_holder.h>

namespace NBASS {

namespace NMarket {

template <class TTypedResponse>
class THttpResponse;

namespace {

template <class TTypedResponse>
TString GetErrorMessage(const THttpResponse<TTypedResponse>& response)
{
    Y_ASSERT(response.IsError());
    TStringBuf errorMessage;
    if (response.IsTimeout()) {
        errorMessage = TStringBuf("Timeout error: ");
    } else if (!response.IsHttpOk()) {
        errorMessage = TStringBuf("Response code is not 200: ");
    } else {
        errorMessage = TStringBuf("cannot fetch data: ");
    }
    return TStringBuilder() << errorMessage << response.GetErrorText();
}

}

class THttpException: public TMarketException {
public:
    template <class TTypedResponse>
    explicit THttpException(const THttpResponse<TTypedResponse>& response)
        : TMarketException(GetErrorMessage(response))
    {
    }

    explicit THttpException(const TString& errMsg)
        : TMarketException(errMsg)
    {
    }
};

class THttpTimeoutException: public THttpException {
public:
    template <class TTypedResponse>
    explicit THttpTimeoutException(const THttpResponse<TTypedResponse>& response)
        : THttpException(response)
    {
    }

    explicit THttpTimeoutException(const TString& errMsg)
        : THttpException(errMsg)
    {
    }
};

template <class TTypedResponse>
class THttpResponseConverter {
public:
    TTypedResponse FromJson(const NSc::TValue& rawValue)
    {
        return TTypedResponse(rawValue);
    }
};

template <class TScheme>
class THttpResponseConverter<TSchemeHolder<TScheme>> {
public:
    TSchemeHolder<TScheme> FromJson(const NSc::TValue& rawValue)
    {
        const size_t MAX_LOG_RESPONSE_LEN = 1024;

        TSchemeHolder<TScheme> typedResponse(rawValue);
        auto onValidateError = [](const TString& path, const TString& msg) {
            LOG(DEBUG) << "Validation error in market response: "
                       << path << " " << msg << Endl;
        };
        if (!typedResponse.Scheme().Validate("", false, onValidateError)) {
            const auto& json = rawValue.ToJson();
            if (json.size() < MAX_LOG_RESPONSE_LEN) {
                LOG(DEBUG) << "Invalid market response: " << json << Endl;
            } else {
                LOG(DEBUG) << "Invalid market response: "
                           << json.substr(0, MAX_LOG_RESPONSE_LEN)
                           << "..." << Endl;
            }

            ythrow TErrorException(
                NBASS::TError::EType::MARKETERROR,
                TStringBuf("can not validate response data"));
        }
        return typedResponse;
    }
};

/**
 * Расширяет класс NHttpFetcher::TResponse
 * Чтобы можно было в бизнес логике получать типизированный ответ и узнавать http код,
 * наличие timeout ошибок и другую мета инфу ответа.
 *
 * @tparam TResponse
 */
template <class TTypedResponse>
class THttpResponse: public NHttpFetcher::TResponse {
public:
    explicit THttpResponse(const NHttpFetcher::TResponse::TRef& response)
        : NHttpFetcher::TResponse(*response)
    {
    }

    TTypedResponse GetResponse() const
    {
        if (IsTimeout()) {
            ythrow THttpTimeoutException(*this);
        } else if (IsError()) {
            ythrow THttpException(*this);
        }

        auto resultValue = NSc::TValue::FromJson(Data);
        if (resultValue.IsNull()) {
            ythrow TErrorException(
                NBASS::TError::EType::MARKETERROR,
                TStringBuf("cannot parse data"));
        }

        return THttpResponseConverter<TTypedResponse>().FromJson(resultValue);
    }
};

template <>
class THttpResponse<void>: public NHttpFetcher::TResponse {
public:
    explicit THttpResponse(const NHttpFetcher::TResponse::TRef& response)
        : NHttpFetcher::TResponse(*response)
    {
    }

    bool GetResponse() const
    {
        if (IsTimeout()) {
            ythrow THttpTimeoutException(*this);
            return false;
        } else if (IsError()) {
            ythrow THttpException(*this);
            return false;
        }
        return true;
    }
};

/**
 * Класс нужен для асинхронных запросов: можно сделать запрос, получить Handle и вызвать Wait позже в нужный момент
 * Позволяет в Wait вернуть THttpResponse вместо стандартного NHttpFetcher::TResponse
 * @tparam TResponse
 */
template <class TTypedResponse>
class TResponseHandle {
public:
    explicit TResponseHandle(NHttpFetcher::THandle::TRef handle, TMaybe<TTSUMTracer> tracer = Nothing())
        : Handle(std::move(handle)), Tracer(std::move(tracer))
    {
    }

    NHttpFetcher::TResponse::TRef DoWait() { return DoWait(TInstant::Max()); }

    NHttpFetcher::TResponse::TRef DoWait(TInstant deadline)
    {
        if (!Handle) {
            ythrow TErrorException(
                NBASS::TError::EType::MARKETERROR,
                "request is not run");
        }
        auto response = Handle->Wait(deadline);
        if (Tracer.Defined()) TSUM_TRACE() << (*Tracer << *response);
        return response;
    }

    THttpResponse<TTypedResponse> Wait() { return Wait(TInstant::Max()); }

    THttpResponse<TTypedResponse> Wait(TInstant deadline)
    {
        return THttpResponse<TTypedResponse>(DoWait(deadline));
    }

private:
    NHttpFetcher::THandle::TRef Handle;
    TMaybe<TTSUMTracer> Tracer;
};

/**
 * Обертка вокруг NHttpFetcher::TRequestPtr
 * Нужна для передачи информации для трассировки.
 *
 * @tparam TTypedResponse
 */
template <class TTypedResponse>
class TPreparedRequest {
public:
    explicit TPreparedRequest(NHttpFetcher::TRequestPtr& request, TMaybe<TTSUMTracer> tracer=Nothing())
        : Request(std::move(request)), Tracer(std::move(tracer))
    {
    }
    TResponseHandle<TTypedResponse> Fetch() {
        if (Tracer.Defined()) *Tracer << *Request;
        auto request = Request->Fetch();
        return TResponseHandle<TTypedResponse>(request, Tracer);
    }
private:
    NHttpFetcher::TRequestPtr Request;
    TMaybe<TTSUMTracer> Tracer;
};

class TBaseClient {
public:
    explicit TBaseClient(const TSourcesRequestFactory& sources, TMarketContext& ctx)
        : Sources(sources)
        , Ctx(ctx)
    {
    }

protected:
    NHttpFetcher::TRequestPtr CompileRequest(
        const TSourceRequestFactory& source,
        const TCgiParameters& params,
        const NSc::TValue& body = NSc::TValue(),
        const THashMap<TString, TString>& headers = THashMap<TString, TString>(),
        TStringBuf method_ = TStringBuf()) const
    {
        NHttpFetcher::TRequestPtr request = source.Request();
        request->AddCgiParams(params);
        request->AddHeader("X-Market-Req-ID", Ctx.MarketRequestId());
        for (const auto& header : headers) {
            Y_ENSURE(!AsciiEqualsIgnoreCase(header.first, NAlice::NNetwork::HEADER_X_MARKET_REQ_ID));
            request->AddHeader(header.first, header.second);
        }
        TStringBuf method = method_.empty()
            ? body.IsNull() ? TStringBuf("GET") : TStringBuf("POST")
            : method_;
        if (!body.IsNull()) {
            request->AddHeader(TStringBuf("Content-Type"), TStringBuf("application/json"));
            request->SetBody(body.ToJson(), method);
        } else {
            request->SetMethod(method);
        }

        return request;
    }
    template <class TTypedResponse>
    TResponseHandle<TTypedResponse> Run(NHttpFetcher::TRequestPtr request) const
    {
        auto handle = TPreparedRequest<TTypedResponse>(request);
        return handle.Fetch();
    }
    template <class TTypedResponse>
    TResponseHandle<TTypedResponse> Run(
        const TSourceRequestFactory& source,
        const TCgiParameters& params,
        const NSc::TValue& body = NSc::TValue(),
        const THashMap<TString, TString>& headers = THashMap<TString, TString>(),
        TStringBuf method_ = TStringBuf()) const
    {
        return Run<TTypedResponse>(CompileRequest(source, params, body, headers, method_));
    }
    template <class TTypedResponse>
    TResponseHandle<TTypedResponse> RunWithTrace(
        TStringBuf sourceModule,
        TStringBuf targetModule,
        NHttpFetcher::TRequestPtr request) const
    {
        auto handle = TPreparedRequest<TTypedResponse>(request,
            TTSUMTracer(TTSUMTracer::ERequestType::OUT, sourceModule, targetModule));
        return handle.Fetch();
    }
    template <class TTypedResponse>
    TResponseHandle<TTypedResponse> RunWithTrace(
        TStringBuf sourceModule,
        TStringBuf targetModule,
        const TSourceRequestFactory& source,
        const TCgiParameters& params,
        const NSc::TValue& body = NSc::TValue(),
        const THashMap<TString, TString>& headers = THashMap<TString, TString>(),
        TStringBuf method_ = TStringBuf()) const
    {
        return RunWithTrace<TTypedResponse>(sourceModule, targetModule,
            CompileRequest(source, params, body, headers, method_));
    }

    TSourcesRequestFactory Sources;
    TMarketContext& Ctx;
};

} // namespace NMarket

} // namespace NBASS
