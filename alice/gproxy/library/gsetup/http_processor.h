#pragma once

#include "frame_converter.h"
#include "metadata.h"

#include <alice/gproxy/library/events/gproxy.ev.pb.h>
#include <alice/gproxy/library/gsetup/subsystem_logging.h>
#include <alice/gproxy/library/protos/gsetup.pb.h>
#include <alice/gproxy/library/protos/service.gproxy.pb.h>

#include <alice/library/json/json.h>
#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/fwd.h>

namespace NGProxy {

class TBasicProcessor {
public:
    virtual ~TBasicProcessor() = default;
    virtual void ProcessHttpInit() = 0;
    virtual void ProcessHttpOutput() =0;
};

/**
    *  @brief Generic processor of http path
    *  All options are located in Method template parameter.
    *  Do not write Method template parameter manually, it is generated from service description in alice/protos/service.proto.
    */
template <class TMethod>
class THttpPathProcessor: public TBasicProcessor {
public:
    using TThisType = THttpPathProcessor<TMethod>;
    using TRequestType = typename TMethod::InputType;
    using TResponseType = typename TMethod::OutputType;

    THttpPathProcessor(NAppHost::IServiceContext& ctx, TLogContext& logContext)
        : Ctx_(ctx)
        , LogContext_(logContext)
    {
                LogEvent("Initializing", "Init");
                // Metrics.PushRate("call", "created", TMethod::MethodName);
    }

    virtual ~THttpPathProcessor() {
        Destroy();
        // LogContext_.LogEventInfoCombo<NEvClass::AsyncCallDelete>(reinterpret_cast<uint64_t>(this));
    }

    virtual void ProcessHttpInit() override {
        InitImpl();
    }

    virtual void ProcessHttpOutput() override {
        OutputImpl();
    }

private: /* methods */
    inline void LogEvent(TStringBuf state, TStringBuf event) {
        LogContext_.LogEventInfoCombo<NEvClass::AsyncCallEvent>(
            reinterpret_cast<uint64_t>(this),
            TMethod::MethodName,
            TString(state),
            TString(event)
        );
    }

    inline void LogAppHostError(TStringBuf reqId, TStringBuf message) {
        LogContext_.LogEventInfoCombo<NEvClass::GrpcApphostError>(TString(reqId), TString(message));
    }

    inline void AddError(
        NAppHost::IServiceContext& ctx,
        const TString& method,
        const TString& component,
        const TString& message,
        TLogContext& logContext
    ) {
        NGProxy::GSetupError error;
        error.SetMethod(method);
        error.SetComponent(component);
        error.SetMessage(message);
        ctx.AddProtobufItem(error, "error");

        logContext.LogEventInfoCombo<NEvClass::GSetupError>(message);
    }

    inline void LogAppHostParams(const NJson::TJsonValue& params) {
        TStringStream ss;
        ss << params;
        LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(ss.Str());
    }

    inline void SetupRequest(TMetadata& meta, GSetupRequestInfo& info) {
        if (!Ctx_.HasProtobufItem("proto_http_request")) {
            AddError(Ctx_, "unknown", "GSETUP_HTTP_INIT", "no http_request item in apphost context", LogContext_);
            return;
        }
        const auto& http_request = Ctx_.GetOnlyProtobufItem<NAppHostHttp::THttpRequest>("proto_http_request");

        auto requestPath = http_request.GetPath().substr(0, http_request.GetPath().find('?'));

        FillMetadata(http_request.GetHeaders(), meta, LogContext_);

        if (auto rtlogger = LogContext_.RtLogPtr()) {
            NRTLogEvents::CreateRequestContext ctx;
            (*ctx.MutableFields())["grpcproxy_session_id"] = meta.GetSessionId();
            (*ctx.MutableFields())["grpcproxy_request_id"] = meta.GetRequestId();
            (*ctx.MutableFields())["uuid"] = meta.GetUuid();
            (*ctx.MutableFields())["device_id"] = meta.GetDeviceId();
            rtlogger->LogEvent(ctx);
        }

        info.SetMethod(TString(TMethod::MethodName));
        info.SetSemanticFrameName(TString(TMethod::SemanticFrameName));

        LogContext_.LogEventInfoCombo<NEvClass::GProxyPayload>("grpc.method", TMethod::MethodName);

        if (meta.GetExtraLogging()) {
            LogContext_.LogEventInfoCombo<NEvClass::GProxyPayload>("grpc.request", Request_.DebugString());
        }

        bool successParse = Request_.ParseFromString(http_request.content());
        if (!successParse) {
            AddError(Ctx_, "unknown", "GSETUP_HTTP_INIT", "cannot parse request proto", LogContext_);
            return;
        }
    }

