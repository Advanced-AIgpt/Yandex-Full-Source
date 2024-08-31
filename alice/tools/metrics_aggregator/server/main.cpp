#include <alice/tools/metrics_aggregator/library/util.h>

#include <apphost/api/service/cpp/service_loop.h>

#include <library/cpp/getopt/opt.h>

#include <library/cpp/neh/rpc.h>

#include <util/system/backtrace.h>
#include <util/system/spinlock.h>

#include <tuple>

namespace {

struct TInternalData {
    TAdaptiveLock Lock;
    TString Sensors;
};

void ApplyBatchHandler(const NNeh::IRequestRef& req, TInternalData& internalData) {
    with_lock(internalData.Lock) {
        internalData.Sensors = TString{req->Data()};
    }

    NNeh::TDataSaver ds;
    ds << "Ok";
    req->SendReply(ds);
}

void DumpSolomonCounters(const NNeh::IRequestRef& req, TInternalData& internalData) {
    TString answer;
    with_lock(internalData.Lock) {
        answer = internalData.Sensors;
    }
    NNeh::TDataSaver ds;
    ds << answer;
    req->SendReply(ds);
}

} // namespace anonymous

int main(int argc, const char** argv) {
    NLastGetopt::TOpts options;

    ui32 port;
    options.AddLongOption('p', "port", "Port to run server on").Required().StoreResult(&port);
    ui32 threads;
    options.AddLongOption('t', "threads", "Number of threads to handle requests").Required().StoreResult(&threads);
    NLastGetopt::TOptsParseResult res(&options, argc, argv);

    TInternalData internalData;

    const std::tuple<TString, void (*)(const NNeh::IRequestRef&, TInternalData&)> handlers[] = {
        {NMetricsAggregator::BATCH_PATH, ApplyBatchHandler},

        {"/solomon/json", DumpSolomonCounters},
    };

    NAppHost::TLoop loop;
    for (const auto& [path, handler] : handlers) {
        loop.Add(port, path, [&internalData, handler = handler](const NNeh::IRequestRef& req) {
            try {
                handler(req, internalData);
            } catch(...) {
                Cerr << "Error: " << TBackTrace::FromCurrentException().PrintToString() << Endl;
                throw;
            }
        });
        Cerr << "Added http path " << path << Endl;
    }

    loop.Loop(threads);
}
