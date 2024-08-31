#include "vins_timings.h"

namespace {
    class TEventAgeCalculator {
    public:
        TEventAgeCalculator(ui64 timestampRequest, ui64 timestampBackend)
            : TimeShift_(timestampBackend > timestampRequest ? timestampBackend - timestampRequest : 0)
        {
        }

        double Seconds(ui64 lag) {
            return (TimeShift_ + lag) / 1000.;
        }

    private:
        ui64 TimeShift_;  // backend timeshift to request_start (milliseconds)
    };

    double SafeLagSeconds(ui64 fromMs, ui64 toMs) {
        if (toMs < fromMs) {
            return 0.;
        }

        return (toMs - fromMs) / 1000.;
    }
}

void NAlice::NCuttlefish::NAppHostServices::BuildLegacyVinsTimings(
    NJson::TJsonValue& payload,
    const NAliceProtocol::TRequestDebugInfo& debugInfo,
    TLogContext* logContext
) {
    (void)logContext;
    if (!debugInfo.HasUniproxy2() || !debugInfo.GetUniproxy2().GetStart()) {
        return;  // can't continue without main/base timestamp
    }

    auto& uniproxy = debugInfo.GetUniproxy2();
    ui64 startRequest = uniproxy.GetStart();
    auto& mmRun = debugInfo.GetMegamindRun();
    auto& mmApply = debugInfo.GetMegamindApply();
    auto& wsOut = debugInfo.GetWsAdapterOut();
    TEventAgeCalculator mmEvage{startRequest, mmRun.GetStart()};
    TEventAgeCalculator mmApplyEvage{startRequest, mmApply.GetStart()};
    TEventAgeCalculator wsOutEvage{startRequest, wsOut.GetStart()};

    if (mmRun.HasEouReceived()) {
        // Event when EOU signal from ASR is received.
        payload["end_of_utterance_sec"] = mmEvage.Seconds(mmRun.GetEouReceived());
    }
    // Server-time epoch when the request processing was started
    payload["epoch"] = startRequest / 1000;  // store as seconds
    if (!mmRun.HasUsefulPartialAsrResult() && mmRun.HasUsefulSubrequestFinish()) {
        // Duration of the finish request to vins with eou=true from VINS.
        // and42@: for me this metric seems useless (calc it only for more legacy compatibility)
        payload["finish_vins_request_eou"] = mmEvage.Seconds(mmRun.GetUsefulSubrequestFinish());
    }
    if (mmRun.HasFirstPartialAsrResult()) {
        // Event for first asr result (include empty partials)
        payload["first_asr_result_sec"] = mmEvage.Seconds(mmRun.GetFirstPartialAsrResult());
    } else if (mmRun.HasEouReceived()) {
        // Event for first asr result (include empty partials)
        payload["first_asr_result_sec"] = mmEvage.Seconds(mmRun.GetEouReceived());
    }
    //TODO: ?! fix collecting metrics for bad spotter
    // "get_spotter_validation_result_end_evage":1.02793657197617,
    // "get_spotter_validation_result_start_evage":0.9233371450100094,
    if (debugInfo.HasMegamindApply()) {
        payload["has_apply_vins_request"] = true;
    }
    if (mmRun.HasUsefulSubrequestFinish() && mmRun.HasEouReceived()) {
        if (mmRun.HasUsefulSubrequestFinish() < mmRun.HasEouReceived()) {
            // Was VINS response ready before EOU has been received.
            payload["has_vins_full_result_on_eou"] = true;
        }
    }
    // Event of the classification result.
    // "last_classify_partial_sec":0.9194171230774373,
    if (mmRun.HasLastPartialAsrResult()) {
        // Event of the last non-EOU partial.
        payload["last_partial_sec"] = mmEvage.Seconds(mmRun.GetLastPartialAsrResult());
    }
    // Event of the scoring result.
    // "last_score_partial_sec":0.9585859630024061,
    if (mmApply.HasSubrequestStart() && mmApply.HasSubrequestFinish()) {
        // Duration of the apply request from VINS.
        payload["last_vins_apply_request_duration_sec"] = SafeLagSeconds(mmApply.GetSubrequestStart(), mmApply.GetSubrequestFinish());
    }
    // Duration of the last run request with side-effects request from VINS.
    // NOTE: really use duration not for last request, but useful
    if (mmRun.HasUsefulSubrequestStart() && mmRun.HasUsefulSubrequestFinish()) {
        double sumRequestsDuration = SafeLagSeconds(mmRun.GetUsefulSubrequestStart(), mmRun.GetUsefulSubrequestFinish());
        if (mmApply.HasSubrequestStart() && mmApply.HasSubrequestFinish()) {
            sumRequestsDuration += SafeLagSeconds(mmApply.GetSubrequestStart(), mmApply.GetSubrequestFinish());
        }
        payload["last_vins_full_request_duration_sec"] = sumRequestsDuration;
    }

    // Duration of waiting responses from music&yabio after receiving result from asr
    // "last_vins_preparing_request_duration_sec":0.04318607202731073,
    if (mmRun.HasUsefulSubrequestStart() && mmRun.HasUsefulSubrequestFinish()) {
        // Duration of the last run request (no side-effects) from VINS.
        // WARNING: fake value (use useful request duration instead)
        payload["last_vins_run_request_duration_sec"] = SafeLagSeconds(mmRun.GetUsefulSubrequestStart(), mmRun.GetUsefulSubrequestFinish());
    }
    if (mmRun.HasResponseIntentName()) {
        // Intent name of the last run request from VINS.
        // NOTE: placed here intent from useful response (but not last).
        payload["last_vins_run_request_intent_name"] = mmRun.GetResponseIntentName();
    }
    // Average duration of waiting responses from music&yabio after receiving result from asr.
    // "mean_vins_preparing_request_duration_sec":0.14738612750079483,
    if (mmRun.HasUsefulSubrequestStart() && mmRun.HasUsefulSubrequestFinish()) {
        // Average duration of all run (no side-effects) requests from VINS.
        // WARNING: fake value (use useful request duration instead)
        payload["mean_vins_request_duration_sec"] = SafeLagSeconds(mmRun.GetUsefulSubrequestStart(), mmRun.GetUsefulSubrequestFinish());
    }
    if (mmApply.HasSubrequestFinish()) {
        // Event when last Run (no side-effects) response from VINS was received.
        // WARNING: a little bit not correct metric (rarely we can has useless request to vins between useful partial & eou)
        payload["result_vins_run_response_is_ready_sec"] = mmEvage.Seconds(mmApply.GetSubrequestFinish());
    }
    if (mmApply.HasSubrequestStart()) {
        // Lag from begin processing VoiceInput to sending apply request to VINS.
        payload["start_vins_apply_request_sec"] = mmApplyEvage.Seconds(mmApply.GetSubrequestStart());
    }
    if (!mmRun.HasUsefulPartialAsrResult() && mmRun.HasUsefulSubrequestStart()) {
        // Duration of the starting request to vins with eou=true from VINS.
        payload["start_vins_request_eou"] = mmEvage.Seconds(mmRun.GetUsefulSubrequestStart());
    }
    if (mmRun.HasUsefulPartialAsrResult()) {
        // Event for asr result with text same as asr_end result(eou).
        payload["useful_partial_sec"] = mmEvage.Seconds(mmRun.GetUsefulPartialAsrResult());
    }
    /*
    // we can calculate prepare lags using only MM debug info
    if (mmRun.HasUsefulSubrequestStart()) {
        ui64 usefulAsrResult = 0;
        if (mmRun.HasUsefulPartialAsrResult()) {
            usefulAsrResult = mmRun.GetUsefulPartialAsrResult();
        } else if (mmRun.HasEouReceived()) {
            usefulAsrResult = mmRun.GetEouReceived();
        }
        if (usefulAsrResult) {
            double lag = SafeLagSeconds(usefulAsrResult, mmRun.GetUsefulSubrequestStart());
            // WARNING: fake values
            payload["useful_vins_prepare_request_asr"] = lag;
            payload["useful_vins_prepare_request_classify"] = lag;
            payload["useful_vins_prepare_request_contacts"] = lag;
            payload["useful_vins_prepare_request_memento"] = lag;
            payload["useful_vins_prepare_request_notification_state"] = lag;
            payload["useful_vins_prepare_request_personal_data"] = lag;
            payload["useful_vins_prepare_request_session"] = lag;
            payload["useful_vins_prepare_request_yabio"] = lag;
        }
    }
    */
    if (mmRun.HasUsefulSubrequestStart() && mmRun.HasUsefulSubrequestFinish()) {
        payload["useful_vins_request_duration_sec"] = SafeLagSeconds(mmRun.GetUsefulSubrequestStart(), mmRun.GetUsefulSubrequestFinish());
    }
    if (mmRun.HasUsefulSubrequestStart()) {
        payload["useful_vins_request_evage"] = mmEvage.Seconds(mmRun.GetUsefulSubrequestStart());
    }
    // "vins_personal_data_end_evage":0.03227115503977984,
    // "vins_personal_data_start_evage":0.002368047018535435,
    payload["vins_request_count"] = mmRun.GetSubrequestsCount() + (debugInfo.HasMegamindApply() ? 1 : 0);
    if (wsOut.HasMegamindResponse()) {
        // Event when VINS response is being sent to the client
        payload["vins_response_sec"] = wsOutEvage.Seconds(wsOut.GetMegamindResponse());
    }
    if (mmRun.HasUsefulSubrequestFinish() && mmRun.HasEouReceived()) {
        // Duration between EOU and VINS run response. (calс lag as 0 if not wait vins)
        payload["vins_run_delay_after_eou_duration_sec"] = SafeLagSeconds(mmRun.GetEouReceived(), mmRun.GetUsefulSubrequestFinish());
    }
    if (mmRun.HasEouReceived() && mmRun.HasUsefulSubrequestFinish()) {
        if (mmRun.HasUsefulSubrequestFinish() > mmRun.HasEouReceived()) {
            // Duration between EOU and VINS run response. (collect here only really waited vins requests)
            payload["vins_run_wait_after_eou_duration_sec"] = SafeLagSeconds(mmRun.GetEouReceived(), mmRun.GetUsefulSubrequestFinish());
        }
    }
    if (mmRun.HasContextsLoaded()) {
        // WARNING: fake falue
        // TODO: ? use here exactly session_load (instead all contexts)
        payload["vins_session_load_end_evage"] = mmEvage.Seconds(mmRun.GetContextsLoaded());
    }
    if (mmRun.HasEouReceived() && mmRun.HasUsefulSubrequestFinish()) {
        double lag = SafeLagSeconds(mmRun.GetEouReceived(), mmRun.GetUsefulSubrequestFinish());
        if (mmApply.HasSubrequestFinish()) {
            lag = SafeLagSeconds(mmRun.GetEouReceived(), mmApply.GetSubrequestFinish());
        }
        // Duration between EOU and VINS run or apply response.
        payload["vins_wait_after_eou_duration_sec"] = lag;
    }
}

