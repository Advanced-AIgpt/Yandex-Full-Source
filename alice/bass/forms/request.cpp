#include "request.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/continuation_register.h>
#include <alice/bass/forms/setup_context.h>
#include <alice/bass/forms/video/protocol_scenario_helpers/request_creator.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/bass/forms/vins.h>

#include <alice/bass/application.h>
#include <alice/bass/libs/eventlog/events.ev.pb.h>
#include <alice/bass/libs/fetcher/util.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/bass_logadapter.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/metrics/signals.h>
#include <alice/bass/libs/video_common/feature_calculator.h>
#include <alice/bass/protobuf_request.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/network/headers.h>
#include <alice/library/proto/proto.h>

#include <google/protobuf/message.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/threading/future/future.h>

#include <util/datetime/base.h>
#include <util/generic/variant.h>
#include <util/thread/pool.h>
#include <util/string/builder.h>
#include <util/string/split.h>
#include <util/system/env.h>
#include <util/system/hostname.h>

namespace NBASS {
namespace {

const TStringBuf BASS_SRCRWR_PREFIX = "BASS_";

using TVoidHandler = void (TApplication::*)();

template <TVoidHandler F>
class TRawRequestHandler : public IHttpRequestHandler {
public:
    bool DoHttpReply(TGlobalContextPtr /* globalCtx */, const TParsedHttpFull& /* http */, const TRequestReplier::TReplyParams& params) override {
        (TApplication::GetInstance()->*F)();
        params.Output << THttpResponse(HTTP_OK) << Endl;
        return true;
    }
};

TString PrintFormResponse(const NSc::TValue& value) {
    NSc::TValue ret = value;
    ret.GetDictMutable().erase("setup_responses");
    return ret.ToJson();
}

const TString& GetAuthorizationHeader(const THttpHeaders& headers) {
    for (const auto& header : headers) {
        if (AsciiEqualsIgnoreCase(header.Name(), NAlice::NNetwork::HEADER_AUTHORIZATION) ||
            AsciiEqualsIgnoreCase(header.Name(), NAlice::NNetwork::HEADER_X_OAUTH_TOKEN))
        {
            return header.Value();
        }
    }

    auto constructCustomToken = []() -> TString {
        const TString customToken{GetEnv("PASSPORT_OAUTH_TOKEN")};
        if (!customToken.empty()) {
            return TStringBuilder() << TStringBuf("OAuth ") << customToken;
        }

        return {};
    };

    static const TString customToken = constructCustomToken();

    return customToken;
}

TMaybe<TString> GetUserTicketHeader(const THttpHeaders& headers) {
    if (const auto* header = FindIfPtr(headers, [](const THttpInputHeader& header) {
            return AsciiEqualsIgnoreCase(header.Name(), NAlice::NNetwork::HEADER_X_YA_USER_TICKET);
        }))
    {
        return header->Value();
    }
    return Nothing();
}

const TString& GetAppInfoHeader(const THttpHeaders& headers) {
    for (const auto& header : headers) {
        if (AsciiEqualsIgnoreCase(header.Name(), NAlice::NNetwork::HEADER_X_YANDEX_APP_INFO)) {
            return header.Value();
        }
    }

    static const TString emptyAppInfoHeader = {};
    return emptyAppInfoHeader;
}

const TString& GetFakeTimeHeader(const THttpHeaders& headers) {
    for (const auto& header : headers) {
        if (AsciiEqualsIgnoreCase(header.Name(), NAlice::NNetwork::HEADER_X_YANDEX_FAKE_TIME)) {
            return header.Value();
        }
    }

    static const TString emptyFakeTimeHeader = {};
    return emptyFakeTimeHeader;
}

const TString& GetMarketReqIdHeader(const THttpHeaders& headers) {
    for (const auto& header : headers) {
        if (AsciiEqualsIgnoreCase(header.Name(), NAlice::NNetwork::HEADER_X_MARKET_REQ_ID)) {
            return header.Value();
        }
    }

    static const TString emptyMarketReqIdHeader = {};
    return emptyMarketReqIdHeader;
}

NSc::TValue GetConfigPatchFromCgiAndHeaders(const TParsedHttpFull& http, const THttpHeaders& headers) {
    static constexpr TStringBuf proxyHeaderNameBegin = "x-yandex-proxy-header-";
    NSc::TValue configPatch;

    auto onSrcRwrCgi = [&configPatch](TStringBuf srcrwr) {
        NSc::TValue parsedSrcrwr;
        NSc::TValue& parsedSrcrwrVins = parsedSrcrwr["Vins"];

        NHttpFetcher::ParseSrcRwrCgi(srcrwr, [&](TStringBuf name, TStringBuf value) {
            Y_ASSERT(!name.Empty());
            Y_ASSERT(!value.Empty());
            if (name.StartsWith(BASS_SRCRWR_PREFIX)) {
                parsedSrcrwrVins[name.substr(BASS_SRCRWR_PREFIX.length())]["Host"] = value;
            }
        });

        if (!parsedSrcrwrVins.IsNull()) {
            configPatch.MergeUpdate(parsedSrcrwr);
        }
    };

    auto onSrcRwrHeader = [&configPatch](TStringBuf srcrwr) {
        NSc::TValue parsedSrcrwr;
        NSc::TValue& parsedSrcrwrVins = parsedSrcrwr["Vins"];

        NHttpFetcher::ParseSrcRwr(srcrwr, [&](TStringBuf name, TStringBuf value) {
            Y_ASSERT(!name.Empty());
            Y_ASSERT(!value.Empty());
            parsedSrcrwrVins[name]["Host"] = value;
        });

        if (!parsedSrcrwrVins.IsNull()) {
            configPatch.MergeUpdate(parsedSrcrwr);
        }
    };

    auto onProxyHeader = [&configPatch](TStringBuf name, TStringBuf value) {
        const TStringBuf proxyHeaderName{name.Skip(proxyHeaderNameBegin.size())};
        if (proxyHeaderName.empty()) {
            return;
        }

        NSc::TValue& headerJson = configPatch["FetcherProxy"]["Headers"].Push();
        headerJson["Name"].SetString(proxyHeaderName);
        headerJson["Value"].SetString(value);
    };

    if (http.Cgi) {
        TCgiParameters cgi;
        cgi.Scan(http.Cgi);

        for (const auto& val: cgi.Range("config")) {
            configPatch.MergeUpdate(NSc::TValue::FromJson(val));
        }

        for (const auto& val: cgi.Range("srcrwr")) {
            onSrcRwrCgi(val);
        }
    }

    for (const auto& h : headers) {
        const TString name{to_lower(h.Name())};

        if (name == TStringBuf("x-yandex-via-proxy")) {
            configPatch["FetcherProxy"]["HostPort"].SetString(h.Value());
        } else if (name == NAlice::NNetwork::HEADER_X_SRCRWR) {
            onSrcRwrHeader(h.Value());
        } else if (name.StartsWith(proxyHeaderNameBegin)) {
            onProxyHeader(name, h.Value());
        }
    }

    // We don't need proxy info if we don't have a proxy
    if (configPatch["FetcherProxy"]["HostPort"].GetString().empty()) {
        configPatch.Delete("FetcherProxy");
    }

    return configPatch;
}

} // namespace

/** Base class for all vins/setup request handlers.
 * Also it is used as a visitor for variant (error or json).
 * @see operator()
 */
template <typename T>
class TBassRequest {
public:
    /// For visitor type if process is failed.
    using TFail = TError;
    /// For visitor type if process is succeeded.
    using TSuccess = TContext::TInitializer;

