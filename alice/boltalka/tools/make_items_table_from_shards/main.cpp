#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>

class TFilterMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TFilterMapper() = default;

    TFilterMapper(size_t maxShards, size_t maxItems, const TString& columnPrefix, ui64 docIdShift)
        : MaxShards(maxShards)
        , MaxItems(maxItems)
        , ContextColumn("context_embedding")
        , ReplyColumn("reply_embedding")
        , DocIdShift(docIdShift)
    {
        if (columnPrefix != "") {
            ContextColumn = columnPrefix + ":" + ContextColumn;
            ReplyColumn = columnPrefix + ":" + ReplyColumn;
        }
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            ui64 shardId = row["shard_id"].AsUint64();
            ui64 id = row["doc_id"].AsUint64() + DocIdShift;
            if (shardId >= MaxShards || id >= MaxItems) {
                return;
            }
            output->AddRow(NYT::TNode()
                ("shard_id", shardId)
                ("doc_id", id)
                ("item", row[ContextColumn].AsString() + row[ReplyColumn].AsString())
            );
        }
    }

    Y_SAVELOAD_JOB(MaxShards, MaxItems, ContextColumn, ReplyColumn, DocIdShift);

private:
    size_t MaxShards;
    size_t MaxItems;
    TString ContextColumn;
    TString ReplyColumn;
    ui64 DocIdShift;
};
REGISTER_MAPPER(TFilterMapper);

int main_make_items(int argc, const char** argv) {
    TString serverProxy;
    TString inputTable;
    TString outputTable;
    size_t maxShards;
    size_t maxItems;
    TString columnPrefix;
    ui64 docIdShift;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .DefaultValue("hahn")
        .StoreResult(&serverProxy)
        .Help("YT server.\n\n\n");
    opts
        .AddLongOption('i', "input")
        .RequiredArgument("TABLE")
        .Required()
        .StoreResult(&inputTable);
    opts
        .AddLongOption('o', "output")
        .RequiredArgument("TABLE")
        .Required()
        .StoreResult(&outputTable);
    opts
        .AddLongOption('s', "max-shards")
        .DefaultValue("1")
        .StoreResult(&maxShards);
    opts
        .AddLongOption('v', "max-items")
        .DefaultValue("100000")
        .StoreResult(&maxItems);
    opts
        .AddLongOption("column-prefix")
        .DefaultValue("")
        .StoreResult(&columnPrefix);
    opts
        .AddLongOption("doc-id-shift")
        .DefaultValue(0)
        .StoreResult(&docIdShift);

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');
    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    auto client = NYT::CreateClient(serverProxy);
    client->Create(outputTable, NYT::NT_TABLE, NYT::TCreateOptions().Recursive(true).IgnoreExisting(true));

    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(inputTable)
            .AddOutput<NYT::TNode>(NYT::TRichYPath(outputTable).SortedBy({ "shard_id", "doc_id" }))
            .Ordered(true),
        new TFilterMapper(maxShards, maxItems, columnPrefix, docIdShift)
    );

    return 0;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    return main_make_items(argc, argv);
}
