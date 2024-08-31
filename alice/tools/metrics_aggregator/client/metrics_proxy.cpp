#include "metrics_proxy.h"

#include <library/cpp/neh/neh.h>
#include <library/cpp/neh/http_common.h>

namespace NMetricsAggregator {

namespace {

TString MakeMetricsAggregatorUrl(const int port, const TString& path) {
    return TStringBuilder{} << "http://localhost:" << port << path;
}

void MakeRequest(const TString& url, const TString& body) {
    NNeh::TMessage msg;
    msg.Addr = url;
    msg.Data = "";
    NNeh::NHttp::MakeFullRequest(msg, "", body, "application/json");

    NNeh::TResponseRef resp = NNeh::Request(msg)->Wait(TDuration::Seconds(10));
    if (!resp) {
        Cerr << "Failed to push data to metrics aggregator" << Endl;
        return;
    }
    if (resp->IsError()) {
        Cerr << "Error: " << resp->GetErrorText() << ", url: " << url << ", body: " << body << Endl;;
    }
}

} // namespace anonymous


TMetricsProxy::TMetricsProxy(int port)
    : BatchUrl_(MakeMetricsAggregatorUrl(port, BATCH_PATH))
    , SolomonSensors_{MakeAtomicShared<NMonitoring::TMetricRegistry>()}
    , SensorsDumper_(*SolomonSensors_)
{
    Scheduler_.Schedule([this](){return this->PushMetrics();});
}

void TMetricsProxy::AddRate(NMonitoring::TLabels&& labels, i32 delta) {
    SolomonSensors_->Rate(std::move(labels))->Add(delta);
}

void TMetricsProxy::IncRate(NMonitoring::TLabels&& labels) {
    AddRate(std::move(labels), 1);
}

void TMetricsProxy::AddHistogram(NMonitoring::TLabels&& labels, ui64 value, const TVector<double>& bins) {
    SolomonSensors_->HistogramRate(std::move(labels), NMonitoring::ExplicitHistogram(bins))->Record(value);
}

void TMetricsProxy::AddIntGauge(NMonitoring::TLabels&& labels, i64 value) {
    SolomonSensors_->IntGauge(std::move(labels))->Add(value);
}

void TMetricsProxy::SetIntGauge(NMonitoring::TLabels&& labels, i64 value) {
    SolomonSensors_->IntGauge(std::move(labels))->Set(value);
}

TDuration TMetricsProxy::PushMetrics() {
    MakeRequest(BatchUrl_, SensorsDumper_.Dump("solomon"));

    return TDuration::Seconds(1);
}

} // namespace NMetricsAggregator
