#pragma once

#include "request.h"
#include "request_event_patcher.h"

#include <library/cpp/neh/http_common.h>

#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/proto2json.h>

namespace NMatrix {

namespace NPrivate {

static const NProtobufJson::TJson2ProtoConfig JSON_2_PROTO_CONFIG = NProtobufJson::TJson2ProtoConfig()
    .SetUseJsonName(true)
;
static const NProtobufJson::TProto2JsonConfig PROTO_2_JSON_CONFIG = NProtobufJson::TProto2JsonConfig()
    .SetUseJsonName(true)
    .SetEnumMode(NProtobufJson::TProto2JsonConfig::EnumName)
;

THolder<NNeh::IHttpRequest> CastNehRequestPtrToNehHttpRequestPtr(const NNeh::IRequestRef& request);
bool HttpRequestContentIsJson(const THttpHeaders& httpRequestHeaders);
TString GetRequestIdFromHttpRequestHeaders(const THttpHeaders& httpRequestHeaders);
TString GetCensoredHeaderValue(const TString& headerName, const TString& headerValue);
TString GetReplyHeadersString(const THttpHeaders& headers);

class TFakeDataEvent {
private:
    TFakeDataEvent() = delete;
    TFakeDataEvent(const TFakeDataEvent& other) = delete;
    TFakeDataEvent(TFakeDataEvent&& other) = delete;
};

} // namespace NPrivate

template <
    typename TRequestDataEvent,
    typename TResponseDataEvent,
    TRequestEventPatcher<TRequestDataEvent> RequestDataEventPatcher = EmptyRequestEventPatcher<TRequestDataEvent>,
    TRequestEventPatcher<TResponseDataEvent> ResponseDataEventPatcher = EmptyRequestEventPatcher<TResponseDataEvent>,

    // Hack for proto requests
    bool LogRawRequestData = true,
    bool LogRawResponseData = true
>
class THttpRequest : public IRequest<void> {
protected:
    struct TReply {
        TReply()
            : Data()
            , Headers()
            , Code(200)
        {}

        TReply(
            const TString& data,
            const THttpHeaders& headers,
            const int code
        )
            : Data(data.begin(), data.end())
            , Headers(headers)
            , Code(code)
        {}

        explicit TReply(const int code)
            : Data()
            , Headers()
            , Code(code)
        {}

        NNeh::TData Data;
        THttpHeaders Headers;
        int Code;
    };

public:
    THttpRequest(
        const TStringBuf name,
        std::atomic<size_t>& requestCounterRef,
        const bool needThreadSafeLogFrame,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        std::function<bool(NNeh::NHttp::ERequestType method)> isHttpMethodAllowed
    )
        : IRequest<void>(
            name,
            requestCounterRef,
            needThreadSafeLogFrame,
            rtLogClient
        )
        , HttpRequest_(NPrivate::CastNehRequestPtrToNehHttpRequestPtr(request))
        , Path_(HttpRequest_->Service())
        , RequestContentIsJson_(NPrivate::HttpRequestContentIsJson(HttpRequest_->Headers()))
        , ResponseHttpCode_(200)
        , ErrorMessage_(Nothing())
    {
        InitRtLog(TryGetRtLogTokenFromHttpRequestHeaders(HttpRequest_->Headers()).GetOrElse(""));

        {
            NEvClass::TMatrixHttpRequestMetaInfo event;
            event.SetRequestId(NPrivate::GetRequestIdFromHttpRequestHeaders(HttpRequest_->Headers()));
            event.SetServiceName(TString(name));
            event.SetPath(Path_);
            event.SetMethod(TString(HttpRequest_->Method()));
            event.SetRemoteHost(HttpRequest_->RemoteHost());
            event.SetCgi(TString(HttpRequest_->Cgi()));

            for (const auto& header : HttpRequest_->Headers()) {
                auto* headerToLog = event.AddHeaders();
                headerToLog->SetName(header.Name());
                headerToLog->SetValue(NPrivate::GetCensoredHeaderValue(header.Name(), header.Value()));
            }

            LogContext_.LogEventInfoCombo<NEvClass::TMatrixHttpRequestMetaInfo>(event);
        }

        if constexpr (LogRawRequestData) {
            TRequestDataEvent event;
            event.SetRawHttpRequestBody(TString(HttpRequest_->Body()));
            RequestDataEventPatcher(event);
            LogContext_.LogEventInfoCombo<TRequestDataEvent>(event);
        }

        if (IsFinished()) {
            return;
        }

        if (!TryFromString(HttpRequest_->Method(), Method_) || !isHttpMethodAllowed(Method_)) {
            SetError(TString::Join("Unsupported method '", HttpRequest_->Method(), '\''), 400);
            Metrics_.SetError("badmethod");
            LogContext_.LogEventErrorCombo<NEvClass::TMatrixUnsupportedHttpMethod>(TString(HttpRequest_->Method()));
            IsFinished_ = true;
            return;
        }
    }

