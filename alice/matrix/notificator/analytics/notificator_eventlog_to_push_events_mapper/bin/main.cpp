#include <alice/matrix/notificator/analytics/notificator_eventlog_to_push_events_mapper/library/mapper.h>
#include <alice/matrix/notificator/analytics/notificator_eventlog_to_push_events_mapper/library/table_helper.h>

#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/last_getopt.h>

int main(int argc, char *argv[]) {
    NYT::Initialize(argc, argv);

    NLastGetopt::TOpts opts;

    TString inputTable = "";
    TString outputTable = "";

    opts.AddLongOption('i', "input")
        .Help("yt table of notificator logs to be mapped")
        .StoreResult(&inputTable);

    opts.AddLongOption('o', "output")
        .Help("path to the output table")
        .DefaultValue("")
        .StoreResult(&outputTable);

    NLastGetopt::TOptsParseResult parsedOptions(&opts, argc, argv);

    if (!outputTable.Size()) {
        outputTable = inputTable + "_mapped";
    }

    Cout << "Mapping [" << inputTable << "] to [" << outputTable << "]" << Endl;

    auto outputPath = NYT::TRichYPath(outputTable).Schema(NMatrix::NNotificator::NAnalytics::GetMapperResultSchema());

    auto client = NYT::CreateClient("hahn");
    auto op = client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(inputTable)
            .AddOutput<NYT::TNode>(outputPath),
        MakeIntrusive<NMatrix::NNotificator::NAnalytics::TNotificatorLogToPushIdInfoMapper>()
    );

    Cout << "Operation: " << op->GetWebInterfaceUrl() << Endl;

    return 0;
}
