#pragma once

#include "async_call_state.h"
#include "async_service.h"
#include "frame_converter.h"
#include "metadata.h"
#include "test_devices.h"

#include <alice/gproxy/library/events/gproxy.ev.pb.h>
#include <alice/gproxy/library/protos/gsetup.pb.h>
#include <alice/gproxy/library/protos/service.gproxy.pb.h>

#include <alice/library/json/json.h>
#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/list.h>
#include <util/stream/str.h>

#include <grpcpp/alarm.h>
#include <grpcpp/server.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/time.h>
#include <grpcpp/support/error_details.h>

#include <contrib/libs/googleapis-common-protos/google/rpc/status.pb.h>

namespace NGProxy {

    using NAlice::NCuttlefish::SaveRtLogInfoEvent;

    using NAlice::NCuttlefish::SaveRtLogErrorEvent;

    class IAsyncCall {
    public:
        virtual ~IAsyncCall() = default;
        virtual void Do() = 0;
    };

    using TOutputDataChunk = ::NAppHost::NClient::TOutputDataChunk;

    template <typename TRequest>
    class TAsyncCallContext {
    public:
        TAsyncCallContext(::NAppHost::NClient::TStream&& stream)
            : Stream_(std::move(stream))
        {
        }

        TAsyncCallContext(::NAppHost::NClient::TStream&& stream, NJson::TJsonValue params)
            : Stream_(std::move(stream))
            , Params_(std::move(params))
        {
        }

        NThreading::TFuture<TMaybe<TOutputDataChunk>> StartRequest(const TMetadata& metadata, const GSetupRequestInfo& info, const NProtoBuf::Message& request, const std::initializer_list<TStringBuf>& flags) {
            ::NAppHost::NClient::TInputDataChunk input;

            input.AddItem("INIT", "metadata", metadata);
            input.AddItem("INIT", "request_info", info);
            input.AddItem("INIT", "request", request);
            for (auto flag : flags) {
                input.AddFlag("INIT", ToString(flag));
            }
            input.AddItem(NAppHost::APP_HOST_PARAMS_SOURCE_NAME, NAppHost::APP_HOST_PARAMS_TYPE, Params_);

            Stream_.Write(input, true);
            Stream_.WritesDone();

            return Stream_.ReadAll();
        }

    private:
        ::NAppHost::NClient::TStream Stream_;
        ::NJson::TJsonValue Params_;
    };

    /**
     *  @brief Generic async call of AppHost graph
     *  AppHost vertical name, graph name, input and output are located in Method template parameter.
     *  Do not write Method template parameter manually, it is generated from service description in alice/protos/service.proto.
     */
    template <class TMethod>
    class TAsyncCall: public IAsyncCall {
    public:
        using TThisType = TAsyncCall<TMethod>;
        using TRequestType = typename TMethod::InputType;
        using TResponseType = typename TMethod::OutputType;

        TAsyncCall(TAsyncService& service)
            : Service_(service)
            , ResponseWriter_(&ServerContext_)
            , Metrics(service.GetMetrics().BeginGrpcScope())
            , LogContext_(service.GetLogger().CreateLogContext())
        {
            LogContext_.LogEventInfoCombo<NEvClass::AsyncCallNew>(reinterpret_cast<uint64_t>(this));
            DoImpl();
        }

        virtual ~TAsyncCall() {
            LogContext_.LogEventInfoCombo<NEvClass::AsyncCallDelete>(reinterpret_cast<uint64_t>(this));
        }

        virtual void Do() override {
            DoImpl();
        }

    private: /* methods */
        inline void LogEvent(TStringBuf state, TStringBuf event) {
            LogContext_.LogEventInfoCombo<NEvClass::AsyncCallEvent>(reinterpret_cast<uint64_t>(this), TMethod::MethodName, TString(state), TString(event));
        }

        inline void LogAppHostError(TStringBuf reqId, TStringBuf message) {
            LogContext_.LogEventInfoCombo<NEvClass::GrpcApphostError>(TString(reqId), TString(message));
        }

        inline void LogAppHostParams(const NJson::TJsonValue& params) {
            TStringStream ss;
            ss << params;
            LogContext_.LogEventInfoCombo<NEvClass::GrpcApphostParams>(ss.Str());
        }

