#include "server.h"

#include <library/cpp/getopt/last_getopt.h>

#include <util/string/cast.h>
#include <util/string/split.h>

using namespace NNlgServer;

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
        .DefaultValue("south_park")
        .Help("One of { south_park, general_conversation }.\n\n\n");
    opts
        .AddLongOption("model-dir")
        .OptionalArgument("MODEL_DIR")
        .DefaultValue(".")
        .Help("Directory with *.dict and encoder_decoder dirs.\n\n\n");

    TOptsParseResult res(&opts, argc, argv);

    TNlgServer server(res.Get<int>("port"), res.Get<int>("thread_count"));
    server.LoadModels(res.Get("model-dir"), res.Get("mode"));
    server.HandleRequests();

    return 0;
}
