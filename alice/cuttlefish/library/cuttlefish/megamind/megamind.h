#pragma once

#include <alice/cuttlefish/library/cuttlefish/megamind/request/request_builder.h>
#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/service.h>
#include <alice/cuttlefish/library/cuttlefish/megamind/speaker/service_apphosted.h>

#include "state_machine.h"
#include "worker.h"

#include <alice/cuttlefish/library/apphost/async_input_request_handler.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/protos/bio_context_save.pb.h>
#include <alice/cuttlefish/library/protos/megamind.pb.h>
#include <alice/cuttlefish/library/protos/yabio.pb.h>
#include <apphost/api/service/cpp/service.h>
#include <voicetech/asr/engine/proto_api/response.pb.h>

#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <alice/protos/api/meta/backend.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>

#include <deque>

namespace NAlice::NCuttlefish::NAppHostServices {

// servant proxy apphost stream-request contain mulitple requests to MM (see TSubrequest)
class TMegamindServant : public TThrRefBase {
public:
    virtual ~TMegamindServant() = default;
    virtual void OnCreate() {}
    virtual void OnCancelRequest() {}
    void OnNextInputSafe();
    void OnNextInput();
    void SubscribeInput(const TString& comment);
    virtual void OnReceiveRequestContext(const NAliceProtocol::TRequestContext&) {}

    inline NThreading::TPromise<void> Promise() {
        return RequestHandler.Promise();
    }

protected:
    TMegamindServant(
        const NAliceCuttlefishConfig::TConfig& config,
        NAppHost::TServiceContextPtr serviceCtx,
        TLogContext logContext,
        const TString& sourceName
    );

    TInputAppHostAsyncRequestHandler& AhRequestHander() {
        return RequestHandler;
    }
    inline NAppHost::IServiceContext& AhContext() {
        return RequestHandler.Context();
    }

    bool CanSendRequest() const;
    void SendRequest(const TSubrequestPtr&);
    void TryAddSingleContextSaveRequest(
        const NAliceProtocol::TMegamindResponse& mmResponse,
        const TStringBuf contextSaveRequestItemType,
        const TStringBuf contextSaveNeedFullIncomingAudioFlag,
        const bool isImportant
    );
    void TryAddContextSaveRequest(const NAliceProtocol::TMegamindResponse&);
    void TryAddSessionSaveRequest(const NAliceProtocol::TMegamindResponse&);

protected:
    virtual ERequestPhase GetRequestPhase() const = 0;
    virtual bool ProcessNextInput() = 0;  // return false if not need more input from apphost
    virtual void OnContextsLoaded() = 0;
    virtual void OnError(const TString& error, const TString& errorCode);
    virtual void OnInternalError(const TString& error, const TString& errorCode);
    virtual void OnSubrequestError(TSubrequest&, const TString& error, const TString& errorCode);
    virtual void OnSubrequestResponse(TSubrequest&, const NAliceProtocol::TMegamindResponse&);
    virtual void OnHasFinalResult(TSubrequest&) = 0;
    void TrySetupExperiments();
    void ProcessUsedMegamindResponse(const NAliceProtocol::TMegamindResponse&);  // process here useful response (from apply or eou, if apply not used)

    template <typename TTypedDirective, typename TEnrich>
    void AddBioContextUpdate(const NSpeechKit::TDirective&, TEnrich);
    void AddUserToBioContext(const NSpeechKit::TDirective& directive);
    void RemoveUserFromBioContext(const NSpeechKit::TDirective& directive);

public:
    void AddRequestDebugInfo(NAliceProtocol::TRequestDebugInfo&&);
    void AddTtsRequestIfNeed(const NAliceProtocol::TMegamindResponse&);
    void AddMegamindRunReady();
    void SendSessionLogDirective(NJson::TJsonValue&& sessionLogValue, const TString& type);

protected:
    template <typename TFlushAndFinishEvent>
    void FlushAndFinish();
    template <typename TFlushAndFinishEvent>
    void SafeFlushAndFinish();

    template <typename TIntermediateFlushEvent>
    void IntermediateFlush();
    template <typename TIntermediateFlushEvent>
    void SafeIntermediateFlush();

protected:
    bool FinalSubrequestSended = false;
    bool NoVinsRequests_{false};

    const NAliceCuttlefishConfig::TConfig& Config;
    TMutex MutexWriteToContext;
    bool AhContextFlushed = false;
    TInputAppHostAsyncRequestHandler RequestHandler;
    TLogContext LogContext;  //TODO: check eventlog thread safed (created with required option)
    TSourceMetrics Metrics;

    TMegamindClientPtr MegamindClient;
    TMegamindRequestBuilderPtr RequestBuilder;

