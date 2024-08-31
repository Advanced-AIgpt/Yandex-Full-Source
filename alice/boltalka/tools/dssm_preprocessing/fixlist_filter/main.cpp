#include <alice/begemot/lib/fixlist_index/fixlist_index.h>

#include <mapreduce/interface/default_server.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>
#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/opt.h>

#include <util/folder/path.h>
#include <util/generic/maybe.h>

class TAliceFixlistFilterMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode> > {
public:
    TAliceFixlistFilterMapper() = default;

    TAliceFixlistFilterMapper(TVector<TString>& columns)
        : Columns(columns)
    {
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        const TFsPath filterPath("filter.yaml");
        TFileInput inputStream(filterPath);
        FixlistIndex.AddFixlist("filter", &inputStream);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for(; input->IsValid(); input->Next()) {
            auto row = input->MoveRow();
            TMaybe<TString> banColumn;
            for (const auto& column : Columns) {
                if (row[column].IsNull())
                    continue;

                NBg::TFixlistIndex::TQuery query {row[column].As<TString>(), "", {}};
                if (!FixlistIndex.MatchAgainst(query, "filter").empty()) {
                    banColumn = column;
                    break;
                }
            }

            if (!banColumn.Defined()) {
                output->AddRow(row, 0);
            } else {
                row["banColumn"] = banColumn.GetRef();
                output->AddRow(row, 1);
            }
        }
    }

    Y_SAVELOAD_JOB(Columns);

private:
    NBg::TFixlistIndex FixlistIndex;
    TVector<TString> Columns;
};

REGISTER_MAPPER(TAliceFixlistFilterMapper);

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    auto opts = NLastGetopt::TOpts::Default();

    TString inputTable;
    TString outputTable;
    TString errorTable;
    TVector<TString> columns;
    TString filterRulesPath;

    opts.AddLongOption('i', "inputTable")
        .Required()
        .RequiredArgument("INPUT")
        .StoreResult(&inputTable)
        .Help("input table");
    opts.AddLongOption('o', "outputTable")
        .Required()
        .RequiredArgument("OUTPUT")
        .StoreResult(&outputTable)
        .Help("output table");
    opts.AddLongOption('e', "errorTable")
        .Required()
        .RequiredArgument("ERROR")
        .StoreResult(&errorTable)
        .Help("error table");
    opts.AddLongOption("column")
        .Help("Columns to filter by")
        .Required()
        .RequiredArgument("COLUMN")
        .Handler1T<TString>([&](auto str) { columns.push_back(str); });
    opts.AddLongOption("filter-rules")
        .Help("File with rules to filter by")
        .Required()
        .RequiredArgument("FILE_PATH")
        .StoreResult(&filterRulesPath);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    auto client = NYT::CreateClient(GetDefaultYTServerOrFail());
    auto tx = client->StartTransaction();
    tx->Map(NYT::TMapOperationSpec()
                .AddInput<NYT::TNode>(inputTable)
                .AddOutput<NYT::TNode>(outputTable)
                .AddOutput<NYT::TNode>(errorTable)
                .MapperSpec(NYT::TUserJobSpec()
                    .AddLocalFile(filterRulesPath, NYT::TAddLocalFileOptions().PathInJob("filter.yaml"))),
            new TAliceFixlistFilterMapper(columns));
    tx->Commit();
}
