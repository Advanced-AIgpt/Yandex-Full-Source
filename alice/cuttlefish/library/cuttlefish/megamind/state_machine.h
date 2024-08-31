#pragma once

#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/evproc/machines.h>
#include <alice/cuttlefish/library/protos/events.pb.h>

#include "events.h"
#include "states.h"
#include "subrequest.h"


namespace NAlice::NCuttlefish::NAppHostServices {

class TMegamindServant;

inline TDuration DurationFromMs(ui64 begin, ui64 end) {
    if (begin >= end || !end || !begin) {
        return {};
    }

    return TDuration::MilliSeconds(end - begin);
}

inline ui64 Lag(ui64 begin, ui64 end) {
    if (begin >= end || !end || !begin) {
        return {};
    }

    return end - begin;
}

inline ui64 Lag(ui64 begin) {
    const ui64 end = TInstant::Now().MilliSeconds();
    if (begin >= end || !end || !begin) {
        return {};
    }

    return end - begin;
}

struct TMegamindRunContext {
    bool HasMmRequest{false};
    bool VoiceInput{false};
    bool HasEou{false};
    NAliceProtocol::TRequestDebugInfo::TMegamindRun Timings;
};

struct TMegamindApplyContext {
    NAliceProtocol::TRequestDebugInfo::TMegamindApply Timings;
};

class TMegamindRunServantStateMachine;

using TMegamindRunServantStateMachineImpl = NSM::TStateMachine<
    TMegamindRunServantStateMachine,
    TMegamindRunContext,
    TTypeList<
        NSM::TInitState,
        NMegamindStates::TWaitingForMmRequestState,
        NMegamindStates::TWaitingForContextsState,
        NMegamindStates::TWaitingForEouState,                      // wait asr eou
        NMegamindStates::TWaitingForFinalSubrequestResponseState,  // wait response for final request to vins/mm
        NMegamindStates::TProcessFinalSubrequestResponseState,
        NMegamindStates::TFinalState
    >,
    TTypeList<
        NSM::TStartEv,
        NMegamindEvents::TReceiveMmRequest,
        NMegamindEvents::TContextsLoaded,
        NMegamindEvents::TFirstPartialAsrResult,
        NMegamindEvents::TNotEmptyPartialAsrResult,
        NMegamindEvents::TSendSubrequest,
        NMegamindEvents::TReceiveEouAsrResult,
        NMegamindEvents::TUseEouAsrResult,
        NMegamindEvents::TUsefulMegamindResponse,
        NMegamindEvents::TSuccess,
        NMegamindEvents::TCancelRequest,
        NMegamindEvents::TError
    >
>;

class TMegamindRunServantStateMachine : public TMegamindRunServantStateMachineImpl {
public:
    TMegamindRunServantStateMachine(TMegamindServant& megamindServant, TSourceMetrics& metrics)
        : MegamindServant_(megamindServant)
        , Metrics_(metrics)
    {}
    using TMegamindRunServantStateMachineImpl::OnEvent;
    using TMegamindRunServantStateMachineImpl::OnTransition;
    using TMegamindRunServantStateMachineImpl::SetState;

    template<class T>
    uint64_t Process(const T& t) {
        with_lock(Mutex_) {
            return ProcessEvent(Ctx_, t);
        }
    }

private:
    NAliceProtocol::TRequestDebugInfo::TMegamindRun& Timings() {
        return Ctx_.Timings;
    }
    ui64 Lag() {
        return NAlice::NCuttlefish::NAppHostServices::Lag(Timings().GetStart(), TInstant::Now().MilliSeconds());
    }
    void SetFinalState() {
        SetState<NMegamindStates::TFinalState>(Ctx_);
        OnPostFinal();
    }
    void OnPostFinal();

    TMegamindServant& MegamindServant_;
    TMutex Mutex_;
    TSourceMetrics& Metrics_;
    TMegamindRunContext Ctx_;

