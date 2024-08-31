#include "unistat.h"

using namespace NAlice::NYabioAdapter;

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
    CreateYabioClient_ = CreateCounter("yabio_client_create");
    YabioClientCounter_ = CreateCounter("yabio_client_counter", "ammx");

    YabioErrorAll_ = CreateCounter("yabio_error_all");
    YabioErrorBadMessage_ = CreateCounter("yabio_error_bad_message");
    YabioErrorInvalidParams_ = CreateCounter("yabio_error_invalid_params");
    YabioErrorTimeout_ = CreateCounter("yabio_error_timeout");
    YabioErrorInternal_ = CreateCounter("yabio_error_internal");
    YabioErrorUnspecific_ = CreateCounter("yabio_error_unspecific");

    YabioSpotterErrorAll_ = CreateCounter("yabio_spotter_error_all");
    YabioSpotterErrorBadMessage_ = CreateCounter("yabio_spotter_error_bad_message");
    YabioSpotterErrorInvalidParams_ = CreateCounter("yabio_spotter_error_invalid_params");
    YabioSpotterErrorTimeout_ = CreateCounter("yabio_spotter_error_timeout");
    YabioSpotterErrorInternal_ = CreateCounter("yabio_spotter_error_internal");
    YabioSpotterErrorDeadline_ = CreateCounter("yabio_spotter_error_deadline");
    YabioSpotterErrorRequest_ = CreateCounter("yabio_spotter_error_request");
    YabioSpotterErrorUnspecific_ = CreateCounter("yabio_spotter_error_unspecific");

    YabioWarning_ = CreateCounter("yabio_warning");
/*
    AppHostRequestTimings_ = CreateHgramCounter("ah_request_timings", HIGH_RESOLUTION_COUNTER_VALUES);
    ClientLastSynthTimings_ = CreateHgramCounter("client_last_synth", LOW_RESOLUTION_COUNTER_VALUES);
*/
}

void TUnistatN::OnCreateYabioClient() {
    CreateYabioClient_->PushSignal(1.);
}

void TUnistatN::OnYabioError(int code) {
    YabioErrorAll_->PushSignal(1.);
    switch (code) {
    case 400:
        YabioErrorBadMessage_->PushSignal(1.);
        break;
    case 404:
        YabioErrorInvalidParams_->PushSignal(1.);
        break;
    case 408:
        YabioErrorTimeout_->PushSignal(1.);
        break;
    case 500:
        YabioErrorInternal_->PushSignal(1.);
        break;
    default:
        YabioErrorUnspecific_->PushSignal(1.);
    }
}

void TUnistatN::OnYabioWarning() {
    YabioWarning_->PushSignal(1.);
}
