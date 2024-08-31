#pragma once

#include <alice/bass/forms/setup_context.h>
#include <alice/bass/libs/source_request/handle.h>

namespace NBASS {

template <class TResponse>
class TSetupRequest;

template <class TResponse>
class TSetupHandle : public IRequestHandle<TResponse> {
public:
    TSetupHandle(THolder<TSetupRequest<TResponse>> request, TContext& ctx, NHttpFetcher::IMultiRequest::TRef multiRequest)
        : Request(std::move(request))
        , Ctx(ctx)
    {
        if (auto responseFromContext = ctx.FindSetupResponse(Request->GetName())) {
            ResponseFromContext = responseFromContext;
        } else {
            Handle = Request->Fetch(multiRequest);
        }
    }

    TResultValue WaitAndParseResponse(TResponse* answer) {
        NSc::TValue factorsData;
        if (ResponseFromContext) {
            return Request->Parse(ResponseFromContext, answer, &factorsData);
        }
        if (!Handle) {
            return TError(TError::EType::SYSTEM, TStringBuilder() << "Trying to read absent request " << Request->GetName());
        }
        NHttpFetcher::TResponse::TRef response = Handle->Wait();

        if (TResultValue err = Request->Parse(response, answer, &factorsData)) {
            return err;
        }
        Ctx.AddSourceResponse(Request->GetName(), response);
        Ctx.AddSourceFactorsData(Request->GetName(), factorsData);

        return TResultValue();
    }

private:
    THolder<TSetupRequest<TResponse>> Request;
    TContext& Ctx;

    NHttpFetcher::TResponse::TConstRef ResponseFromContext;
    NHttpFetcher::THandle::TRef Handle;
};

template <class TResponse>
class TSetupRequest {
protected:
    explicit TSetupRequest(const TString& name)
        : Name(name)
    {
    }

public:
    virtual ~TSetupRequest() = default;

    virtual NHttpFetcher::THandle::TRef Fetch(NHttpFetcher::IMultiRequest::TRef multiRequest) = 0;
    virtual TResultValue Parse(NHttpFetcher::TResponse::TConstRef httpResponse, TResponse* response, NSc::TValue* factorsData) = 0;

    const TString& GetName() const {
        return Name;
    }

private:
    const TString Name;
};

template <class TResponse>
class TSetupRequestFactory : public TSetupRequest<TResponse> {
protected:
    using TBase = TSetupRequest<TResponse>;
    using TFactory = std::function<NHttpFetcher::THandle::TRef(NHttpFetcher::IMultiRequest::TRef)>;

    TSetupRequestFactory(const TString& name, TFactory factory)
        : TBase(name)
        , Factory(factory)
    {
    }

public:
    NHttpFetcher::THandle::TRef Fetch(NHttpFetcher::IMultiRequest::TRef multiRequest) override final {
        return Factory(multiRequest);
    }

private:
    TFactory Factory;
};

using TJsonSetupRequest = TSetupRequest<NSc::TValue>;

template <class TResponse>
std::unique_ptr<IRequestHandle<TResponse>> FetchSetupRequest(
    THolder<TSetupRequest<TResponse>> request, TContext& ctx, NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest())
{
    return std::make_unique<TSetupHandle<TResponse>>(std::move(request), ctx, multiRequest);
}

} // namespace NBASS