    using TResult = std::variant<TSuccess, TFail>;
    using TScheme = T;

public:
    TBassRequest(TGlobalContextPtr globalCtx, const NSc::TValue& input, NSc::TValue& output)
        : GlobalCtx{globalCtx}
        , Input{input}
        , Output{output}
    {
    }

    virtual ~TBassRequest() = default;

    HttpCodes Process(const TParsedHttpFull& http, const THttpHeaders& headers) {
        TScheme reqScheme{&Input};
        auto result = CreateContextInitializer(reqScheme, http, headers);
        return std::visit(*this, result);
    }

    virtual HttpCodes operator()(const TFail& error) = 0;
    virtual HttpCodes operator()(TSuccess& ctxInitData) = 0;

protected:
    virtual TStringBuf LogPrefix() const = 0;

private:
    TResult CreateContextInitializer(TScheme reqScheme, const TParsedHttpFull& http, const THttpHeaders& headers) {
        TLogging::ReqInfo.Get().Update(TLogging::BassReqId.Get());

        TResultValue error;
        auto validateCb = [&error](TStringBuf key, TStringBuf errmsg) {
            if (!error) {
                error = TError{TError::EType::INVALIDPARAM};
            } else {
                error->Msg << "; ";
            }
            error->Msg << key << ": " << errmsg;
        };
        if (!reqScheme.Validate({}, false, validateCb)) {
            Y_ASSERT(error);
            return *error;
        }

        TMaybe<NAlice::TEvent> speechKitEvent;
        if (reqScheme.Meta().HasEvent()) {
            const NJson::TJsonValue json = reqScheme.Meta().Event().GetRawValue()->ToJsonValue();
            NAlice::TEvent tmpEvent;
            if (NAlice::JsonToProto(json, tmpEvent).ok()) {
                speechKitEvent = tmpEvent;
            }
        }

        const TStringBuf vinsReqId{reqScheme.Meta().RequestId()};
        if (vinsReqId) {
            // Replace BASS reqid (from http-header) with VINS reqid (from request meta)
            if (speechKitEvent.Defined() && speechKitEvent->HasHypothesisNumber()) {
                TLogging::ReqInfo.Get().Update(vinsReqId, speechKitEvent->GetHypothesisNumber());
            } else {
                TLogging::ReqInfo.Get().Update(vinsReqId);
            }
        }

        const auto& authorizationHeader = GetAuthorizationHeader(headers);
        const auto& appInfoHeader = GetAppInfoHeader(headers);
        const auto& fakeTimeHeader = GetFakeTimeHeader(headers);
        const auto& marketRequIdHeader = GetMarketReqIdHeader(headers);
        TMaybe<TString> userTicketHeader = GetUserTicketHeader(headers);
        TContext::TInitializer ctxInitData{GlobalCtx, TLogging::ReqInfo.Get().ReqId(), marketRequIdHeader,
                                           authorizationHeader, appInfoHeader,
                                           fakeTimeHeader, userTicketHeader, std::move(speechKitEvent)};
        ctxInitData.ConfigPatch = GetConfigPatchFromCgiAndHeaders(http, headers);

        LOG(INFO) << LogPrefix() << "request : " << PrintReportRequest(reqScheme) << Endl;
        if (http.Cgi) {
            LOG(INFO) << LogPrefix() << "request cgi : " << http.Cgi << Endl;
        }

        if (authorizationHeader) {
            LOG(INFO) << "authorization header is presented" << Endl;
        } else {
            LOG(INFO) << "authorization header is missed" << Endl;
        }

        return std::move(ctxInitData);
    }

private:
    static TString PrintReportRequest(TScheme scheme) {
        NSc::TValue ret = *scheme.GetRawValue();
        ret.GetDictMutable().erase("setup_responses");
        ret.GetDictMutable().erase("data_sources");
        ret["meta"].GetDictMutable().erase("cookies");
        return ret.ToJson();
    }

protected:
    TGlobalContextPtr GlobalCtx;
    const NSc::TValue& Input;
    NSc::TValue& Output;
};

class TContextedRequest : public TBassRequest<NBASSRequest::TRequestConst<TSchemeTraits>> {
public:
    TContextedRequest(TGlobalContextPtr globalCtx, const NSc::TValue& input, NSc::TValue& output, TString type)
        : TBassRequest<NBASSRequest::TRequestConst<TSchemeTraits>>(globalCtx, input, output)
        , Type(std::move(type))
    {
    }

