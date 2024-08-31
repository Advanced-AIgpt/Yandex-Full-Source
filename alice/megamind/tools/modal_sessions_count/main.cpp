#include "config_helpers.h"
#include "mapper.h"

#include <alice/library/yt/util.h>

#include <mapreduce/yt/common/config.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/logging/logger.h>
#include <mapreduce/yt/util/ypath_join.h>

#include <library/cpp/getopt/last_getopt.h>

using namespace NAlice::NModalSessionMapper;
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
    TString megamindLogPath;
    TString startTable, endTable;
    TString outputTable;
    TString configPath;

    NLastGetopt::TOpts opts;
    opts.AddLongOption('s', "server", "YT server").Required().RequiredArgument().StoreResult(&server);
    opts.AddLongOption("megamind-log", "Input YT path (table >>>//logs/megamind-log/1d/<<<TABLE)")
        .Required()
        .RequiredArgument()
        .StoreResult(&megamindLogPath);
    opts.AddLongOption("from", "Start table name (table //logs/megamind-log/1d/>>>TABLE<<<)")
        .Required()
        .RequiredArgument()
        .StoreResult(&startTable);
    opts.AddLongOption("to", "End table name (table //logs/megamind-log/1d/>>>TABLE<<<). Can be the same as `from`"
                             " if a single table is required")
        .Required()
        .RequiredArgument()
        .StoreResult(&endTable);
    opts.AddLongOption('c', "config", "The megamind production config path")
        .Required()
        .RequiredArgument()
        .StoreResult(&configPath);
    opts.AddLongOption('o', "output", "Output YT path (table)")
        .Required()
        .RequiredArgument()
        .StoreResult(&outputTable);
    NLastGetopt::TOptsParseResult optsResult(&opts, argc, argv);
    Y_ENSURE(startTable <= endTable, "End table should be greater than start table!");

    SetLogger(CreateStdErrLogger(NYT::ILogger::INFO));
    TConfig::Get()->UseClientProtobuf = false;

    auto ytClient = CreateClient(server);
    Y_ASSERT(ytClient);

    auto scenarioConfig = ScenarioMaxTurnsFromCmdline(configPath);
    auto tables = GetTablesFromRange(ytClient, megamindLogPath, startTable, endTable);
    ComputeModalStats(scenarioConfig, ytClient, tables, outputTable);

    return 0;
}
