#include "unistat.h"

using namespace NAlice::NAsrAdapter;

namespace {
/*
    const NUnistat::TIntervals HIGH_RESOLUTION_COUNTER_VALUES({
        0, 0.005, 0.010, 0.015, 0.020, 0.025, 0.030, 0.035, 0.040, 0.045, 0.050, 0.06, 0.07, 0.08, 0.09,
        0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 2.0, 3.0, 5.0, 7.5, 10.0
    });

    const NUnistat::TIntervals LOW_RESOLUTION_COUNTER_VALUES({
        0.0, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0, 5.0, 7.5, 10.0
    });
*/
    ::NUnistat::IHolePtr CreateCounter(const TString& name, const TString& suffix = "summ") {
        return TUnistat::Instance().DrillFloatHole(
            name,
            suffix,
            ::NUnistat::TPriority(0),
            ::NUnistat::TStartValue(0));
    }
/*
    ::NUnistat::IHolePtr CreateHgramCounter(const TString& name, const NUnistat::TIntervals& intervals) {
        return TUnistat::Instance().DrillHistogramHole(
            name,
            "hgram",
            ::NUnistat::TPriority(0),
            intervals
        );
    }
*/
}  // anonymous namespace


TUnistatN::TUnistatN() {
    Asr2ViaAsr1ClientCounter_ = CreateCounter("asr2_via_asr1_client", "ammx");
    CreateAsr1Client_ = CreateCounter("asr1_client_create");
    Asr1ClientCounter_ = CreateCounter("asr1_client_counter", "ammx");

    Asr1ErrorAll_ = CreateCounter("asr1_error_all");
    Asr1ErrorBadMessage_ = CreateCounter("asr1_error_bad_message");
    Asr1ErrorConnect_ = CreateCounter("asr1_error_connect");
    Asr1ErrorParseHttpResponse_ = CreateCounter("asr1_error_parse_http_resp");
    Asr1ErrorInvalidParams_ = CreateCounter("asr1_error_invalid_params");
    Asr1ErrorTimeout_ = CreateCounter("asr1_error_timeout");
    Asr1ErrorInternal_ = CreateCounter("asr1_error_internal");
    Asr1ErrorUnspecific_ = CreateCounter("asr1_error_unspecific");

    Asr1SpotterErrorAll_ = CreateCounter("asr1_spotter_error_all");
    Asr1SpotterErrorBadMessage_ = CreateCounter("asr1_spotter_error_bad_message");
    Asr1SpotterErrorInvalidParams_ = CreateCounter("asr1_spotter_error_invalid_params");
    Asr1SpotterErrorTimeout_ = CreateCounter("asr1_spotter_error_timeout");
    Asr1SpotterErrorInternal_ = CreateCounter("asr1_spotter_error_internal");
    Asr1SpotterErrorDeadline_ = CreateCounter("asr1_spotter_error_deadline");
    Asr1SpotterErrorRequest_ = CreateCounter("asr1_spotter_error_request");
    Asr1SpotterErrorUnspecific_ = CreateCounter("asr1_spotter_error_unspecific");

    Asr1SpotterCancel_ = CreateCounter("asr1_spotter_cancel");

    RecvFromAppHostReqRawBytes_ = CreateCounter("recv_ah_req_raw_bytes");
    RecvFromAppHostRawBytes_ = CreateCounter("recv_ah_raw_bytes");
    RecvFromAppHostItems_ = CreateCounter("recv_ah_items");
    RecvFromAsrRawBytes_ = CreateCounter("recv_asr_raw_bytes");
    SendToAppHostItems_ = CreateCounter("send_ah_items");
    SendToAppHostRawBytes_ = CreateCounter("send_ah_raw_bytes");
    SendToAsrRawBytes_ = CreateCounter("send_asr_raw_bytes");
/*
    AppHostRequestTimings_ = CreateHgramCounter("ah_request_timings", HIGH_RESOLUTION_COUNTER_VALUES);
    ClientLastSynthTimings_ = CreateHgramCounter("client_last_synth", LOW_RESOLUTION_COUNTER_VALUES);
*/
}

void TUnistatN::OnCreateAsr1Client() {
    CreateAsr1Client_->PushSignal(1.);
}

void TUnistatN::OnAsr1Error(int code) {
    Asr1ErrorAll_->PushSignal(1.);
    switch (code) {
    case 400:
        Asr1ErrorBadMessage_->PushSignal(1.);
        break;
    case 404:
        Asr1ErrorInvalidParams_->PushSignal(1.);
        break;
    case 408:
        Asr1ErrorTimeout_->PushSignal(1.);
        break;
    case 500:
        Asr1ErrorInternal_->PushSignal(1.);
        break;
    case ConnectFailed:
        Asr1ErrorConnect_->PushSignal(1.);
        break;
    case ParseHttpResponseFailed:
        Asr1ErrorParseHttpResponse_->PushSignal(1.);
        break;
    default:
        Asr1ErrorUnspecific_->PushSignal(1.);
    }
}

void TUnistatN::OnAsr1SpotterError(int code) {
    Asr1SpotterErrorAll_->PushSignal(1.);
    switch (code) {
    case 400:
        Asr1SpotterErrorBadMessage_->PushSignal(1.);
        break;
    case 404:
        Asr1SpotterErrorInvalidParams_->PushSignal(1.);
        break;
    case 408:
        Asr1SpotterErrorTimeout_->PushSignal(1.);
        break;
    case 500:
        Asr1SpotterErrorInternal_->PushSignal(1.);
        break;
    case SpotterErrorDeadline:
        Asr1SpotterErrorDeadline_->PushSignal(1.);
        break;
    case SpotterRequestFailed:
        Asr1SpotterErrorRequest_->PushSignal(1.);
        break;
    default:
        Asr1SpotterErrorUnspecific_->PushSignal(1.);
    }
}

void TUnistatN::OnAsr1SpotterCancel() {
    Asr1SpotterCancel_->PushSignal(1.);
}

void TUnistatN::OnReceiveFromAppHostReqRaw(size_t size) {
    RecvFromAppHostReqRawBytes_->PushSignal(size);
}

void TUnistatN::OnReceiveFromAppHostRaw(size_t size) {
    RecvFromAppHostItems_->PushSignal(1.);
    RecvFromAppHostRawBytes_->PushSignal(size);
}

void TUnistatN::OnReceiveFromAsrRaw(size_t size) {
    RecvFromAsrRawBytes_->PushSignal(size);
}

void TUnistatN::OnSendToAppHostRaw(size_t size) {
    SendToAppHostItems_->PushSignal(1.);
    SendToAppHostRawBytes_->PushSignal(size);
}

void TUnistatN::OnSendToAsrRaw(size_t size) {
    SendToAsrRawBytes_->PushSignal(size);
}