    using TBassRequest<NBASSRequest::TRequestConst<TSchemeTraits>>::TBassRequest;

public:
    HttpCodes operator()(const TFail& error) override {
        error.ToJson(Output);
        // These logs will be with BASS reqid (from balancer)
        const TString errorMsg = TStringBuilder() << "context parsing error: " << Output.ToJson();
        LOG(ERR) << errorMsg << Endl;
        Y_STATS_INC_COUNTER(Type + "_bad");
        return HTTP_BAD_REQUEST;
    }

    HttpCodes operator()(TSuccess& ctxInitData) override {
        TContext::TPtr context;
        if (const auto error = TContext::FromJson(Input, ctxInitData, &context)) {
            error->ToJson(Output);
            // These logs will be with BASS reqid (from balancer)
            const TString errorMsg = TStringBuilder() << "context parsing error: " << Output.ToJson();
            LOG(ERR) << errorMsg << Endl;
            Y_STATS_INC_COUNTER(Type + "_bad");
            return HTTP_BAD_REQUEST;
        }

        if (context->Meta().RequestId()->empty()) {
            LOG(WARNING) << "Empty VINS request_id" << Endl;
        }
        LOG(INFO) << "BASS reqid : " << TLogging::ReqInfo.Get().ReqId() << Endl;

        return Handle(context);
    }

protected:
    virtual HttpCodes Handle(TContext::TPtr ctx) = 0;

    TStringBuf LogPrefix() const override {
        return {};
    }

private:
    const TString Type;
};

/** Class which handles report (/vins) requests.
 */
class TReportRequest : public TContextedRequest {
public:
    TReportRequest(TGlobalContextPtr globalCtx, const NSc::TValue& input, NSc::TValue& output)
        : TContextedRequest(globalCtx, input, output, "bass_request")
    {
    }

protected:
    HttpCodes Handle(TContext::TPtr context) override {
        if (context->HasInputAction()) {
            return HandleAction(context);
        }
        return HandleForm(context);
    }

private:
    HttpCodes HandleForm(TContext::TPtr context) {
        TString counterName = TString{TStringBuf{context->FormName()}.RNextTok('.')};
        Y_STATS_SCOPE_HISTOGRAM("bass_request_" + counterName);

        TRequestHandler handler{context};
        if (const auto error = handler.RunFormHandler(&Output)) {
            error->ToJson(Output);
            const TString json = PrintFormResponse(Output);
            LOG(ERR) << "form error: " << json << Endl;
            Y_STATS_INC_COUNTER("bass_request_fail_" + counterName);

            return GetHttpCode(*error);
        }

        const TString json = PrintFormResponse(Output);
        LOG(INFO) << "form response: " << json << Endl;
        Y_STATS_INC_COUNTER("bass_request_success_" + counterName);

        return HTTP_OK;
    }

    HttpCodes HandleAction(TContext::TPtr context) {
        const TInputAction& action = context->InputAction().GetRef();
        const TString counterName = TString{TStringBuf{action.Name}.RNextTok('.')};
        Y_STATS_SCOPE_HISTOGRAM("bass_action_" + counterName);

        TRequestHandler handler{context};
        if (const auto error = handler.RunActionHandler(&Output)) {
            error->ToJson(Output);
            LOG(ERR) << "action error: " << Output.ToJson() << Endl;
            Y_STATS_INC_COUNTER("bass_action_fail_" + counterName);
            return GetHttpCode(*error);
        }

        LOG(INFO) << "action response: " << Output.ToJson() << Endl;
        Y_STATS_INC_COUNTER("bass_action_success_" + counterName);

        return HTTP_OK;
    }
};

/** Class which handles report (/megamind) requests.
 */
class TMegamindPrepareRequest : public TContextedRequest {
public:
    TMegamindPrepareRequest(TGlobalContextPtr globalCtx, const NSc::TValue& input, NSc::TValue& output)
        : TContextedRequest(globalCtx, input, output, "bass_prepare")
    {
    }

protected:
    TStringBuf LogPrefix() const override {
        return TStringBuf("prepare ");
    }

