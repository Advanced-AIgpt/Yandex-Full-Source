#include <alice/matrix/notificator/analytics/push_events_reducer/library/mapreduce_by_push_id.h>

#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/last_getopt.h>

int main(int argc, char *argv[]) {
    NYT::Initialize(argc, argv);

    auto client = NYT::CreateClient("hahn");

    TString outputTable = "";

    NLastGetopt::TOpts opts;
    opts.AddLongOption('o', "output")
        .Help("path to the output table")
        .StoreResult(&outputTable);
    
    opts.SetTrailingArgTitle("tables to be reduced");
    
    NLastGetopt::TOptsParseResult parsedOptions(&opts, argc, argv);
    
    TVector<TString> inputTables; 
    for (const auto& arg : parsedOptions.GetFreeArgs()) {
        inputTables.push_back(arg);
    }
    
    NMatrix::NNotificator::NAnalytics::ReducePushEventsById(
        client,
        inputTables,
        outputTable
    );

    return 0;
}
