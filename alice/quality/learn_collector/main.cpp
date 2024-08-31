#include "collect_learn.h"

#include <library/cpp/getopt/last_getopt.h>

#include <mapreduce/yt/client/client.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/init.h>
#include <mapreduce/yt/util/ypath_join.h>

#include <util/draft/date.h>
#include <util/generic/string.h>

int main(int argc, char** argv)
{
    NYT::Initialize(argc, argv);

    NLastGetopt::TOpts opts;

    opts.AddHelpOption('h');

    NYT::TYPath outputPath;
    opts.AddLongOption('o', "output", "path to output results")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&outputPath);

    NYT::TYPath tmpDir;
    opts.AddLongOption('t', "tmp", "directory to keep tmp tables in")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&tmpDir);

    TDate startDate;
    opts.AddLongOption('s', "start-date", "date to start collecting from")
        .Required()
        .RequiredArgument("DATE")
        .StoreResult(&startDate);

    TDate endDate;
    opts.AddLongOption('e', "end-date", "date to stop collecting at")
        .Required()
        .RequiredArgument("DATE")
        .StoreResult(&endDate);

    TString proxy;
    opts.AddLongOption('p', "proxy", "yt proxy")
        .Required()
        .RequiredArgument("STR")
        .StoreResult(&proxy);

    ui32 maxOperations = 4;
    opts.AddLongOption('m', "max-operations", "how many YT operations can run at one time")
        .DefaultValue(maxOperations)
        .RequiredArgument("NUM")
        .StoreResult(&maxOperations);

    ui32 maxContextDepth = 2;
    opts.AddLongOption('c', "max-context", "maximum depth of request in a session")
        .DefaultValue(maxContextDepth)
        .RequiredArgument("NUM")
        .StoreResult(&maxContextDepth);

    bool collectQuasar = false;
    opts.AddLongOption('q', "quasar", "collect learn for quasar (collect for general by default)")
        .NoArgument()
        .SetFlag(&collectQuasar);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult parseOpts{&opts, argc, argv};
    auto client = NYT::CreateClient(proxy);
    if (tmpDir.back() == '/') {
        tmpDir.pop_back();
    }
    if (!client->Exists(tmpDir)) {
        client->Create(tmpDir, NYT::NT_MAP, NYT::TCreateOptions().Recursive(true));
    } else if (client->Get(NYT::JoinYPaths(tmpDir, "@type")).AsString() != "map_node") {
        ythrow yexception() << "Expected: " << tmpDir << " to be type NT_MAP, but got " << client->Get(tmpDir + "/@type").AsString();
    }
    const auto platform = collectQuasar ? NAlice::EPlatformType::Quasar : NAlice::EPlatformType::General;
    NAlice::CollectDates(startDate, endDate, outputPath, tmpDir, client, platform, maxContextDepth, maxOperations);
}
