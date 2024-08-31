#include <util/string/cast.h>

#include <library/cpp/neh/http2.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/rpc.h>

#include <library/cpp/unistat/unistat.h>

#include <library/cpp/getoptpb/getoptpb.h>

#include <alice/cuttlefish/tools/fdstatsrv/config.pb.h>

#include "metrics.h"
#include "fs.h"
#include "netstat.h"
#include "ulimit.h"


int main(int argc, const char **argv) {
    NGetoptPb::TGetoptPbSettings getOptSettings;
    getOptSettings.PrettyOpts = true;
    getOptSettings.DontRequireRequired = false;
    getOptSettings.DumpConfig = false;

    TStatsServiceConfig config;

    TString errorMessage;
    if (!NGetoptPb::GetoptPb(argc, argv, config, errorMessage, getOptSettings)) {
        Cerr << errorMessage << Endl;
        return 1;
    }

    NNeh::IServicesRef srv = NNeh::CreateLoop();

    const TString endpoint = TString("http://*:") + ToString(config.GetPort()) + "/fdstat";

    TMetrics metrics;

    std::vector<THolder<IMetricsUpdater>> updaters;
    updaters.emplace_back(MakeHolder<TFileHandlerMetricsUpdater>());
    updaters.emplace_back(MakeHolder<TNetstatMetricsUpdater>());
    updaters.emplace_back(MakeHolder<TUlimitMetricsUpdater>());

    srv->Add(endpoint, [&updaters, &metrics](const NNeh::IRequestRef& request) {
        NNeh::IHttpRequest *req = dynamic_cast<NNeh::IHttpRequest*>(request.Get());

        if (!req) {
            request->SendError(NNeh::IRequest::BadRequest, "request is bad");
        }

        for (auto& up : updaters) {
            up->UpdateMetrics(metrics);
        }

        const TString stats = metrics.DumpJson();
        const TString headers = "\r\nContent-Type: application/json";
        NNeh::TData data(stats.data(), stats.data() + stats.size());
        req->SendReply(data, headers, 200);
    });

    srv->Loop(2);

    return 0;
}