    HttpCodes Handle(TContext::TPtr context) override {
        TString counterName = TString{TStringBuf{context->FormName()}.RNextTok('.')};
        Y_STATS_SCOPE_HISTOGRAM("bass_prepare_" + counterName);

        TRequestHandler handler{context};
        if (const auto error = handler.RunContinuableHandler(&Output)) {
            error->ToJson(Output);
            LOG(ERR) << "bass prepare error: " << Output.ToJson() << Endl;
            Y_STATS_INC_COUNTER("bass_prepare_fail_" + counterName);
            return GetHttpCode(*error);
        }

        LOG(INFO) << "prepare response: " << Output.ToJson() << Endl;
        Y_STATS_INC_COUNTER("bass_prepare_success_" + counterName);

        return HTTP_OK;
    }
};

class TMegamindApplyRequest {
public:
    TMegamindApplyRequest(const TString& authHeader, const TString& appInfoHeader, TMaybe<TString> userTicketHeader,
                          TGlobalContextPtr globalCtx, const NSc::TValue& input, NSc::TValue& output,
                          const NSc::TValue& configPatch)
        : AuthHeader(authHeader)
        , AppInfoHeader(appInfoHeader)
        , UserTicketHeader(std::move(userTicketHeader))
        , GlobalCtx(globalCtx)
        , Input(input)
        , Output(output)
        , ConfigPatch(configPatch)
    {
    }

    HttpCodes Process() {
        TStringBuf name = Input["ObjectTypeName"].GetString();
        const NSc::TValue meta = NSc::TValue::FromJson(Input["Meta"].GetString());
        Y_STATS_SCOPE_HISTOGRAM(TStringBuilder{} << "bass_apply_" << name);

        IContinuation::TPtr continuation = GlobalCtx->ContinuationRegistry().MakeFromJson(
            Input, GlobalCtx, meta, AuthHeader, AppInfoHeader, /* fakeTimeHeader = */ {}, UserTicketHeader, ConfigPatch);
        if (!continuation) {
            return ReportError({TError::EType::INVALIDPARAM, "Failed to deserialize continuation"}, name);
        }

        if (auto error = continuation->ApplyIfNotFinished()) {
            return ReportError(std::move(*error), name);
        }

        NSc::TValue context;
        continuation->GetContext().AddClientFeaturesBlock();
        continuation->GetContext().ToJson(&context);
        const TString json = PrintFormResponse(context);
        LOG(INFO) << "apply response: " << json << Endl;
        Output["State"] = json;
        Y_STATS_INC_COUNTER(TStringBuilder{} << "bass_apply_success_" << name);

        return HTTP_OK;
    }

private:
    HttpCodes ReportError(const TError& error, TStringBuf name) {
        error.ToJson(Output);
        LOG(ERR) << "bass apply error: " << Output.ToJson() << Endl;
        Y_STATS_INC_COUNTER(TStringBuilder{} << "bass_apply_fail_" << name);
        return GetHttpCode(error);
    }

private:
    TString AuthHeader; // usually "OAuth <token>"
    TString AppInfoHeader;
    TMaybe<TString> UserTicketHeader;
    TGlobalContextPtr GlobalCtx;
    const NSc::TValue& Input;
    NSc::TValue& Output;
    const NSc::TValue& ConfigPatch;
};

void MakeIrrelevantResponse(TString& out) {
    NAlice::NScenarios::TScenarioRunResponse irrelevantResponse;
    irrelevantResponse.MutableFeatures()->SetIsIrrelevant(true);
    irrelevantResponse.MutableResponseBody()->Clear();
    Y_PROTOBUF_SUPPRESS_NODISCARD irrelevantResponse.SerializeToString(&out);
}

// Protocol requests
class TMegamindProtocolRunRequest : public TContextedRequest {
public:
    TMegamindProtocolRunRequest(TGlobalContextPtr globalCtx, const NSc::TValue& input,
                                NSc::TValue& output,
                                NAlice::NVideoCommon::TVideoFeatures& features,
                                TStringBuf intentType, TMaybe<TString> searchText,
                                ELanguage language)
        : TContextedRequest(globalCtx, input, output, "bass_protocol_run" /* type */)
        , Features(features)
        , IntentType(intentType)
        , SearchText(searchText)
        , Language(language)
    {
    }

protected:
    TStringBuf LogPrefix() const override {
        return TStringBuf("protocol run ");
    }

    HttpCodes Handle(TContext::TPtr context) override {
        const TString counterName = TString{TStringBuf{context->FormName()}.RNextTok('.')};
        Y_STATS_SCOPE_HISTOGRAM("bass_protocol_run_" + counterName);

        TRequestHandler handler{context};
        NSc::TValue outValue;
        NAlice::NScenarios::TScenarioRunResponse outProto;
        if (const auto error = handler.RunContinuableHandler(outProto, outValue)) {
            error->ToJson(Output);
            LOG(ERR) << "bass protocol run error: " << Output.ToJson() << Endl;
            if (error->Type == TError::EType::PROTOCOL_IRRELEVANT) {
                // TODO(amullanurov@): Add some layout responses for VideoCommand scenario
                TString outString;
                MakeIrrelevantResponse(outString);
                Output = outString;
                return HTTP_OK;
            }
            Y_STATS_INC_COUNTER("bass_protocol_run_fail_" + counterName);
            return GetHttpCode(*error);
        }

        NVideoCommon::CalculateFeaturesAtFinish(
            IntentType,
            SearchText,
            Features,
            NSc::TValue::FromJson(outValue["State"]),
            outValue["IsFinished"].GetBool(),
            Language,
            TBassLogAdapter{}
        );

        *outProto.MutableFeatures()->MutableVideoFeatures() = Features;
        outProto.MutableFeatures()->SetIntent("mm." + TString{IntentType});

        if ((IntentType == NAlice::NVideoCommon::VIDEO_COMMAND_SKIP_VIDEO_FRAGMENT ||
             IntentType == NAlice::NVideoCommon::VIDEO_COMMAND_CHANGE_TRACK) &&
            NVideo::GetCurrentScreen(*context) == NVideo::EScreenId::VideoPlayer)
        {
            outProto.MutableFeatures()->MutablePlayerFeatures()->SetRestorePlayer(true);
            outProto.MutableFeatures()->MutablePlayerFeatures()->SetSecondsSincePause(0);
        }

        TString outString;
        Y_PROTOBUF_SUPPRESS_NODISCARD outProto.SerializeToString(&outString);
        Output = outString;
        LOG(INFO) << "protocol run response: " << outProto << Endl;
        Y_STATS_INC_COUNTER("bass_protocol_run_success_" + counterName);

        return HTTP_OK;
    }

private:
    NAlice::NVideoCommon::TVideoFeatures& Features;
    TStringBuf IntentType;
    TMaybe<TString> SearchText;
    ELanguage Language;
};

class TMegamindCompletionRequest {
public:
    TMegamindCompletionRequest(const TString& authHeader, const TString& appInfoHeader,
                               const TMaybe<TString>& userTicketHeader, TGlobalContextPtr globalCtx,
                               const NSc::TValue& input, NSc::TValue& output, TStringBuf actionName,
                               const NSc::TValue& configPatch)
        : AuthHeader(authHeader)
        , AppInfoHeader(appInfoHeader)
        , UserTicketHeader(userTicketHeader)
        , GlobalCtx(globalCtx)
        , Input(input)
        , Output(output)
        , ActionName(actionName)
        , ConfigPatch(configPatch)
    {
    }

