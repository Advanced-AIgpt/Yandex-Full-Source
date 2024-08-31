#include "subsystem_metrics.h"

namespace NGProxy {

void TMetricsSubsystem::Init() {
    NVoice::NMetrics::TMetrics& metrics = NVoice::NMetrics::TMetrics::Instance();

    NVoice::NMetrics::TAggregationRules rules;

    metrics.SetBackend(
        NVoice::NMetrics::EMetricsBackend::Solomon,
        MakeHolder<NVoice::NMetrics::TSolomonBackend>(rules, NVoice::NMetrics::MakeMillisBuckets())
    );

    GrpcLabels.GroupName = "gsetup";
    GrpcLabels.SubgroupName = "all";
    GrpcLabels.AppId = "any";
    GrpcLabels.DeviceName = "grpc";
    GrpcLabels.ClientType = NVoice::NMetrics::EClientType::User;

    AppHostLabels.GroupName = "gproxy";
    AppHostLabels.SubgroupName = "all";
    AppHostLabels.AppId = "any";
    AppHostLabels.DeviceName = "apphost";
    AppHostLabels.ClientType = NVoice::NMetrics::EClientType::User;
}

}   // namespace NGProxy
