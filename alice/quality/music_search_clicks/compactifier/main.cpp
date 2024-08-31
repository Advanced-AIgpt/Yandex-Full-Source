#include <alice/quality/music_search_clicks/lib/clicks_computer.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/util/temp_table.h>

#include <library/cpp/getopt/last_getopt.h>


REGISTER_MAPPER(NAlice::NMusicWebSearch::NListeningCounters::TCountersCompactifier);


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
    options.AddLongOption('i', "input", "input table with counters after reducer")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&inputTable);

    NYT::TYPath outputTable;
    options.AddLongOption('o', "output", "output table with compactified counters")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&outputTable);

    bool force = false;
    options.AddLongOption('f', "force", "output table will be overwritten if exists")
        .StoreTrue(&force);

    NLastGetopt::TOptsParseResult parseOpts{&options, argc, argv};

    const auto client = NYT::CreateClient(proxyName);

    const NYT::TTempTable unsortedOutput(client);

    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NAlice::NMusicWebSearch::NListeningCounters::TFullState>(inputTable)
            .AddOutput<NAlice::NMusicWebSearch::NListeningCounters::TCompactState>(unsortedOutput.Name()),
        new NAlice::NMusicWebSearch::NListeningCounters::TCountersCompactifier
    );

    // patching original schema to be convertible to dynamic table
    NYT::TTableSchema schema = NAlice::NMusicWebSearch::NListeningCounters::CreateCompactTableSchema();
    schema.UniqueKeys(true);
    schema.SortBy({"Hash", "Id"});

    NYT::TNode sortedSchema;
    NYT::TNodeBuilder builder{&sortedSchema};
    NYT::Serialize(schema, &builder);

    client->CreateTable<NAlice::NMusicWebSearch::NListeningCounters::TCompactState>(
        outputTable,
        {"Hash", "Id"},
        NYT::TCreateOptions()
            .Force(force)
            .Attributes(NYT::TNode{}("schema", sortedSchema))
    );

    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(unsortedOutput.Name())
            .Output(outputTable)
            .SortBy({"Hash", "Id"}),
        NYT::TOperationOptions().Spec(
            NYT::TNode{} // taken from https://yt.yandex-team.ru/docs/description/dynamic_tables/dynamic_tables_mapreduce#ispolzovanie-vychislyaemyh-kolonok
                ("partition_job_io", NYT::TNode()("table_writer", NYT::TNode()("block_size", 256 * 1024)))
                ("merge_job_io", NYT::TNode()("table_writer", NYT::TNode()("block_size", 256 * 1024)))
                ("sort_job_io", NYT::TNode()("table_writer", NYT::TNode()("block_size", 256 * 1024)))
        )
    );

    return 0;
}