    HttpCodes Process() {
        const TStringBuf name = Input["ObjectTypeName"].GetString();
        const NSc::TValue meta = Input["Meta"];
        Y_STATS_SCOPE_HISTOGRAM(TStringBuilder{} << "bass_protocol_" << ActionName << '_' << name);

        IContinuation::TPtr continuation = GlobalCtx->ContinuationRegistry().MakeFromJson(
            Input, GlobalCtx, meta, AuthHeader, AppInfoHeader, /* fakeTimeHeader = */ {}, UserTicketHeader, ConfigPatch);
        if (!continuation)
            return ReportError({TError::EType::INVALIDPARAM,
                                TStringBuilder() << "Failed to deserialize continuation for protocol " << ActionName},
                               name);

        // Commit is an apply action kind as well.
        if (auto error = continuation->ApplyIfNotFinished())
            return ReportError(std::move(*error), name);

        ToOutput(*continuation);
        Y_STATS_INC_COUNTER(TStringBuilder{} << "bass_protocol_" << ActionName << "_success_" << name);
        return HTTP_OK;
    }

protected:
    virtual void ToOutput(const IContinuation& continuation) = 0;

    template <typename TProto>
    void ProtoToOutput(const TProto& outProto) {
        TString outString;
        Y_PROTOBUF_SUPPRESS_NODISCARD outProto.SerializeToString(&outString);
        Output = std::move(outString);
        LOG(INFO) << "protocol " << ActionName << " response: " << outProto.DebugString() << Endl;
    }

private:
    HttpCodes ReportError(const TError& error, TStringBuf name) {
        error.ToJson(Output);
        LOG(ERR) << "bass protocol " << ActionName << " error: " << Output.ToJson() << Endl;
        Y_STATS_INC_COUNTER(TStringBuilder{} << "bass_protocol_" << ActionName << "_fail_" << name);
        return GetHttpCode(error);
    }

private:
    TString AuthHeader;
    TString AppInfoHeader;
    TMaybe<TString> UserTicketHeader;
    TGlobalContextPtr GlobalCtx;
    const NSc::TValue& Input;
    NSc::TValue& Output;
    TString ActionName;
    const NSc::TValue& ConfigPatch;
};

class TMegamindProtocolCommitRequest final : public TMegamindCompletionRequest {
public:
    TMegamindProtocolCommitRequest(const TString& authHeader, const TString& appInfoHeader,
                                   const TMaybe<TString>& userTicketHeader, TGlobalContextPtr globalCtx,
                                   const NSc::TValue& input, NSc::TValue& output, const NSc::TValue& configPatch)
        : TMegamindCompletionRequest(authHeader, appInfoHeader, userTicketHeader, globalCtx, input, output,
                                     COMMIT_MODE, configPatch)
    {
    }

protected:
    void ToOutput(const IContinuation& continuation) override {
        auto outProto = continuation.AsProtocolCommitResponse();
        ProtoToOutput(outProto);
    }
};

class TMegamindProtocolApplyRequest final : public TMegamindCompletionRequest {
public:
    TMegamindProtocolApplyRequest(const TString& authHeader, const TString& appInfoHeader,
                                  const TMaybe<TString>& userTicketHeader, TGlobalContextPtr globalCtx,
                                  const NSc::TValue& input, NSc::TValue& output, const NSc::TValue& configPatch)
        : TMegamindCompletionRequest(authHeader, appInfoHeader, userTicketHeader, globalCtx, input, output,
                                     APPLY_MODE, configPatch)
    {
    }

protected:
    void ToOutput(const IContinuation& continuation) override {
        auto outProto = continuation.AsProtocolApplyResponse();
        ProtoToOutput(outProto);
    }
};

/** A thread worker to process form from setup request.
 */
class TSetupFormTaskWorker : public IObjectInQueue, NNonCopyable::TNonCopyable {
public:
    using TResult = std::variant<TError, NSc::TValue>;
    using TResultFuture = NThreading::TFuture<TResult>;

public:
    TSetupFormTaskWorker(const NSc::TValue& meta, const NSc::TValue& form,
                         TContext::TInitializer initForContext,
                         TResultFuture* future)
        : Result{NThreading::NewPromise<TResult>()}
        , Meta{meta}
        , Form{form}
        , InitForContext{std::move(initForContext)}
        , RequestLogger(TLogging::RequestLogger)
    {
        Y_ASSERT(future);
        *future = Result.GetFuture();
    }