    // these items are needed by MegamindClient
    TMaybe<NJson::TJsonValue> AppHostParams;
    TMaybe<NAliceProtocol::TSessionContext> SessionCtx;
    TMaybe<NAliceProtocol::TRequestContext> RequestCtx;
    TMaybe<NAliceProtocol::TMegamindRequest> Request;
    TMaybe<NAliceProtocol::TContextLoadResponse> ContextLoadResponse;
    TApphostedSpeakerService<TSpeakerService> SpeakerService;
    NAlice::TLoggerOptions AliceLoggerOptions;  // default object is valid.
};

using TMegamindServantPtr = TIntrusivePtr<TMegamindServant>;


class TMegamindRunServant : public TMegamindServant {
public:
    static TMegamindServantPtr Create(const NAliceCuttlefishConfig::TConfig& cfg, NAppHost::TServiceContextPtr serviceCtx, TLogContext logContext) {
        TMegamindServantPtr ptr{new TMegamindRunServant(cfg, std::move(serviceCtx), std::move(logContext))};
        ptr->OnCreate();
        return ptr;
    }

    TMegamindRunServant(const NAliceCuttlefishConfig::TConfig& config, NAppHost::TServiceContextPtr serviceCtx, TLogContext logContext)
        : TMegamindServant(config, serviceCtx, logContext, "megamind_run")
        , StateMachine_(*this, Metrics)
    {}

    void OnReceiveRequestContext(const NAliceProtocol::TRequestContext&) override;

private:
    using TMegamindServant::TMegamindServant;

    ERequestPhase GetRequestPhase() const override;
    void OnCreate() override;
    void OnContextsLoaded() override;
    void OnError(const TString& error, const TString& errorCode) override {
        StateMachine_.Process(NMegamindEvents::TError(errorCode, error));
        TMegamindServant::OnError(error, errorCode);
    }
    void OnInternalError(const TString& error, const TString& errorCode) override {
        StateMachine_.Process(NMegamindEvents::TError(errorCode, error));
        TMegamindServant::OnInternalError(error, errorCode);
    }
    bool ProcessNextInput() override;

    struct TReceivedAsrResponse {
        TReceivedAsrResponse(AsrEngineResponseProtobuf::TASRResponse&& resp)
            : Received(TInstant::Now())
            , AsrResponse(std::move(resp))
        {
        }
        const TInstant Received;
        const AsrEngineResponseProtobuf::TASRResponse AsrResponse;
    };
    void ProcessAsrResponse(const TReceivedAsrResponse&, bool postponed=false);
    void InitAndSendSubrequestForSpareAsrResult(const TSubrequest&);
    TString GetAddDataResponseWords(const AsrEngineResponseProtobuf::TAddDataResponse&);
    void ProcessYabioResponse(const NYabio::TResponse&);
    void SendSubrequestForAsrResponse(TSubrequestPtr req, const AsrEngineResponseProtobuf::TAddDataResponse&);
    void OnHasFinalResult(TSubrequest&) override;
    void OnCancelRequest() override;
    void SendLegacySessionLogPartial(const TString& status);

private:
    bool HaveApply{false}; // should wait for apply response
    bool VoiceInput_{false};
    bool UseAsrPartials_{false};
    bool HasEou_{false};
    bool HasFirstAsrPartial_{false};
    bool HasFirstNotEmptyAsrPartial_{false};
    std::deque<TReceivedAsrResponse> AsrResponses_;

    TMaybe<NAlice::TBiometryScoring> BiometryScoring;
    TMaybe<NAlice::TBiometryClassification> BiometryClassification;
    TMaybe<int> AsrPartialNumber = Nothing();
    TMaybe<double> MaxBiometryScore;

    using TSpeakerPtr = void*;
    typedef std::tuple<TString, TString, TSpeakerPtr> TSubrequestKey;  // cache_key & text(word via ' ') & pers_id of speaker
    typedef std::map<TSubrequestKey, TIntrusivePtr<TSubrequest> > TSubrequests;
    TSubrequests Subrequests_;
    TMegamindRunServantStateMachine StateMachine_;
};

class TMegamindApplyServant : public TMegamindServant {
public:
    TMegamindApplyServant(const NAliceCuttlefishConfig::TConfig& config, NAppHost::TServiceContextPtr serviceCtx, TLogContext logContext)
        : TMegamindServant(config, serviceCtx, logContext, "megamind_apply")
        , StateMachine_(*this, Metrics)
    {}

    static TMegamindServantPtr Create(const NAliceCuttlefishConfig::TConfig& cfg, NAppHost::TServiceContextPtr serviceCtx, TLogContext logContext) {
        TMegamindServantPtr ptr{new TMegamindApplyServant(cfg, std::move(serviceCtx), std::move(logContext))};
        ptr->OnCreate();
        return ptr;
    }

private:
    using TMegamindServant::TMegamindServant;

    ERequestPhase GetRequestPhase() const override;
    void OnCreate() override;
    void OnContextsLoaded() override;
    void OnError(const TString& error, const TString& errorCode) override {
        StateMachine_.Process(NMegamindEvents::TError(errorCode, error));
        TMegamindServant::OnError(error, errorCode);
    }
    void OnInternalError(const TString& error, const TString& errorCode) override {
        StateMachine_.Process(NMegamindEvents::TError(errorCode, error));
        TMegamindServant::OnInternalError(error, errorCode);
    }
    bool ProcessNextInput() override;
    void OnHasFinalResult(TSubrequest&) override;

private:
    TMaybe<NAlice::TSpeechKitRequestProto> ApplyRequest;
    TMegamindApplyServantStateMachine StateMachine_;
};

}  // namespace NAlice::NCuttlefish::NAppHostServices
