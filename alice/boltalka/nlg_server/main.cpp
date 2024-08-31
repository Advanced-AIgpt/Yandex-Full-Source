#include "server.h"

#include <cv/imgclassifiers/danet/backend/cpu_mkl/factory.h>
#include <cv/imgclassifiers/danet/common/danet_env/danet_env.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/string/cast.h>
#include <util/string/split.h>

using namespace NNlgServer;
using namespace NNeuralNet;

int main(int argc, char** argv)
{
    using namespace NLastGetopt;
    TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "port")
        .OptionalArgument("PORT")
        .DefaultValue("1612")
        .Help("Http-port.\n\n\n");
    opts
        .AddLongOption('t', "thread_count")
        .OptionalArgument("THREAD_COUNT")
        .DefaultValue("8")
        .Help("Thread count.\n\n\n");
    opts
        .AddLongOption("mode")
        .OptionalArgument("MODE")
        .DefaultValue("general_conversation")
        .Help("One of { general_conversation, south_park }.\n\n\n");
    opts
        .AddLongOption("model-dir")
        .OptionalArgument("MODEL_DIR")
        .DefaultValue(".")
        .Help("Directory model directories.\n\n\n");

    if (TCpuMKLBackendFactory().IsMKLAvailable()) {
        Cout << "Setting MKL backend." << Endl;
        DanetEnv().SetCpuBackend(new TCpuMKLBackendFactory());
    }

    TOptsParseResult res(&opts, argc, argv);

    TNlgServer server(res.Get<int>("port"), res.Get<int>("thread_count"));
    server.LoadModels(res.Get("model-dir"), res.Get("mode"));
    server.HandleRequests();

    return 0;
}