void NAlice::NCuttlefish::NAppHostServices::BuildLegacyTtsTimings(
    NJson::TJsonValue& payload,
    const NAliceProtocol::TRequestDebugInfo& debugInfo,
    TLogContext* logContext
) {
    (void)logContext;
    if (!debugInfo.HasUniproxy2() || !debugInfo.GetUniproxy2().GetStart()) {
        return;  // can't continue without main/base timestamp
    }

    auto& uniproxy = debugInfo.GetUniproxy2();
    ui64 startRequest = uniproxy.GetStart();
    auto& wsOut = debugInfo.GetWsAdapterOut();
    TEventAgeCalculator wsOutEvage{startRequest, wsOut.GetStart()};
    if (wsOut.HasFirstTtsChunk()) {
        //Duration between event processing start and first TTS chunk.
        payload["first_tts_chunk_sec"] = wsOutEvage.Seconds(wsOut.GetFirstTtsChunk());
    }
    //TODO: payload["tts_cache_response_evage"] = ?
    //TODO: payload["tts_cache_success"] = true | false;
    //TODO: payload["tts_start_evage"] = ?
    if (wsOut.HasFirstTtsChunk()) {
        //Duration between first tts chunk (if has tts) or vins response and event processing start.
        payload["useful_response_for_user_evage"] = wsOutEvage.Seconds(wsOut.GetFirstTtsChunk());
    } else if (wsOut.HasMegamindResponse()) {
        //Duration between first tts chunk (if has tts) or vins response and event processing start.
        payload["useful_response_for_user_evage"] = wsOutEvage.Seconds(wsOut.GetMegamindResponse());
    }
}