        inline void SetupRequest(TMetadata& meta, GSetupRequestInfo& info) {
            if (!FillMetadata(ServerContext_.client_metadata(), meta, LogContext_)) {
                Metrics.PushRate("call", "bad_metadata", TMethod::MethodName);
            }

            LogContext_ = Service_.GetLogger().CreateLogContext(
                meta.HasRtLogToken() ? meta.GetRtLogToken() : ""
            );

            if (auto rtlogger = LogContext_.RtLogPtr()) {
                NRTLogEvents::CreateRequestContext ctx;
                (*ctx.MutableFields())["grpcproxy_session_id"] = meta.GetSessionId();
                (*ctx.MutableFields())["grpcproxy_request_id"] = meta.GetRequestId();
                (*ctx.MutableFields())["uuid"] = meta.GetUuid();
                (*ctx.MutableFields())["device_id"] = meta.GetDeviceId();
                rtlogger->LogEvent(ctx);
            }

            LogContext_.LogEventInfoCombo<NEvClass::GrpcHeader>("X-Grpc-Peer", ServerContext_.peer());

            info.SetMethod(TString(TMethod::MethodName));
            info.SetSemanticFrameName(TString(TMethod::SemanticFrameName));

            LogContext_.LogEventInfoCombo<NEvClass::GProxyPayload>("grpc.method", TMethod::MethodName);

            if (meta.GetExtraLogging()) {
                LogContext_.LogEventInfoCombo<NEvClass::GProxyPayload>("grpc.request", Request_.DebugString());
            }
        }

        inline void ReplyError(const TString& message) {
            ResponseWriter_.Finish(Response_, grpc::Status(grpc::INTERNAL, message), AbstractThis());
        }

        inline void ReplyError(const ::NAppHost::NClient::TOutputItem& item) {
            GSetupError resp;

            item.ParseProtobuf(resp);

            TStringStream ss;
            resp.PrintJSON(ss);

            ReplyError(ss.Str());
        }

        inline void ReplyError(const google::rpc::Status& grpcError) {
            grpc::Status replyStatus(grpc::StatusCode(grpcError.code()), grpcError.message());
            if (grpcError.details_size()) {
                SetErrorDetails(grpcError, &replyStatus);
            }
            ResponseWriter_.Finish(Response_, replyStatus, AbstractThis());
        }

        inline void ReplyErrors(const TList<::NAppHost::NClient::TOutputItem>& errors) {
            GSetupError resp;
            NJson::TJsonValue out;

            for (const auto& err : errors) {
                NJson::TJsonValue item;
                err.ParseProtobuf(resp);
                item["component"] = resp.GetComponent();
                item["message"] = resp.GetMessage();
                out.AppendValue(item);
                LogContext_.LogEventInfoCombo<NEvClass::GProxyError>(resp.GetComponent(), resp.GetMessage());
            }

            ReplyError(NJson::WriteJson(out, false));
        }