    typedef TMegamindRunContext TNoUse;  // short alias for useless input parameter

public:
    void UpdateLastPartialAsrResultTiming() {
        with_lock(Mutex_) {
            Timings().SetLastPartialAsrResult(NAlice::NCuttlefish::NAppHostServices::Lag(Timings().GetStart(), TInstant::Now().MilliSeconds()));
        }
    }
    void SetUsefulPartialTrashedByMM() {
        with_lock(Mutex_) {
            Timings().SetUsefulPartialTrashedByMM(true);
        }
        Metrics_.PushRate(1, "vins_partial_trash_vins");  // use same name as in python_uniproxy
    }
    void SetUsefulPartialNotFound() {
        Metrics_.PushRate(1, "vins_partial_bad");  // use same name as in python_uniproxy
    }
    void SetUsefulPartialOk() {
        Metrics_.PushRate(1, "vins_partial_good");  // use same name as in python_uniproxy
    }
    void SetUsefulPartialWaitOk() {
        Metrics_.PushRate(1, "vins_partial_good_wait");  // use same name as in python_uniproxy
    }
    void PushMaxBiometryScore(double maxScore) {
        Metrics_.PushHist((1-maxScore)*10e6, "max_biometry_score_uncertainty");
    }

    void OnEvent(TNoUse&, const NSM::TInitState&, const NSM::TStartEv&) {
        Timings().SetStart(TInstant::Now().MilliSeconds());
        SetState<NMegamindStates::TWaitingForMmRequestState>(Ctx_);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForMmRequestState&, const NMegamindEvents::TReceiveMmRequest&) {
        Ctx_.HasMmRequest = true;
        SetState<NMegamindStates::TWaitingForContextsState>(Ctx_);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForMmRequestState&, const NMegamindEvents::TContextsLoaded& event) {
        OnEvent(event);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForContextsState&, const NMegamindEvents::TContextsLoaded& event) {
        OnEvent(event);
    }
    void OnEvent(const NMegamindEvents::TContextsLoaded& contextLoaded) {
        Timings().SetContextsLoaded(Lag());
        Metrics_.PushHist(TDuration::MilliSeconds(Timings().GetContextsLoaded()), "wait_contexts");
        Timings().SetRequestName(contextLoaded.RequestName);
        if (contextLoaded.RequestName == "vins.voiceinput") {
            Ctx_.VoiceInput = true;
            if (!Ctx_.HasEou) {
                SetState<NMegamindStates::TWaitingForEouState>(Ctx_);
                return;
            }
        }
        SetState<NMegamindStates::TWaitingForFinalSubrequestResponseState>(Ctx_);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForMmRequestState&, const NMegamindEvents::TFirstPartialAsrResult& event) {
        OnEvent(event);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForContextsState&, const NMegamindEvents::TFirstPartialAsrResult& event) {
        OnEvent(event);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForEouState&, const NMegamindEvents::TFirstPartialAsrResult& event) {
        OnEvent(event);
    }
    void OnEvent(const NMegamindEvents::TFirstPartialAsrResult&) {
        if (!Timings().HasFirstPartialAsrResult()) {
            Timings().SetFirstPartialAsrResult(Lag());
        }
        Metrics_.PushHist(TDuration::MilliSeconds(Timings().GetFirstPartialAsrResult()), "first_partial", "", "asr");
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForMmRequestState&, const NMegamindEvents::TNotEmptyPartialAsrResult& event) {
        OnEvent(event);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForContextsState&, const NMegamindEvents::TNotEmptyPartialAsrResult& event) {
        OnEvent(event);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForEouState&, const NMegamindEvents::TNotEmptyPartialAsrResult& event) {
        OnEvent(event);
    }
    void OnEvent(const NMegamindEvents::TNotEmptyPartialAsrResult&) {
        if (!Timings().HasFirstNotEmptyPartialAsrResult()) {
            Timings().SetFirstNotEmptyPartialAsrResult(Lag());
        }
        Metrics_.PushHist(TDuration::MilliSeconds(Timings().GetFirstNotEmptyPartialAsrResult()), "first_not_empty_partial", "", "asr");
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForMmRequestState&, const NMegamindEvents::TSendSubrequest& event) {
        OnEvent(event);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForContextsState&, const NMegamindEvents::TSendSubrequest& event) {
        OnEvent(event);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForEouState&, const NMegamindEvents::TSendSubrequest& event) {
        OnEvent(event);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForFinalSubrequestResponseState&, const NMegamindEvents::TSendSubrequest& event) {
        OnEvent(event);
    }
    void OnEvent(const NMegamindEvents::TSendSubrequest&) {
        if (!Timings().HasFirstSubrequest()) {
            Timings().SetFirstSubrequest(Lag());
        }
        if (!Timings().HasSubrequestsCount()) {
            Timings().SetSubrequestsCount(1);
        } else {
            Timings().SetSubrequestsCount(Timings().GetSubrequestsCount() + 1);
        }
        if (State_ != NMegamindStates::TWaitingForEouState::Id && State_ != NMegamindStates::TWaitingForFinalSubrequestResponseState::Id) {
            SetState<NMegamindStates::TWaitingForEouState>(Ctx_);
        }
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForMmRequestState&, const NMegamindEvents::TReceiveEouAsrResult& asrResult) {
        OnEvent(asrResult);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForContextsState&, const NMegamindEvents::TReceiveEouAsrResult& asrResult) {
        OnEvent(asrResult);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForEouState&, const NMegamindEvents::TReceiveEouAsrResult& asrResult) {
        OnEvent(asrResult);
    }
    void OnEvent(const NMegamindEvents::TReceiveEouAsrResult& asrResult) {
        Ctx_.HasEou = true;
        Timings().SetEouReceived(Lag());
        Timings().SetEmptyEouResult(asrResult.Empty);
        if (asrResult.Empty) {
            Metrics_.PushRate(1, "vins_partial_trash_asr");  // use same name as in python_uniproxy (and42@: imho bad name, but for compatibility...)
            Metrics_.PushRate(1, "eou", "empty", "asr");
            SetFinalState();
            return;
        }
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForEouState&, const NMegamindEvents::TUseEouAsrResult& asrResult) {
        OnEvent(Ctx_, asrResult);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForFinalSubrequestResponseState&, const NMegamindEvents::TUseEouAsrResult& asrResult) {
        OnEvent(Ctx_, asrResult);
    }
    void OnEvent(TNoUse&, const NMegamindEvents::TUseEouAsrResult& asrResult) {
        if (asrResult.Postponed) {
            Metrics_.PushHist(DurationFromMs(Timings().GetEouReceived(), Timings().GetUseEouResult()), "delay_usage_eou");
        }
        Metrics_.PushRate(1, "eou", "useful", "asr");
        if (asrResult.Subrequest) {
            Timings().SetUsefulPartialAsrResult(NAlice::NCuttlefish::NAppHostServices::Lag(Timings().GetStart(), asrResult.Subrequest->ReceiveAsrResult.MilliSeconds()));
            Timings().SetUsefulSubrequestStart(NAlice::NCuttlefish::NAppHostServices::Lag(Timings().GetStart(), asrResult.Subrequest->Start.MilliSeconds()));
            Metrics_.PushHist(DurationFromMs(Timings().GetUsefulSubrequestStart(), Timings().GetEouReceived()), "useful_partial_to_eou", "", "asr");
            if (asrResult.Subrequest->Postponed) {
                Metrics_.PushRate(1, "postponed_useful_request");
            }
            if (asrResult.SubrequestFinished) {
                Metrics_.PushRate(1, "useful_partial", "quick", "asr");
                Timings().SetUsefulSubrequestFinish(NAlice::NCuttlefish::NAppHostServices::Lag(Timings().GetStart(), asrResult.Subrequest->Finish.MilliSeconds()));
                OnUsefulResponse();
            } else {
                Metrics_.PushRate(1, "useful_partial", "normal", "asr");
                SetState<NMegamindStates::TWaitingForFinalSubrequestResponseState>(Ctx_);
            }
        } else {
            Metrics_.PushRate(1, "useful_partial", "false", "asr");
            Timings().SetUsefulSubrequestStart(Lag());
            SetState<NMegamindStates::TWaitingForFinalSubrequestResponseState>(Ctx_);
        }
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForFinalSubrequestResponseState&, const NMegamindEvents::TUsefulMegamindResponse& response) {
        Timings().SetUsefulSubrequestStart(NAlice::NCuttlefish::NAppHostServices::Lag(Timings().GetStart(), response.Start.MilliSeconds()));
        Timings().SetUsefulSubrequestFinish(NAlice::NCuttlefish::NAppHostServices::Lag(Timings().GetStart(), response.Finish.MilliSeconds()));
        if (response.IntentName) {
            Timings().SetResponseIntentName(response.IntentName);
        }
        OnUsefulResponse();
    }
    void OnUsefulResponse() {
        if (Timings().HasUsefulPartialAsrResult() && !Timings().GetUsefulPartialTrashedByMM()
            && Timings().GetEouReceived() < Timings().GetUsefulSubrequestFinish())
        {
            SetUsefulPartialWaitOk();
        }
        Metrics_.PushHist(DurationFromMs(Timings().GetUsefulSubrequestStart(), Timings().GetUsefulSubrequestFinish()), "request_duration", "", "megamind");
        if (Timings().GetEouReceived()) {
            if (Timings().GetUsefulSubrequestFinish() > Timings().GetEouReceived()) {
                Metrics_.PushHist(DurationFromMs(Timings().GetEouReceived(), Timings().GetUsefulSubrequestFinish()), "wait", Timings().GetRequestName(), "megamind");
            } // else got fast mm response for partial result
        } else {
            Metrics_.PushHist(DurationFromMs(Timings().GetUsefulSubrequestStart(), Timings().GetUsefulSubrequestFinish()), "wait", Timings().GetRequestName(), "megamind");
        }
        SetState<NMegamindStates::TProcessFinalSubrequestResponseState>(Ctx_);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TProcessFinalSubrequestResponseState&, const NMegamindEvents::TSuccess& success) {
        if (success.NeedApply) {
            Timings().SetNeedApply(true);
        }
        SetFinalState();
    }
    template <class T>
    void OnEvent(TNoUse&, const T&, const NMegamindEvents::TError& event) {
        static const TString unknown{"unknown"};
        Timings().SetErrorCode(event.ErrorCode ? event.ErrorCode : unknown);
        Metrics_.SetError(Timings().GetErrorCode());
        if (event.ErrorText) {
            Timings().SetErrorText(event.ErrorText);
        }
        SetFinalState();
    }
    template <class T>
    void OnEvent(TNoUse&, const T&, const NMegamindEvents::TCancelRequest&) {
        Metrics_.SetError("canceled");
        Timings().SetRequestCanceled(true);
        SetFinalState();
    }
};

class TMegamindApplyServantStateMachine;

using TMegamindApplyServantStateMachineImpl = NSM::TStateMachine<
    TMegamindApplyServantStateMachine,
    TMegamindApplyContext,
    TTypeList<
        NSM::TInitState,
        NMegamindStates::TWaitingForContextsState,
        NMegamindStates::TWaitingForApplyRequestState,  // if got asr partial result, count as request only not empty partial
        NMegamindStates::TWaitingForFinalSubrequestResponseState,
        NMegamindStates::TProcessFinalSubrequestResponseState,
        NMegamindStates::TFinalState
    >,
    TTypeList<
        NSM::TStartEv,
        NMegamindEvents::TError,
        NMegamindEvents::TContextsLoaded,
        NMegamindEvents::TReceiveApplyRequest,
        NMegamindEvents::TUsefulMegamindResponse,
        NMegamindEvents::TSuccess,
        NMegamindEvents::TCancelRequest
    >
>;

class TMegamindApplyServantStateMachine : public TMegamindApplyServantStateMachineImpl {
public:
    TMegamindApplyServantStateMachine(TMegamindServant& megamindServant, TSourceMetrics& metrics)
        : MegamindServant_(megamindServant)
        , Metrics_(metrics)
    {}
    using TMegamindApplyServantStateMachineImpl::OnEvent;
    using TMegamindApplyServantStateMachineImpl::OnTransition;
    using TMegamindApplyServantStateMachineImpl::SetState;

