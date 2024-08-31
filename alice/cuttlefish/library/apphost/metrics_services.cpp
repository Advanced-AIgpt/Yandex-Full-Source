#include "metrics_services.h"

#include <alice/cuttlefish/library/metrics/metrics.h>

#include <library/cpp/neh/http_common.h>

namespace NAlice::NCuttlefish {

void GolovanMetricsService(const NNeh::IRequestRef& raw) {
    NNeh::TRequestOut out(raw.Get());
    out << TUnistat::Instance().CreateJsonDump(0, true);
}

void SolomonMetricsService(const NNeh::IRequestRef& raw) {
    NVoice::NMetrics::TSourceMetrics metrics(
        "Solomon",
        NVoice::NMetrics::TSourceMetrics::MakeEmptyClientInfo(),
        NVoice::NMetrics::EMetricsBackend::Solomon
    );

    NNeh::IHttpRequest *req = dynamic_cast<NNeh::IHttpRequest*>(raw.Get());
    if (!req) {
        return;
    }

    TCgiParameters cgi;
    cgi.Scan(req->Cgi());
    NVoice::NMetrics::EOutputFormat format = NVoice::NMetrics::EOutputFormat::SPACK_LZ4;
    TString requestedFormat = cgi.Get("format");
    if (requestedFormat == "json"sv) {
        format = NVoice::NMetrics::EOutputFormat::JSON;
    }

    NNeh::TData data;
    {
        TStringStream ss;
        NVoice::NMetrics::TMetrics::Instance().SerializeMetrics(NVoice::NMetrics::EMetricsBackend::Solomon, ss, format);
        const TString str = ss.Str();
        data = NNeh::TData(str.data(), str.data() + str.size());
    }

    if (format == NVoice::NMetrics::EOutputFormat::JSON) {
        req->SendReply(data, "\r\nContent-Type: application/json");
    } else {
        req->SendReply(data, "\r\nContent-Type: application/x-solomon-spack");
    }
}

} // namespace NAlice::NCuttlefish