        inline void Reply(const ::NAppHost::NClient::TOutputItem& item, bool extraLogging = false) {
            if (TMethod::UseRawRequestResponse) {
                google::protobuf::Any responseAny;
                item.ParseProtobuf(responseAny);
                if (!responseAny.Is<TResponseType>()) {
                    LogContext_.LogEventInfoCombo<NEvClass::GProxyError>("GPROXY", "wrong response type in Any");
                    ReplyError("(gproxy) wrong response type in Any");
                    return;
                }
                responseAny.UnpackTo(&Response_);
            } else {
                GSetupResponse resp;
                item.ParseProtobuf(resp);

                if (!resp.HasData()) {
                    LogContext_.LogEventInfoCombo<NEvClass::GProxyError>("GPROXY", "no data in response");
                    ReplyError("(gproxy) no data in response");
                    return;
                }
                const TString& data = resp.GetData();
                if (data.StartsWith('{')) {
                    // json content
                    NJson::TJsonValue json = NAlice::JsonFromString(data);
                    google::protobuf::util::Status st = NAlice::JsonToProto(json, Response_, true, true);
                    if (!st.ok()) {
                        LogContext_.LogEventInfoCombo<NEvClass::GProxyError>("GPROXY", "failed to parse proto from json response");
                        ReplyError("(gproxy) failed to parse proto from json response");
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

            ResponseWriter_.Finish(this->Response_, grpc::Status::OK, AbstractThis());
        }

        inline void ProcessResponse(const ::NAppHost::NClient::TOutputDataChunk* output, bool extraLogging = false) {
            if (!output) {
                LogContext_.LogEventInfoCombo<NEvClass::GProxyError>("GPROXY", "empty response");
                ReplyError("(gproxy) empty response");
                return;
            }

            TList<::NAppHost::NClient::TOutputItem> err;
            TMaybe<::NAppHost::NClient::TOutputItem> out;
            TMaybe<::NAppHost::NClient::TOutputItem> grpcErr;

            for (const auto& item : output->GetAllItems()) {
                if (item.GetType() == "response") {
                    out = item;
                } else if (item.GetType() == "error") {
                    err.push_back(item);
                } else if (item.GetType() == "grpc_error") {
                    grpcErr = item;
                } else if (item.GetType() == "__json_dump") {
                    NJson::TJsonValue value = item.ParseJson();
                    Cerr << "(gproxy) __json_dump: " << value << Endl;
                }
            }

            if (out) {
                Reply(*out, extraLogging);
            } else if (!err.empty()) {
                ReplyErrors(err);
            } else if (grpcErr) {
                google::rpc::Status status;
                grpcErr->ParseProtobuf(status);
                LogContext_.LogEventInfoCombo<NEvClass::GProxyError>("GPROXY", "reply with grpc_error item");
                ReplyError(status);
            } else {
                LogContext_.LogEventInfoCombo<NEvClass::GProxyError>("GPROXY", "no required output chunks");
                ReplyError("no required output chunks");
            }
        };

        inline void DoImpl() {
            switch (State_) {
                case EAsyncCallState::Initializing:
                    Cerr << "DoImpl()" << Endl;
                    LogEvent("Initializing", "Init");
                    Metrics.PushRate("call", "created", TMethod::MethodName);
                    StartWaitingForRequest().SetState(EAsyncCallState::Executing);
                    break;

                case EAsyncCallState::Executing: {
                    LogEvent("Executing", "RequestReceived");
                    ScheduleNext();

                    TMetadata meta;
                    GSetupRequestInfo info;
                    SetupRequest(meta, info);

                    const TString childToken = LogContext_.RtLog()->LogChildActivationStarted(false, TMethod::GraphName);

                    {
                        const bool isTestDev = IsTestDevice(meta.deviceid());
                        const bool allowSrcRwr = Service_.GetAllowSrcrwr() || isTestDev;
                        const bool allowDumpRR = Service_.GetAllowDumpRequestsResponses() || isTestDev;
                        NJson::TJsonValue appHostParams = CreateAppHostParams(
                            ServerContext_.client_metadata(),
                            allowSrcRwr,
                            allowDumpRR,
                            childToken
                        );

                        LogAppHostParams(appHostParams);

                        CallContext_ = MakeHolder<TAsyncCallContext<TRequestType>>(
                            Service_.GetAppHost().CreateStream("VOICE", TMethod::GraphName, TMethod::GraphTimeout),
                            std::move(appHostParams)
                        );
                    }

                    LogEvent("Executing", "AppHostRequest");

                    NThreading::TFuture<TMaybe<TOutputDataChunk>> future;
                    if (TMethod::UseRawRequestResponse) {
                        google::protobuf::Any wrapped;
                        wrapped.PackFrom(Request_);
                        future = CallContext_->StartRequest(meta, info, wrapped, TMethod::ApphostFlags);
                    } else {
                        GSetupRequest req;
                        req.SetData(ConvertToFrame<TMethod, TRequestType>(Request_));
                        future = CallContext_->StartRequest(meta, info, req, TMethod::ApphostFlags);
                    }
                    this->SetState(EAsyncCallState::Waiting);
                    future.Subscribe([this, meta, childToken](const NThreading::TFuture<TMaybe<TOutputDataChunk>>& result) {
                        LogEvent("Waiting", "AppHostResponse");

                        try {
                            const TOutputDataChunk* output = result.GetValueSync().Get();
                            LogContext_.RtLog()->LogChildActivationFinished(childToken, true);
                            ProcessResponse(output, meta.GetExtraLogging());
                        } catch (const std::exception& ex) {
                            LogContext_.RtLog()->LogChildActivationFinished(childToken, false, ex.what());
                            LogAppHostError(meta.GetRequestId(), ex.what());
                            ReplyError(ex.what());
                        }

                        SetState(EAsyncCallState::Completing);
                    });
                } break;

                case EAsyncCallState::Waiting:
                    break;

                case EAsyncCallState::Completing:
                    LogEvent("Completing", "Destroy");
                    Metrics.PushRate("call", "complete", TMethod::MethodName);
                    SetState(EAsyncCallState::Finished).Destroy();
                    break;

                case EAsyncCallState::Finished:
                    /* should be unreachable */
                    break;
            }
        }

        inline TThisType& SetState(EAsyncCallState state) {
            State_ = state;
            return *this;
        }

        inline TThisType& StartWaitingForRequest() {
            TMethod::Request(Service_.Get(), &ServerContext_, &Request_, &ResponseWriter_, Service_.GetQueue(), AbstractThis());
            return *this;
        }

        inline IAsyncCall* AbstractThis() {
            // do not use dynamic cast here
            // do not call virtual methods here
            return static_cast<IAsyncCall*>(this);
        }

        inline void Destroy() {
            delete this;
        }

        inline TThisType& ScheduleNext() {
            new TThisType(Service_);
            return *this;
        }

    private: /* data */
        EAsyncCallState State_{EAsyncCallState::Initializing};
        TAsyncService& Service_;
        grpc::ServerContext ServerContext_;

        TRequestType Request_;
        TResponseType Response_;

        THolder<TAsyncCallContext<TRequestType>> CallContext_;

        grpc::ServerAsyncResponseWriter<TResponseType> ResponseWriter_;
        NVoice::NMetrics::TScopeMetrics Metrics;

        TLogContext LogContext_;
    };

} // namespace NGProxy