    ~TSetupFormTaskWorker() {
        if (!Result.HasValue() && !Result.HasException()) {
            Result.SetValue(TError{TError::EType::SYSTEM, "destructor: no result from worker"});
        }
    }

    void Process(void*) override {
        TLogging::SetSubReqId(InitForContext.Id);

        const TStringBuf vinsReqId{Meta["request_id"].GetString()};
        if (vinsReqId.empty()) {
            TLogging::ReqInfo.Get().Update(TLogging::BassReqId.Get());
        } else {
            // Replace BASS reqid (from http-header) with VINS reqid (from request meta)
            const auto& speechKitEvent = InitForContext.SpeechKitEvent;
            if (speechKitEvent.Defined() && speechKitEvent->HasHypothesisNumber()) {
                TLogging::ReqInfo.Get().Update(vinsReqId, speechKitEvent->GetHypothesisNumber());
            } else {
                TLogging::ReqInfo.Get().Update(vinsReqId);
            }
        }
        TLogging::RequestLogger = RequestLogger;

        NSc::TValue request;
        request["meta"] = Meta;
        request["form"] = Form;

        TSetupContext::TPtr ctx;
        if (const auto error = TSetupContext::FromJson(request, InitForContext, &ctx)) {
            Result.SetValue(*error);
            return;
        }

        TStringBuilder counterName;
        counterName << TStringBuf("bass_request_setup_form_");
        counterName << TString{TStringBuf{ctx->FormName()}.RNextTok('.')};

        Y_STATS_SCOPE_HISTOGRAM(counterName);

        NSc::TValue out;
        if (TResultValue error = Run(*ctx, &out)) {
            LOG(ERR) << "setup form handling error: '" << ctx->FormName() << "', " << *error << Endl;
            Result.SetValue(*error);
        } else {
            Result.SetValue(out);
        }
    }

private:
    TResultValue Run(TSetupContext& ctx, NSc::TValue* out) noexcept {
        try {
            const TStringBuf formName{ctx.FormName()};
            const THandlerFactory* handlerFactory = ctx.GlobalCtx().FormHandler(formName);
            if (!handlerFactory) {
                return TError{TError::EType::SYSTEM,
                              TStringBuilder{} << "No setup handler for the form '" << formName << "' found"};
            }

            THolder<IHandler> handler = (*handlerFactory)();
            if (!handler) {
                return TError{TError::EType::SYSTEM, "Unable to create setup handler"};
            }

            TResultValue result;
            try {
                result = handler->DoSetup(ctx);
            } catch (TErrorException& e) {
                result = e.Error();
            } catch (yexception& e) {
                result = TError(
                    TError::EType::SYSTEM,
                    TStringBuilder() << "Caught exception: " << e.what()
                );
            }

            ctx.ToJson(out);
            return result;
        }
        catch (...) {
            return TError{TError::EType::SYSTEM,
                          TStringBuilder{} << "Exception: " << CurrentExceptionMessage()};
        }
    }

private:
    NThreading::TPromise<TResult> Result;
    const NSc::TValue& Meta;
    const NSc::TValue& Form;
    const TContext::TInitializer InitForContext;
    NRTLog::TRequestLogger* RequestLogger;
};

/** Class which handles setup (/setup) requests.
 * It dispatches processing all forms (from request) in separate threads, wait unitl they finish
 * and create response json.
 */
class TSetupRequest : public TBassRequest<NBASSRequest::TSetupRequestConst<TSchemeTraits>> {
public:
    TSetupRequest(IThreadPool& threadPool, TGlobalContextPtr globalCtx, const NSc::TValue& input, NSc::TValue& output)
        : TBassRequest{globalCtx, input, output}
        , ThreadPool{threadPool}
    {
    }

public:
    HttpCodes operator()(const TFail& error) override {
        error.ToJson(Output);
        // These logs will be with BASS reqid (from balancer)
        const TString errorMsg = TStringBuilder{} << "setup context parsing error: " << Output.ToJson();
        LOG(ERR) << errorMsg << Endl;
        Y_STATS_INC_COUNTER("bass_request_setup_fail");

        return HttpCodes::HTTP_BAD_REQUEST;
    }