    inline void ReplyError(const TString& message) {
        ReplyError(500, message);
    }

    inline void ReplyError(const unsigned statusCode, const TString& message) {
        NAppHostHttp::THttpResponse httpResponse;
        httpResponse.SetStatusCode(statusCode);
        httpResponse.SetContent(message);

        Ctx_.AddProtobufItem(httpResponse, "proto_http_response");
    }

    inline void ReplyError(const NGProxy::GSetupError& item) {
        TStringStream ss;
        item.PrintJSON(ss);


        ReplyError(512u, ss.Str());
    }

    inline void ProcessResponse(bool extraLogging = false) {
        if (TMethod::UseRawRequestResponse) {
            const auto& responseAny = Ctx_.GetOnlyProtobufItem<google::protobuf::Any>("response");
            if (!responseAny.Is<TResponseType>()) {
                LogContext_.LogEventInfoCombo<NEvClass::GProxyError>("GPROXY", "wrong response type in Any");
                ReplyError("(gproxy) wrong response type in Any");
                return;
            }
            responseAny.UnpackTo(&Response_);
        } else {
            const auto& item = Ctx_.GetOnlyProtobufItem<NGProxy::GSetupResponse>("response");
            if (!item.HasData()) {
                LogContext_.LogEventInfoCombo<NEvClass::GProxyError>("GPROXY", "no data in response");
                ReplyError("(gproxy) no data in response");
                return;
            }
            const TString& data = item.GetData();
            if (data.StartsWith('{')) {
                NJson::TJsonValue json = NAlice::JsonFromString(data);
                google::protobuf::util::Status st = NAlice::JsonToProto(json, Response_, true, true);
                if (!st.ok()) {
                    LogContext_.LogEventInfoCombo<NEvClass::GProxyError>("GPROXY", "failed to parse proto from json response");
                    ReplyError(
                        st.code() == google::protobuf::util::status_internal::StatusCode::kNotFound ? 404 : 500,
                        "(gproxy) failed to parse proto from json response: "
                    );
                    return;
                }
            } else {
                // base64-encoded protobuf content
                TString bin = Base64Decode(data);
                if (!Response_.ParseFromString(bin)) {
                    LogContext_.LogEventInfoCombo<NEvClass::GProxyError>("GPROXY", "failed to parse proto from base64 response");
                    ReplyError("(gproxy) failed to parse proto from base64 response");
                    return;
                }
            }
        }

        if (extraLogging) {
            LogContext_.LogEventInfoCombo<NEvClass::GProxyPayload>("grpc.response", Response_.DebugString());
        }

        NAppHostHttp::THttpResponse httpResponse;
        httpResponse.SetStatusCode(200);
        httpResponse.SetContent(this->Response_.SerializeAsString());
        Ctx_.AddProtobufItem(httpResponse, "proto_http_response");
    };