    template<class T>
    uint64_t Process(const T& t) {
        with_lock(Mutex_) {
            return ProcessEvent(Ctx_, t);
        }
    }

private:
    NAliceProtocol::TRequestDebugInfo::TMegamindApply& Timings() {
        return Ctx_.Timings;
    }
    ui64 Lag() {
        return NAlice::NCuttlefish::NAppHostServices::Lag(Timings().GetStart(), TInstant::Now().MilliSeconds());
    }
    void SetFinalState() {
        SetState<NMegamindStates::TFinalState>(Ctx_);
        OnPostFinal();
    }
    void OnPostFinal();

    TMegamindServant& MegamindServant_;
    TMutex Mutex_;
    TSourceMetrics& Metrics_;
    TMegamindApplyContext Ctx_;

    typedef TMegamindApplyContext TNoUse;  // short alias for useless input parameter

public:
    void OnEvent(TNoUse&, const NSM::TInitState&, const NSM::TStartEv&) {
        Timings().SetStart(TInstant::Now().MilliSeconds());
        SetState<NMegamindStates::TWaitingForContextsState>(Ctx_);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForContextsState&, const NMegamindEvents::TContextsLoaded&) {
        Timings().SetContextsLoaded(Lag());
        Metrics_.PushHist(TDuration::MilliSeconds(Timings().GetContextsLoaded()), "wait_contexts");
        SetState<NMegamindStates::TWaitingForApplyRequestState>(Ctx_);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForApplyRequestState&, const NMegamindEvents::TReceiveApplyRequest&) {
        Timings().SetSubrequestStart(Lag());
        Metrics_.PushHist(TDuration::MilliSeconds(Timings().GetSubrequestStart()), "wait_apply_request", "", "");
        SetState<NMegamindStates::TWaitingForFinalSubrequestResponseState>(Ctx_);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TWaitingForFinalSubrequestResponseState&, const NMegamindEvents::TUsefulMegamindResponse&) {
        Timings().SetSubrequestFinish(Lag());
        Metrics_.PushHist(TDuration::MilliSeconds(Timings().GetSubrequestFinish()), "request_duration", "", "megamind");
        SetState<NMegamindStates::TProcessFinalSubrequestResponseState>(Ctx_);
    }
    void OnEvent(TNoUse&, const NMegamindStates::TProcessFinalSubrequestResponseState&, const NMegamindEvents::TSuccess&) {
        SetFinalState();
    }
    template <class T>
    void OnEvent(TNoUse&, const T&, const NMegamindEvents::TError& event) {
        static const TString unknown{"unknown"};
        Timings().SetErrorCode(event.ErrorCode ? event.ErrorCode : unknown);
        Metrics_.SetError(Timings().GetErrorCode());
        if (event.ErrorText) {
            Timings().SetErrorText(event.ErrorText);
        }
        SetFinalState();
    }
    template <class T>
    void OnEvent(TNoUse&, const T&, const NMegamindEvents::TCancelRequest&) {
        Metrics_.SetError("canceled");
        Timings().SetRequestCanceled(true);
        SetFinalState();
    }
};

}  // namespace NAlice::NCuttlefish::NAppHostServices