    HttpCodes operator()(TSuccess& success) override {
        TScheme reqScheme{&Input};
        TVector<TSetupFormTaskWorker::TResultFuture> futures{Reserve(reqScheme.Forms().Size())};

        for (size_t i = 0, total = reqScheme.Forms().Size(); i < total; ++i) {
            auto ctxInit = success;
            ctxInit.Id = ToString(i);

            TSetupFormTaskWorker::TResultFuture future;
            ThreadPool.SafeAddAndOwn(THolder(new TSetupFormTaskWorker{*reqScheme.Meta().GetRawValue(), *reqScheme.Forms()[i].GetRawValue(), std::move(ctxInit), &future}));
            futures.emplace_back(std::move(future));
        }

        class TResponseWriterVisitor {
        public:
            void operator()(const TError& error) {
                ++FailedForms;
                NSc::TValue& form = Forms.Push();
                error.ToJson(form);
            }

            void operator()(const NSc::TValue& output) {
                Forms.Push(output);
            }

            void Write(NSc::TValue* output) const {
                if (!Forms.IsNull()) {
                    (*output)["forms"] = Forms;
                }
            }

        public:
            ui32 FailedForms = 0;

        private:
            NSc::TValue Forms;
        };

        TResponseWriterVisitor formsWriter;

        for (TSetupFormTaskWorker::TResultFuture& future : futures) {
            try {
                const TSetupFormTaskWorker::TResult& result = future.GetValueSync();
                std::visit(formsWriter, result);
            } catch (...) {
                formsWriter(TError{TError::EType::SYSTEM, CurrentExceptionMessage()});
            }
        }

        formsWriter.Write(&Output);

        const TString json{PrintSetupResponse(Output)};
        LOG(INFO) << "setup response: " << json << Endl;

        return HttpCodes::HTTP_OK;
    }

protected:
    TStringBuf LogPrefix() const override {
        return TStringBuf("setup ");
    }

private:
    static TString PrintSetupResponse(const NSc::TValue& value) {
        NSc::TValue ret = value;

        if (NSc::TValue* forms = ret.GetDictMutable().FindPtr("forms"); forms && forms->IsArray()) {
            for (NSc::TValue& form : forms->GetArrayMutable()) {
                form.GetDictMutable().erase("setup_responses");
            }
        }
        return ret.ToJson();
    }

private:
    IThreadPool& ThreadPool;
};

namespace {

const TString VINS_REQID_CLASS{"vins"};
const TString APPLY_REQID_CLASS{"apply"};
const TString PROTOCOL_APPLY_REQID_CLASS{"protocol_apply"};
const TString PROTOCOL_COMMIT_REQID_CLASS{"protocol_commit"};
const TString PROTOCOL_RUN_REQID_CLASS{"protocol_run"};

} // namespace

// TSetupBassRequestHandler ----------------------------------------------------
TSetupBassRequestHandler::TSetupBassRequestHandler(TGlobalContextPtr globalCtx)
    : ThreadPool{CreateThreadPool(globalCtx->Config().SetupThreads())}
{
}

HttpCodes TSetupBassRequestHandler::DoJsonReply(TGlobalContextPtr globalCtx, const NSc::TValue& in,
                                                const TParsedHttpFull& http, const THttpHeaders& headers,
                                                NSc::TValue* out)
{
    NMonitoring::TMsTimerScope scope(&NMonitoring::GetHistogram("bass_request_setup"));
    return TSetupRequest{*ThreadPool, globalCtx, in, *out}.Process(http, headers);
}

// TReportBassRequestHandler ---------------------------------------------------
HttpCodes TReportBassRequestHandler::DoJsonReply(TGlobalContextPtr globalCtx, const NSc::TValue& in,
                                                 const TParsedHttpFull& http, const THttpHeaders& headers,
                                                 NSc::TValue* out)
{
    NMonitoring::TMsTimerScope scope(&NMonitoring::GetHistogram("bass_request_report"));
    return TReportRequest{globalCtx, in, *out}.Process(http, headers);
}

// TMegamindApplyRequestHandler ------------------------------------------------
const TString& TMegamindApplyRequestHandler::GetReqIdClass() const {
    return APPLY_REQID_CLASS;
}

HttpCodes TMegamindApplyRequestHandler::DoJsonReply(TGlobalContextPtr globalCtx, const NSc::TValue& in,
                                                    const TParsedHttpFull& http,
                                                    const THttpHeaders& headers, NSc::TValue* out) {
    NMonitoring::TMsTimerScope scope(&NMonitoring::GetHistogram("bass_request_apply"));
    const auto& authHeader = GetAuthorizationHeader(headers);
    const auto& appInfoHeader = GetAppInfoHeader(headers);
    const NSc::TValue configPatch = GetConfigPatchFromCgiAndHeaders(http, headers);
    TMaybe<TString> userTicketHeader = GetUserTicketHeader(headers);
    return TMegamindApplyRequest{authHeader, appInfoHeader, std::move(userTicketHeader), globalCtx, in, *out, configPatch}
        .Process();
}

// TMegamindProtocolRunRequestHandler ------------------------------------------
const TString& TMegamindProtocolRunRequestHandler::GetReqIdClass() const {
    return PROTOCOL_RUN_REQID_CLASS;
}

HttpCodes TMegamindProtocolRunRequestHandler::DoJsonReply(TGlobalContextPtr globalCtx, const TString& in,
                                                          const TParsedHttpFull& http,
                                                          const THttpHeaders& headers, TString& out) {
    NAlice::NVideoCommon::TVideoFeatures features;
    TStringBuf intentType;
    TMaybe<TString> searchText;

    NAlice::NScenarios::TScenarioRunRequest requestProto;
    Y_PROTOBUF_SUPPRESS_NODISCARD requestProto.ParseFromString(in);
    LOG(INFO) << "initial protocol run request : " << SerializeProtoText(requestProto) << Endl;

    NSc::TValue inValue;
    if (const auto err = NVideoProtocol::CreateBassRunVideoRequest(requestProto, inValue,
        features, intentType, searchText))
    {
        LOG(ERR) << *err << Endl;
        if (err->Type == TError::EType::PROTOCOL_IRRELEVANT) {
            MakeIrrelevantResponse(out);
            return HTTP_OK;
        }
        return HTTP_BAD_REQUEST;
    }

    NMonitoring::TMsTimerScope scope(&NMonitoring::GetHistogram("bass_request_protocol_run"));
    NSc::TValue outValue;

    const auto httpCode = TMegamindProtocolRunRequest{
        globalCtx,
        inValue,
        outValue,
        features,
        intentType,
        searchText,
        LanguageByName(requestProto.GetBaseRequest().GetClientInfo().GetLang())
    }.Process(http, headers);

    out = outValue;
    return httpCode;
}

// TMegamindProtocolCompletionRequestHandler -----------------------------------
HttpCodes TMegamindProtocolCompletionRequestHandler::DoJsonReply(TGlobalContextPtr globalCtx, const TString& in,
                                                                 const TParsedHttpFull& http,
                                                                 const THttpHeaders& headers, TString& out) {
    NAlice::NScenarios::TScenarioApplyRequest requestProto;
    Y_PROTOBUF_SUPPRESS_NODISCARD requestProto.ParseFromString(in);
    LOG(INFO) << "initial protocol " << ActionName << " request : " << SerializeProtoText(requestProto) << Endl;

    NSc::TValue inValue;
    if (const auto err = NVideoProtocol::CreateBassApplyVideoRequest(requestProto, inValue)) {
        LOG(ERR) << *err << Endl;
        return HTTP_BAD_REQUEST;
    }
    NMonitoring::TMsTimerScope scope(
        &NMonitoring::GetHistogram(TStringBuilder{} << "bass_request_protocol_" << ActionName));
    NSc::TValue outValue;
    const auto& authHeader = GetAuthorizationHeader(headers);
    const auto& appInfoHeader = GetAppInfoHeader(headers);
    const NSc::TValue configPatch = GetConfigPatchFromCgiAndHeaders(http, headers);
    const auto userTicketHeader = GetUserTicketHeader(headers);
    const auto httpCode = Process(authHeader, appInfoHeader, userTicketHeader, globalCtx, inValue, outValue, configPatch);
    out = outValue;
    return httpCode;
}

// TMegamindProtocolCommitRequestHandler ----------------------------------------
const TString& TMegamindProtocolCommitRequestHandler::GetReqIdClass() const {
    return PROTOCOL_COMMIT_REQID_CLASS;
}

HttpCodes TMegamindProtocolCommitRequestHandler::Process(const TString& authHeader, const TString& appInfoHeader,
                                                         const TMaybe<TString>& userTicketHeader,
                                                         TGlobalContextPtr globalCtx, const NSc::TValue& input,
                                                         NSc::TValue& output, const NSc::TValue& configPatch) const {
    return TMegamindProtocolCommitRequest{authHeader, appInfoHeader, userTicketHeader, globalCtx, input, output, configPatch}
        .Process();
}

// TMegamindProtocolApplyRequestHandler ----------------------------------------
const TString& TMegamindProtocolApplyRequestHandler::GetReqIdClass() const {
    return PROTOCOL_APPLY_REQID_CLASS;
}

HttpCodes TMegamindProtocolApplyRequestHandler::Process(const TString& authHeader, const TString& appInfoHeader,
                                                        const TMaybe<TString>& userTicketHeader,
                                                        TGlobalContextPtr globalCtx, const NSc::TValue& input,
                                                        NSc::TValue& output, const NSc::TValue& configPatch) const {
    return TMegamindProtocolApplyRequest{authHeader, appInfoHeader, userTicketHeader, globalCtx, input, output, configPatch}
        .Process();
}

// TMegamindPrepareRequestHandler ----------------------------------------------
HttpCodes TMegamindPrepareRequestHandler::DoJsonReply(TGlobalContextPtr globalCtx, const NSc::TValue& in,
                                                      const TParsedHttpFull& http, const THttpHeaders& headers,
                                                      NSc::TValue* out) {
    NMonitoring::TMsTimerScope scope(&NMonitoring::GetHistogram("bass_request_prepare"));
    return TMegamindPrepareRequest{globalCtx, in, *out}.Process(http, headers);
}

// TBassRequestHandler ---------------------------------------------------------
const TString& TBassRequestHandler::GetReqIdClass() const {
    return VINS_REQID_CLASS;
}

// static
void TBassRequestHandler::RegisterHttpHandlers(THttpHandlersMap* handlers, TGlobalContextPtr globalCtx) {
    Y_ASSERT(handlers);

    handlers->Add(TStringBuf("/vins"), []() {
        static const IHttpRequestHandler::TPtr handler = MakeIntrusive<TReportBassRequestHandler>();
        return handler;
    });
    handlers->Add(TStringBuf("/setup"), [globalCtx]() {
        static const IHttpRequestHandler::TPtr handler = MakeIntrusive<TSetupBassRequestHandler>(globalCtx);
        return handler;
    });
    handlers->Add(TStringBuf("/admin/update_yastroka_fixlist"), []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TRawRequestHandler<&TApplication::ReloadYaStrokaFixList>>();
        return handler;
    });
    handlers->Add(TStringBuf("/admin/update_bno_apps"), []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TRawRequestHandler<&TApplication::ReloadBnoApps>>();
        return handler;
    });
    handlers->Add(TStringBuf("/megamind/prepare"), []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TMegamindPrepareRequestHandler>();
        return handler;
    });
    handlers->Add(TStringBuf("/megamind/apply"), []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TMegamindApplyRequestHandler>();
        return handler;
    });
    handlers->Add(TStringBuf("/megamind/protocol/video_general/run"), []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TMegamindProtocolRunRequestHandler>();
        return handler;
    });
    handlers->Add(TStringBuf("/megamind/protocol/video_general/commit"), []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TMegamindProtocolCommitRequestHandler>();
        return handler;
    });
    handlers->Add(TStringBuf("/megamind/protocol/video_general/apply"), []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TMegamindProtocolApplyRequestHandler>();
        return handler;
    });
    handlers->Add(TStringBuf("/megamind/protocol/vins/commit"), []() {
      static IHttpRequestHandler::TPtr handler = MakeIntrusive<TMegamindProtocolCommitRequestHandler>();
      return handler;
    });

}

} // namespace NBASS
