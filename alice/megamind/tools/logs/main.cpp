#include <alice/megamind/tools/logs/errors_stats.h>
#include <alice/megamind/tools/logs/requests_stats.h>
#include <alice/megamind/tools/logs/table_preparer.h>
#include <alice/megamind/tools/logs/timing_stats.h>

#include <mapreduce/yt/common/config.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/logging/logger.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

using namespace NYT;

int RunTimingStats(int argc, const char** argv) {
    NLastGetopt::TOpts opts;

    TString server;
    opts.AddLongOption('s', "server", "YT server")
        .Required().RequiredArgument().StoreResult(&server);
    TString uniproxyLog;
    opts.AddLongOption("uniproxy-log", "Input YT path (table //logs/alice-production-uniproxy/1d/TABLE)")
        .Required().RequiredArgument().StoreResult(&uniproxyLog);
    TString megamindLog;
    opts.AddLongOption("megamind-log", "Input YT path (table //logs/megamind-log/1d/TABLE)")
        .Optional().RequiredArgument().StoreResult(&megamindLog);
    TString megamindAnalyticsLog;
    opts.AddLongOption("megamind-analytics-log", "Input YT path (table //logs/megamind-analytics-log/1d/TABLE)")
        .Required().RequiredArgument().StoreResult(&megamindAnalyticsLog);
    TString output;
    opts.AddLongOption('o', "output", "Output YT path (table)")
        .Required().RequiredArgument().StoreResult(&output);
    bool force;
    opts.AddLongOption('f', "force", "Force rewrite destination tables")
        .NoArgument().SetFlag(&force);
    i64 memoryLimit;
    opts.AddLongOption("memory-limit", "Jobs memory limit in gigabytes")
        .DefaultValue(4ULL).StoreResult(&memoryLimit);

    NLastGetopt::TOptsParseResult optsResult(&opts, argc, argv);

    NYT::TUserJobSpec jobSpec;
    jobSpec.MemoryLimit(memoryLimit << 30ULL);  // `memoryLimit` Gb

    auto client = NYT::CreateClient(server);
    NMegamindLog::PrepareTimingTable(client, jobSpec, uniproxyLog, megamindAnalyticsLog, output);

    return 0;
}

int RunUniproxyVinsTimings(int argc, const char** argv) {
    NLastGetopt::TOpts opts;

    TString server;
    opts.AddLongOption('s', "server", "YT server")
        .Required().RequiredArgument().StoreResult(&server);
    TString input;
    opts.AddLongOption('i', "input", "Input YT path (table after timing-stats)")
        .Required().RequiredArgument().StoreResult(&input);
    TString output;
    opts.AddLongOption('o', "output", "Output YT path (table)")
        .Required().RequiredArgument().StoreResult(&output);
    bool force;
    opts.AddLongOption('f', "force", "Force rewrite destination tables")
        .NoArgument().SetFlag(&force);

    NLastGetopt::TOptsParseResult optsResult(&opts, argc, argv);

    auto client = NYT::CreateClient(server);
    NMegamindLog::PrepareUniproxyVinsTimings(client, input, output);

    return 0;
}

int RunSimpleStats(const NMegamindLog::TSimpleTablePreparer& preparer, int argc, const char** argv) {
    NLastGetopt::TOpts opts;

    TString server;
    opts.AddLongOption('s', "server", "YT server")
        .Required().RequiredArgument().StoreResult(&server);
    TString megamindLog;
    opts.AddLongOption("megamind-log", "Input YT path (table //logs/megamind-log/1d/TABLE)")
        .Required().RequiredArgument().StoreResult(&megamindLog);
    TString output;
    opts.AddLongOption('o', "output", "Output YT path (table)")
        .Required().RequiredArgument().StoreResult(&output);

    NLastGetopt::TOptsParseResult optsResult(&opts, argc, argv);

    auto client = NYT::CreateClient(server);
    preparer.Prepare(client, megamindLog, output);

    return 0;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    NYT::SetLogger(NYT::CreateStdErrLogger(NYT::ILogger::INFO));
    NYT::TConfig::Get()->UseClientProtobuf = false;

    TModChooser mc;
    mc.AddMode("timing-stats", RunTimingStats, "make table with timing stats");
    mc.AddMode("uniproxy-vins-timings", RunUniproxyVinsTimings, "make table with UniproxyVinsTimings before sending to stat");

    using namespace std::placeholders; // for _1, _2, _3...
    mc.AddMode("requests-stats", std::bind(RunSimpleStats, NMegamindLog::TRequestsStatsTablePreparer(), _1, _2), "make table with requests stats");
    mc.AddMode("errors-stats", std::bind(RunSimpleStats, NMegamindLog::TErrorsStatsTablePreparer(), _1, _2), "make table with errors stats");
    return mc.Run(argc, argv);
}