    ~THttpRequest() {
        // Report request time with Path_
        try {
            Metrics_.PushTimeDiffWithNowHist(
                StartTime_,
                "request_time",
                "", /* code */
                Path_
            );
        } catch (...) {
        }
    }

    void SetResponseHttpCode(int code) {
        ResponseHttpCode_ = code;
    }

    void SetError(const TString& message, int code) {
        ErrorMessage_ = message;
        SetResponseHttpCode(code);
    }

    NThreading::TFuture<void> ReplyWithFutureCheck(const NThreading::TFuture<void>& rspFut) override final {
        try {
            rspFut.GetValueSync();
        } catch (...) {
            SetError(CurrentExceptionMessage(), 500);
        }

        return Reply();
    }

    NThreading::TFuture<void> Reply() override final {
        auto reply = ErrorMessage_.Defined() ? TReply(*ErrorMessage_, THttpHeaders(), ResponseHttpCode_) : GetReply();

        {
            NEvClass::TMatrixHttpResponseMetaInfo event;
            event.SetCode(reply.Code);

            for (const auto& header : reply.Headers) {
                auto* headerToLog = event.AddHeaders();
                headerToLog->SetName(header.Name());
                headerToLog->SetValue(NPrivate::GetCensoredHeaderValue(header.Name(), header.Value()));
            }

            LogContext_.LogEventInfoCombo<NEvClass::TMatrixHttpResponseMetaInfo>(event);
        }

        LogResponseData(reply);

        try {
            HttpRequest_->SendReply(reply.Data, NPrivate::GetReplyHeadersString(reply.Headers), reply.Code);
            Metrics_.RateHttpCode(1, reply.Code, Path_);
        } catch (...) {
            Metrics_.SetError("send_reply");
            LogContext_.LogEventErrorCombo<NEvClass::TMatrixSendHttpReplyError>(CurrentExceptionMessage());
        }

        return NThreading::MakeFuture();
    }

protected:
    virtual TReply GetReply() const = 0;

    virtual void LogResponseData(const TReply& reply) {
        if constexpr (LogRawResponseData) {
            TResponseDataEvent event;

            if (ErrorMessage_.Defined()) {
                event.SetErrorMessage(*ErrorMessage_);
                ResponseDataEventPatcher(event);
                LogContext_.LogEventErrorCombo<TResponseDataEvent>(event);
            } else {
                event.SetRawHttpResponseBody(TString(reply.Data.begin(), reply.Data.end()));
                ResponseDataEventPatcher(event);
                LogContext_.LogEventInfoCombo<TResponseDataEvent>(event);
            }
        }
    }

protected:
    THolder<NNeh::IHttpRequest> HttpRequest_;

    const TString Path_;
    NNeh::NHttp::ERequestType Method_;
    const bool RequestContentIsJson_;

    int ResponseHttpCode_;
    TMaybe<TString> ErrorMessage_;
};

// Http request with protobuf in the body
template <
    typename TRequest,
    typename TRequestDataEvent,
    typename TResponseDataEvent,
    TRequestEventPatcher<TRequestDataEvent> RequestDataEventPatcher = EmptyRequestEventPatcher<TRequestDataEvent>,
    TRequestEventPatcher<TResponseDataEvent> ResponseDataEventPatcher = EmptyRequestEventPatcher<TResponseDataEvent>,
    bool LogRawResponseData = true
>
class TProtoHttpRequest : public THttpRequest<
    /* TRequestDataEvent = */ NPrivate::TFakeDataEvent,
    TResponseDataEvent,
    /* TRequestDataEventPatcher = */ EmptyRequestEventPatcher<NPrivate::TFakeDataEvent>,
    ResponseDataEventPatcher,

    // Do not log raw request data
    // Special logging is implemented in this class
    /* LogRawRequestData = */ false,