    inline void InitImpl() {
        LogEvent("Executing", "RequestReceived");

        TMetadata meta;
        GSetupRequestInfo info;
        SetupRequest(meta, info);

        const TString childToken = LogContext_.RtLog()->LogChildActivationStarted(false, TMethod::GraphName);

        LogEvent("Executing", "AppHostRequest");

        if (TMethod::UseRawRequestResponse) {
            google::protobuf::Any wrapped;
            wrapped.PackFrom(Request_);
            Ctx_.AddProtobufItem(wrapped, "request");
        } else {
            GSetupRequest wrapped;
            wrapped.SetData(ConvertToFrame<TMethod, TRequestType>(Request_));
            Ctx_.AddProtobufItem(wrapped, "request");
        }

        if (TMethod::GraphName == "gproxy_common") {
            Ctx_.AddFlag("common_call");
        } else if (TMethod::GraphName == "gproxy_mm_rpc") {
            Ctx_.AddFlag("mm_rpc_call");
        } else {
            Ctx_.AddFlag("gproxy_call");
        }

        LogContext_.LogEventInfoCombo<NEvClass::GSetupRequestInfo>(
            meta.GetSessionId(),
            meta.GetRequestId(),
            meta.GetUuid(),
            info.GetMethod(),
            "GSETUP_HTTP_INIT"
        );

        Ctx_.AddProtobufItem(meta, "metadata");
        Ctx_.AddProtobufItem(info, "request_info");

        for (auto flag : TMethod::ApphostFlags) {
            Ctx_.AddFlag(ToString(flag));
        }
    }

    inline void OutputImpl() {
        const auto& childToken = Ctx_.GetOnlyItem(NAppHost::APP_HOST_PARAMS_TYPE)["reqid"].GetString();

        try {
            if (!Ctx_.HasProtobufItem("response")) {
                ReplyError(512, "No response item in apphost context");
            } else if (Ctx_.HasProtobufItem("error")) {
                // const auto& error = Ctx_.FindFirstItem("error");
                // TStringStream ss;
                // ss << error;
                ReplyError(512, "Some errors");
            } else {
                const auto& meta = Ctx_.GetOnlyProtobufItem<NGProxy::TMetadata>("metadata");
                LogContext_.RtLog()->LogChildActivationFinished(childToken, true);
                ProcessResponse(meta.GetExtraLogging());
            }

        } catch (const std::exception& ex) {
            LogContext_.RtLog()->LogChildActivationFinished(childToken, false, ex.what());
            LogContext_.LogEventInfoCombo<NEvClass::GProxyError>("GPROXY", "no required output chunks");
            ReplyError(500, ex.what());
        }
    }

    inline TThisType& StartWaitingForRequest() {
        // TMethod::Request(Service_.Get(), &ServerContext_, &Request_, &ResponseWriter_, Service_.GetQueue(), AbstractThis());
        return *this;
    }

    inline THttpPathProcessor* AbstractThis() {
        // do not use dynamic cast here
        // do not call virtual methods here
        return static_cast<TBasicProcessor*>(this);
    }

    inline void Destroy() {
        delete this;
    }

private: /* data */
    TRequestType Request_;
    TResponseType Response_;

    // NVoice::NMetrics::TScopeMetrics Metrics;
    NAppHost::IServiceContext& Ctx_;
    TLogContext& LogContext_;
};

using GProxyService = NGProxyTraits::TGProxyService<::NGProxy::GProxy>;
class THttpProcessorFabric {
public:
    static std::unique_ptr<TBasicProcessor> GetProcessorForHttpPath(const TString& httpPath, NAppHost::IServiceContext& ctx, TLogContext& logContext) {
        if (httpPath == GProxyService::TServiceMethodExample::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodExample>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodGetTvCardDetail::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodGetTvCardDetail>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodGetTvFeatureBoarding::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodGetTvFeatureBoarding>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodGetTvFeatureBoarding::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodReportTvFeatureBoardingTemplateShown>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodGetFakeGalleries::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodGetFakeGalleries>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodGetFake2Galleries::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodGetFake2Galleries>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodGetTvSearchResult::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodGetTvSearchResult>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodGetTvSearchResultRpc::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodGetTvSearchResultRpc>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodGetCatalogTags::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodGetCatalogTags>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodTvWatchListAdd::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodTvWatchListAdd>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodTvWatchListDelete::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodTvWatchListDelete>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodTvGetGalleries::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodTvGetGalleries>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodTvGetGallery::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodTvGetGallery>>(ctx, logContext);
        } else if (httpPath == GProxyService::TServiceMethodTvCheckChannelLicense::HttpPath) {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodTvCheckChannelLicense>>(ctx, logContext);
        } else {
            return std::make_unique<THttpPathProcessor<GProxyService::TServiceMethodExample>>(ctx, logContext);
        }
    }
};

} // namespace NGProxy