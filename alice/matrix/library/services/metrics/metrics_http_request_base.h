#pragma once

#include <alice/matrix/library/request/http_request.h>

namespace NMatrix {

template <typename TRequestDataEvent, typename TResponseDataEvent>
class TMetricsHttpRequestBase : public THttpRequest<
    TRequestDataEvent,
    /* TResponseDataEvent = */ NPrivate::TFakeDataEvent,
    EmptyRequestEventPatcher<TRequestDataEvent>,
    /* ResponseDataEventPatcher = */ EmptyRequestEventPatcher<NPrivate::TFakeDataEvent>,
    /* LogRawRequestData = */ true,

    // Do not log raw response data
    // Special logging is implemented in this class
    /* LogRawResponseData = */ false
> {
public:
    using TReply = typename THttpRequest<
        TRequestDataEvent,
        /* TResponseDataEvent = */ NPrivate::TFakeDataEvent,
        EmptyRequestEventPatcher<TRequestDataEvent>,
        /* ResponseDataEventPatcher = */ EmptyRequestEventPatcher<NPrivate::TFakeDataEvent>,
        /* LogRawRequestData = */ true,

        // Do not log raw response data
        // Special logging is implemented in this class
        /* LogRawResponseData = */ false
    >::TReply;

public:
    TMetricsHttpRequestBase(
        const TStringBuf name,
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request
    )
        : THttpRequest<
            TRequestDataEvent,
            /* TResponseDataEvent = */ NPrivate::TFakeDataEvent,
            EmptyRequestEventPatcher<TRequestDataEvent>,
            /* ResponseDataEventPatcher = */ EmptyRequestEventPatcher<NPrivate::TFakeDataEvent>,
            /* LogRawRequestData = */ true,

            // Do not log raw response data
            // Special logging is implemented in this class
            /* LogRawResponseData = */ false
        >(
            name,
            requestCounterRef,
            /* needThreadSafeLogFrame = */ false,
            rtLogClient,
            request,
            [](NNeh::NHttp::ERequestType method) {
                return NNeh::NHttp::ERequestType::Get == method;
            }
        )
        , ResponseFormat_(GetResponseFormat())
    {}


    NThreading::TFuture<void> ServeAsync() override final {
        TStringStream outStream;

        switch (ResponseFormat_) {
            case EResponseFormat::JSON: {
                GetMetricsJson(outStream);
                HttpReply_.Headers.AddHeader("Content-Type", "application/json");
                break;
            }
            case EResponseFormat::SPACK: {
                GetMetricsSpack(outStream);
                HttpReply_.Headers.AddHeader("Content-Type", "application/x-solomon-spack");
                break;
            }
        }

        HttpReply_.Code = 200;
        {
            const TString str = outStream.Str();
            HttpReply_.Data = NNeh::TData(str.data(), str.data() + str.size());
        }

        return NThreading::MakeFuture();
    }

private:
    enum class EResponseFormat {
        JSON = 0,
        SPACK = 1,
    };

private:
    EResponseFormat GetResponseFormat() const {
        TCgiParameters cgi(this->HttpRequest_->Cgi());
        const TString& requestedFormat = cgi.Get("format");

        return requestedFormat == "json"sv ? EResponseFormat::JSON : EResponseFormat::SPACK;
    }

protected:
    TReply GetReply() const override final {
        return HttpReply_;
    }

    virtual void GetMetricsJson(TStringStream& outStream) = 0;
    virtual void GetMetricsSpack(TStringStream& outStream) = 0;

    void LogResponseData(const TReply& reply) override final {
        TResponseDataEvent event;

        if (this->ErrorMessage_.Defined()) {
            event.SetErrorMessage(*this->ErrorMessage_);
            this->LogContext_.template LogEventErrorCombo<TResponseDataEvent>(event);
            return;
        }

        switch (ResponseFormat_) {
            case EResponseFormat::JSON: {
                event.SetJsonResponse(TString(reply.Data.begin(), reply.Data.end()));
                break;
            }
            case EResponseFormat::SPACK: {
                event.SetSpackResponseSize(reply.Data.size());
                break;
            }
        }
        this->LogContext_.template LogEventInfoCombo<TResponseDataEvent>(event);
    }

private:
    TReply HttpReply_;
    EResponseFormat ResponseFormat_;
};

} // namespace NMatrix