    LogRawResponseData
> {
public:
    TProtoHttpRequest(
        const TStringBuf name,
        std::atomic<size_t>& requestCounterRef,
        const bool needThreadSafeLogFrame,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        std::function<bool(NNeh::NHttp::ERequestType method)> isHttpMethodAllowed
    )
        : THttpRequest<
            /* TRequestDataEvent = */ NPrivate::TFakeDataEvent,
            TResponseDataEvent,
            /* TRequestDataEventPatcher = */ EmptyRequestEventPatcher<NPrivate::TFakeDataEvent>,
            ResponseDataEventPatcher,
            /* LogRawRequestData = */ false,
            LogRawResponseData
        >(
            name,
            requestCounterRef,
            needThreadSafeLogFrame,
            rtLogClient,
            request,
            isHttpMethodAllowed
        )
        , Request_(MakeAtomicShared<TRequest>())
    {
        if (this->IsFinished()) {
            return;
        }

        bool protoParseIsOk = false;
        TMaybe<TString> protoParseErrorMessage;
        if (this->RequestContentIsJson_) {
            try {
                *Request_ = NProtobufJson::Json2Proto<TRequest>(this->HttpRequest_->Body(), NPrivate::JSON_2_PROTO_CONFIG);
                protoParseIsOk = true;
            } catch (...) {
                protoParseErrorMessage = CurrentExceptionMessage();
                protoParseIsOk = false;
            }
        } else {
            protoParseIsOk = Request_->ParseFromArray(this->HttpRequest_->Body().data(), this->HttpRequest_->Body().size());
        }

        if (protoParseIsOk) {
            TRequestDataEvent event;
            event.MutableProtoRequest()->CopyFrom(*Request_);
            RequestDataEventPatcher(event);
            this->LogContext_.template LogEventInfoCombo<TRequestDataEvent>(event);
        } else {
            const TString error = TString::Join(
                "Unable to parse proto",
                protoParseErrorMessage.Defined()
                    ? TString::Join(": '", *protoParseErrorMessage, '\'')
                    : ""
            );

            this->SetError(error, 400);
            this->Metrics_.SetError("proto_parse");

            TRequestDataEvent event;
            event.SetUnparsedRawRequest(TString(this->HttpRequest_->Body()));
            event.SetErrorMessage(error);
            RequestDataEventPatcher(event);
            this->LogContext_.template LogEventErrorCombo<TRequestDataEvent>(event);

            this->IsFinished_ = true;
            return;
        }
    }

protected:
    TAtomicSharedPtr<TRequest> Request_;
};

template <
    typename THttpRequestBase,
    typename TResponse,
    typename TResponseDataEvent,
    TRequestEventPatcher<TResponseDataEvent> ResponseDataEventPatcher = EmptyRequestEventPatcher<TResponseDataEvent>
>
class THttpRequestWithProtoResponse : public THttpRequestBase {
public:
    using THttpRequestBase::THttpRequestBase;
    using TReply = typename THttpRequestBase::TReply;

protected:
    TReply GetReply() const override final {
        return TReply(GetReplyData(), THttpHeaders(), this->ResponseHttpCode_);
    }

    void LogResponseData(const TReply& /* reply */) override final {
        TResponseDataEvent event;

        if (this->ErrorMessage_.Defined()) {
            event.SetErrorMessage(this->ErrorMessage_.GetRef());
            ResponseDataEventPatcher(event);
            this->LogContext_.template LogEventErrorCombo<TResponseDataEvent>(event);
        } else {
            event.MutableProtoResponse()->CopyFrom(Response_);
            ResponseDataEventPatcher(event);
            this->LogContext_.template LogEventInfoCombo<TResponseDataEvent>(event);
        }
    }

private:
    TString GetReplyData() const {
        if (this->RequestContentIsJson_) {
            return NProtobufJson::Proto2Json(Response_, NPrivate::PROTO_2_JSON_CONFIG);
        } else {
            return Response_.SerializeAsString();
        }
    }

protected:
    TResponse Response_;
};

template<
    typename TResponse,
    typename TRequestDataEvent,
    typename TResponseDataEvent,
    TRequestEventPatcher<TRequestDataEvent> RequestDataEventPatcher = EmptyRequestEventPatcher<TRequestDataEvent>,
    TRequestEventPatcher<TResponseDataEvent> ResponseDataEventPatcher = EmptyRequestEventPatcher<TResponseDataEvent>
>
using TRawHttpRequestWithProtoResponse = THttpRequestWithProtoResponse<
    THttpRequest<
        TRequestDataEvent,
        /* TResponseDataEvent = */ NPrivate::TFakeDataEvent,
        RequestDataEventPatcher,
        /* ResponseDataEventPatcher = */ EmptyRequestEventPatcher<NPrivate::TFakeDataEvent>,

        /* LogRawRequestData = */ true,

        // Do not log raw response data
        // Special logging is implemented in this class
        /* LogRawResponseData = */ false
    >,
    TResponse,
    TResponseDataEvent,
    ResponseDataEventPatcher
>;

template<
    typename TRequest,
    typename TResponse,
    typename TRequestDataEvent,
    typename TResponseDataEvent,
    TRequestEventPatcher<TRequestDataEvent> RequestDataEventPatcher = EmptyRequestEventPatcher<TRequestDataEvent>,
    TRequestEventPatcher<TResponseDataEvent> ResponseDataEventPatcher = EmptyRequestEventPatcher<TResponseDataEvent>
>
using TProtoHttpRequestWithProtoResponse = THttpRequestWithProtoResponse<
    TProtoHttpRequest<
        TRequest,
        TRequestDataEvent,
        /* TResponseDataEvent = */ NPrivate::TFakeDataEvent,
        RequestDataEventPatcher,
        /* ResponseDataEventPatcher = */ EmptyRequestEventPatcher<NPrivate::TFakeDataEvent>,

        // Do not log raw response data
        // Special logging is implemented in this class
        /* LogRawResponseData = */ false
    >,
    TResponse,
    TResponseDataEvent,
    ResponseDataEventPatcher
>;

} // namespace NMatrix
