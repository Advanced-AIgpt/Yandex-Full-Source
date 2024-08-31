#include <alice/quality/music_search_clicks/lib/clicks_computer.h>

#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/last_getopt.h>


REGISTER_REDUCER(NAlice::NMusicWebSearch::NListeningCounters::TCollectCountersReducer);


int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    NLastGetopt::TOpts options;
    options.SetFreeArgsNum(0);

    TString proxyName;
    options.AddLongOption('p', "proxy", "YT proxy name")
        .Required()
        .RequiredArgument("PROXY")
        .StoreResult(&proxyName);

    NYT::TYPath inputTable;
    options.AddLongOption('i', "input", "input table with user music listenings")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&inputTable);

    NYT::TYPath outputTable;
    options.AddLongOption('o', "output", "output table with counters")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&outputTable);

    NLastGetopt::TOptsParseResult parseOpts{&options, argc, argv};

    const auto client = NYT::CreateClient(proxyName);

    client->MapReduce(
        NYT::TMapReduceOperationSpec()
            .AddInput<NAlice::NMusicWebSearch::NListeningCounters::TUserListening>(inputTable)
            .AddOutput<NAlice::NMusicWebSearch::NListeningCounters::TFullState>(outputTable)
            .ReduceBy({"Puid"})
            .SortBy({"Puid", "Timestamp"}),
        /* mapper */ nullptr,
        new NAlice::NMusicWebSearch::NListeningCounters::TCollectCountersReducer,
        NYT::TOperationOptions()
            .InferOutputSchema(true));

    return 0;
}
