#include "sensors.h"

#include <alice/library/metrics/names.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/metrics/util.h>

#include <util/string/cast.h>

namespace NAlice::NMegamind {
namespace {

NMonitoring::TLabels SignalForStage(EUniproxyStage stage, ERequestResult result) {
    NMonitoring::TLabels signal;
    switch (stage) {
        case EUniproxyStage::Run:
            signal = NSignal::HANDLER_RUN;
            break;
        case EUniproxyStage::Apply:
            signal = NSignal::HANDLER_APPLY;
            break;
    }
    signal.Add("result", ToString(result));
    return signal;
}

} // namespace

TRequestTimeMetrics::TRequestTimeMetrics(NMetrics::ISensors& sensors, TRTLogger& log,
                                         TStringBuf nodeName, EUniproxyStage stage, ERequestResult result)
    : Sensors_{sensors}
    , Log_{log}
    , NodeName_{nodeName}
    , Stage_{stage}
    , Result_{result}
{
}

void TRequestTimeMetrics::SetStartTime(TInstant startTime) {
    StartTime_ = startTime;
}

void TRequestTimeMetrics::SetSkrInfo(const TString& clientName, const TString& requestPath) {
    ClientName_ = clientName;
    RequestPath_ = requestPath;
}

void TRequestTimeMetrics::UpdateLabels(NMonitoring::TLabels& labels) const {
    labels.Add("client", ClientName_);
    labels.Add("path", RequestPath_);
    labels.Add("node", NodeName_);
}

TRequestTimeMetrics::~TRequestTimeMetrics() {
    try {
        if (ClientName_.Empty()) {
            LOG_WARN(Log_) << "Update time metrics: ClientName is empty";
        }

        if (RequestPath_.Empty()) {
            LOG_WARN(Log_) << "Update time metrics: RequestPath is empty";
        }

        if (StartTime_.Defined()) {
            auto time = SignalForStage(Stage_, Result_);
            UpdateLabels(time);
            Sensors_.AddHistogram(time, (TInstant::Now() - *StartTime_).MilliSeconds(), NMetrics::TIME_INTERVALS);
        }

        NMonitoring::TLabels client{{"name", "speechkit_request"}};
        UpdateLabels(client);
        Sensors_.IncRate(client);
    }
    catch (...) {
        LOG_ERR(Log_) << "Exception during add histogram: " << CurrentExceptionMessage();
    }
}

void TRequestTimeMetrics::UpdateResult(ERequestResult result) {
    Result_ = result;
}

} // NAlice::NMegamind
