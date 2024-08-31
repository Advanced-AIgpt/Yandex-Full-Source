#include <alice/rtlog/agent/impl/server.h>
#include <alice/rtlog/agent/protos/config.pb.h>

#include <library/cpp/getopt/opt.h>
#include <robot/rthub/misc/signals.h>
#include <robot/rthub/misc/common.h>
#include <ydb/library/yql/utils/backtrace/backtrace.h>

using namespace NRTHub;
using namespace NRTLogAgent;

class TCommandLine {
public:
    ::NRTLogAgent::TConfig Config;

public:
    TCommandLine(int argc, const char* argv[]) {
        NLastGetopt::TOpts opts;
        TString configPath;

        opts
            .AddLongOption('c', "config", "path to config")
            .RequiredArgument("PATH")
            .Required()
            .StoreResult(&configPath);

        opts.AddHelpOption();
        opts.AddVersionOption();
        NLastGetopt::TOptsParseResult res(&opts, argc, argv);

        ParseFromTextFormat(configPath, Config);
    }
};

int main(int argc, const char* argv[]) {
    SetFatalSignalHandler();
    EnableKikimrBacktraceFormat();
    SetShutdownSignalHandler();

    TCommandLine commandLine(argc, argv);
    Log(ELogType::Info, "rtlog agent starting");
    auto server = MakeServer(commandLine.Config);
    server->Start();
    Log(ELogType::Info, "rtlog agent started");
    WaitForShutdown();
    server->Stop();
    Log(ELogType::Info, "rtlog agent stopped");
    return 0;
}
