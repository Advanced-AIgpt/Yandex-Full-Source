#include "alice/megamind/tools/slowest_scenario/library/mapper.h"

#include <alice/library/yt/util.h>

#include <mapreduce/yt/common/config.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/logging/logger.h>
#include <mapreduce/yt/util/ypath_join.h>

#include <library/cpp/getopt/last_getopt.h>

using namespace NAlice::NSlowestScenario;
using namespace NYT;

namespace {

TTableMapping GetTablesFromRange(IClientPtr ytClient, const TString& dir, const TString& from, const TString& to) {
    Y_ENSURE(ytClient);

    TTableMapping mapping;
    NAliceYT::ForEachInMapNode(*ytClient, dir, ENodeType::NT_TABLE, [&](const TString& name) {
        if (name >= from && name <= to) {
            mapping.push_back(JoinYPaths(dir, name));
        }
    });
    Sort(mapping);
    return mapping;
}

} // namespace

int main(int argc, const char** argv) {
    Initialize(argc, argv);

    TString server;
    TString analyticsLogPath;
    TString startTable, endTable;
    TString outputTable;
    TString clientRegexp;
    bool shouldSplitVinsIntents = false;
    bool useWonderlogs = false;

    NLastGetopt::TOpts opts;
    opts.AddLongOption('s', "server", "YT server").Required().RequiredArgument().StoreResult(&server);
    opts.AddLongOption("megamind-log", "Input YT path (table >>>//logs/megamind-log/1d/<<<TABLE)")
        .Required()
        .RequiredArgument()
        .StoreResult(&analyticsLogPath);
    opts.AddLongOption("from", "Start table name (table //logs/megamind-log/1d/>>>TABLE<<<)")
        .Required()
        .RequiredArgument()
        .StoreResult(&startTable);
    opts.AddLongOption("to", "End table name (table //logs/megamind-log/1d/>>>TABLE<<<). Can be same as `from`"
                             " if a single table is required")
        .RequiredArgument()
        .Required()
        .StoreResult(&endTable);
    opts.AddLongOption('o', "output", "Output YT path (table)")
        .Required()
        .RequiredArgument()
        .StoreResult(&outputTable);
    opts.AddLongOption("client-regexp", "Regexp for client filtering")
        .Optional()
        .RequiredArgument()
        .StoreResult(&clientRegexp);
    opts.AddLongOption("split-vins-intents", "Use Vins intent name instead of Vins if possible")
        .NoArgument()
        .SetFlag(&shouldSplitVinsIntents);
    opts.AddLongOption("use-wonderlogs", "Use wonderlogs instead of megamind-log")
        .NoArgument()
        .SetFlag(&useWonderlogs);
    NLastGetopt::TOptsParseResult optsResult(&opts, argc, argv);
    Y_ENSURE(startTable <= endTable, "End table should be greater than start table!");

    SetLogger(CreateStdErrLogger(NYT::ILogger::INFO));
    TConfig::Get()->UseClientProtobuf = false;

    auto ytClient = CreateClient(server);
    Y_ASSERT(ytClient);

    auto tables = GetTablesFromRange(ytClient, analyticsLogPath, startTable, endTable);
    const auto computeTimingsDelegate = useWonderlogs ? ComputeTimingsOverWonderLogs : ComputeTimings;
    computeTimingsDelegate(clientRegexp, shouldSplitVinsIntents, ytClient, tables, outputTable);

    return 0;
}
