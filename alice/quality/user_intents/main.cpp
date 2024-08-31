#include <alice/quality/user_intents/intents.h>
#include <alice/quality/user_intents/proto/personal_intents.pb.h>

#include <mapreduce/yt/client/init.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/common.h>
#include <mapreduce/yt/util/ypath_join.h>
#include <quality/tools/top/top_reducer/top_reducer.h>
#include <quality/tools/util/yt_helpers.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <util/datetime/base.h>
#include <util/draft/date.h>
#include <util/draft/datetime.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>

namespace {

void AddLastUsedTimeToIntents(NYT::TNode& row) {
    NYT::TNode::TMapType intentsMapNode;
    for (const auto& [intent, value] : row["Intents"].AsMap()) {
        if (value.IsMap()) {
            intentsMapNode[intent] = value;
            if (intentsMapNode[intent]["Count"].IsUndefined()) {
                intentsMapNode[intent]["Count"] = NYT::TNode(1u);
            }
            if (intentsMapNode[intent]["LastUsedTime"].IsUndefined()) {
                intentsMapNode[intent]["LastUsedTime"] = NYT::TNode(0u);
            }
        } else if (value.IsUint64()) {
            intentsMapNode[intent] = NYT::TNode{}("Count", value.AsUint64())("LastUsedTime", 0u);
        }
    }
    row["Intents"] = intentsMapNode;
}

template <typename TReader>
void MergeIntentsWithNextRows(NYT::TNode::TMapType& intents, TReader* input) {
    input->Next();
    for (; input->IsValid(); input->Next()) {
        NYT::TNode row = input->MoveRow();
        AddLastUsedTimeToIntents(row);
        for (const auto& [intent, value] : row["Intents"].AsMap()) {
            if (intents.contains(intent)) {
                intents[intent]["Count"] = intents[intent]["Count"].AsUint64() + value["Count"].AsUint64();
                intents[intent]["LastUsedTime"] =
                    std::max(intents[intent]["LastUsedTime"].AsUint64(), value["LastUsedTime"].AsUint64());
            } else {
                intents[intent]["Count"] = value["Count"];
                intents[intent]["LastUsedTime"] = value["LastUsedTime"];
            }
        }
    }
}

class TExtractIntentsMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    void Do(TReader* input, TWriter* output) override {
        for (; input->IsValid(); input->Next()) {
            const NYT::TNode row = input->MoveRow();
            if (!row.HasKey("uuid") || !row["uuid"].IsString()
                || !row.HasKey("form_name") || !row["form_name"].IsString()
                || !row.HasKey("client_time") || !row["client_time"].IsUint64()) {
                continue;
            }

            TStringBuf uuid = row["uuid"].AsString();
            const auto& formName = row["form_name"].AsString();
            const auto& analyticsInfo = row["analytics_info"];

            if (uuid.empty() || formName.empty()) {
                continue;
            }

            Y_ENSURE(uuid.SkipPrefix("uu/"));

            const auto intentName = NAlice::GetIntentName(analyticsInfo, formName);
            const ui64 timestamp = row["client_time"].AsUint64();

            NYT::TNode intentNode = NYT::TNode{}("Count", 1u)("LastUsedTime", timestamp);
            output->AddRow(
                NYT::TNode{}
                    ("Uuid", uuid)
                    ("Intents", NYT::TNode{}(intentName, std::move(intentNode)))
            );
        }
    }
};
REGISTER_MAPPER(TExtractIntentsMapper);

class TExtractConversionsMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    void Do(TReader* input, TWriter* output) override {
        for (; input->IsValid(); input->Next()) {
            const NYT::TNode row = input->MoveRow();
            if (!row.HasKey("uuid") || !row["uuid"].IsString()
                || !row.HasKey("condition_ids") || !row["condition_ids"].IsList()
                || !row.HasKey("server_time_ms") || !row["server_time_ms"].IsInt64()) {
                continue;
            }

            TStringBuf uuid = row["uuid"].AsString();
            if (uuid.empty()) {
                continue;
            }

            const ui64 timestamp = row["server_time_ms"].AsInt64() / 1000;
            NYT::TNode conditionNode = NYT::TNode{}("Count", 1u)("LastUsedTime", timestamp);
            for (const auto& conditionId : row["condition_ids"].AsList()) {
                output->AddRow(
                    NYT::TNode{}
                        ("Uuid", uuid)
                        ("Intents", NYT::TNode{}(conditionId.AsString(), conditionNode))
                );
            }
        }
    }
};
REGISTER_MAPPER(TExtractConversionsMapper);

class TAggregateCombiner : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    void Do(TReader* input, TWriter* output) override {
        if (!input->IsValid()) {
            return;
        }
        NYT::TNode row = input->MoveRow();
        AddLastUsedTimeToIntents(row);
        auto& intents = row["Intents"].AsMap();

        MergeIntentsWithNextRows(intents, input);

        output->AddRow(row);
    }
};
REGISTER_REDUCER(TAggregateCombiner);

class TMergeTablesReducer
    : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NAlice::TPersonalIntentsRecord>> {
public:
    void Do(TReader* input, TWriter* output) override {
        if (!input->IsValid()) {
            return;
        }
        NYT::TNode row = input->MoveRow();
        AddLastUsedTimeToIntents(row);

        const auto& uuid = row["Uuid"].AsString();
        auto& intents = row["Intents"].AsMap();
        MergeIntentsWithNextRows(intents, input);

        NAlice::TPersonalIntentsRecord outRow;
        outRow.SetUrl(uuid);
        auto* personalIntents = outRow.MutablePersonalIntents();

        for (const auto& [intent, value] : intents) {
            auto* intentCounts = personalIntents->AddIntents();
            intentCounts->SetIntent(intent);
            intentCounts->SetCount(value["Count"].AsUint64());
            intentCounts->SetLastUsedTime(value["LastUsedTime"].AsUint64());
        }

        output->AddRow(outRow);
    }
};
REGISTER_REDUCER(TMergeTablesReducer);


class TMergeIntetnsWithConversionsReducer
    : public NYT::IReducer<NYT::TTableReader<NAlice::TPersonalIntentsRecord>, NYT::TTableWriter<NAlice::TPersonalIntentsRecord>> {
public:
    void Do(TReader* input, TWriter* output) override {
        if (!input->IsValid()) {
            return;
        }
        NAlice::TPersonalIntentsRecord outRow;
        for (auto& cursor : *input) {
            const auto& row = cursor.GetRow();
            const auto tableIndex = cursor.GetTableIndex();
            outRow.SetUrl(row.GetUrl());
            if (tableIndex == 0) { // intents
                for (const auto& intentCount : row.GetPersonalIntents().GetIntents()) {
                    outRow.MutablePersonalIntents()->AddIntents()->CopyFrom(intentCount);
                }
            } else if (tableIndex == 1) { // conversions
                for (const auto& conversionCount : row.GetPersonalIntents().GetIntents()) {
                    outRow.MutablePersonalIntents()->AddPostrollConversions()->CopyFrom(conversionCount);
                }
            } else {
                Y_FAIL();
            }
        }
        output->AddRow(outRow);
    }
};
REGISTER_REDUCER(TMergeIntetnsWithConversionsReducer);


void SelectInputTables(NYT::IClientBasePtr client, const TString& inputTablePrefix,
                       const TString& dailyResultsTablePrefix, const TVector<TDate>& dates,
                       TVector<TString>& inputTables, TVector<TString>& outputTables, TVector<TString>& resultTables) {

    size_t triedDays = 0;
    for (const auto& currentDay : dates) {
        const auto tableName = currentDay.ToStroka("%Y-%m-%d");

        const auto logsTable = inputTablePrefix + "/" + tableName;
        const auto outputTable = dailyResultsTablePrefix + "/" + tableName;
        bool addTableToResults = true;

        if (!client->Exists(logsTable)) {
            Cerr << "Skipping missing table: " << logsTable << Endl;
            addTableToResults = false;
        } else if (client->Exists(outputTable)) {
            Cerr << "Output table " << outputTable << " already exists; will not re-compute it" << Endl;
        } else {
            inputTables.push_back(logsTable);
            outputTables.push_back(outputTable);
        }

        if (addTableToResults) {
            resultTables.push_back(outputTable);
        }
        ++triedDays;
    }
}

TVector<TString> PrepareDailyTables(NYT::IClientBasePtr client, const TString& inputTablePrefix, const TString& dailyResultsTablePrefix,
                                    TIntrusivePtr<NYT::IMapperBase> mapper, const NYT::TSortColumns logColumns,
                                    const TVector<TDate>& dates, ui32 possibleMissingTables) {
    TVector<TString> inputTables;
    TVector<TString> outputTables;
    TVector<TString> resultTables;
    resultTables.reserve(dates.size());

    SelectInputTables(client, inputTablePrefix, dailyResultsTablePrefix, dates, inputTables, outputTables, resultTables);
    Y_ENSURE(resultTables.size() + possibleMissingTables >= dates.size(), "Too many missing tables, bailing out");

    for (size_t i : xrange(inputTables.size())) {
        const auto& inputTable = inputTables[i];
        const auto& outputTable = outputTables[i];

        const auto transaction = client->StartTransaction();
        try {
            transaction->MapReduce(
                NYT::TMapReduceOperationSpec{}
                    .AddInput<NYT::TNode>(NYT::TRichYPath{inputTable}.Columns(logColumns))
                    .AddOutput<NYT::TNode>(outputTable)
                    .ReduceBy({"Uuid"}),
                mapper, new TAggregateCombiner, new TAggregateCombiner,
                NYT::TOperationOptions{}.MountSandboxInTmpfs(true));

            transaction->Sort(NYT::TSortOperationSpec().AddInput(outputTable).Output(outputTable).SortBy({"Uuid"}));

            transaction->Commit();
        } catch (...) {
            transaction->Abort();
            throw;
        }
    }

    return resultTables;
}

void CombineTables(NYT::IClientBasePtr client, const TVector<TString>& inputTables, const TString& outputTable) {
    const auto transaction = client->StartTransaction();

    try {
        NYT::TReduceOperationSpec spec;

        for (const auto& inputTable : inputTables) {
            spec.AddInput<NYT::TNode>(NYT::TRichYPath{inputTable}.Columns({"Uuid", "Intents"}));
        }

        spec.AddOutput<NAlice::TPersonalIntentsRecord>(
            NQualityTools::GetRichTablePath(outputTable, NYT::CreateTableSchema<NAlice::TPersonalIntentsRecord>()));
        spec.ReduceBy({"Uuid"});

        transaction->Reduce(spec, new TMergeTablesReducer, NYT::TOperationOptions{}.MountSandboxInTmpfs(true));
        transaction->Sort(NYT::TSortOperationSpec().AddInput(outputTable).Output(outputTable).SortBy({"Url"}));
        transaction->Commit();
    } catch (...) {
        transaction->Abort();
        Cerr << CurrentExceptionMessage() << Endl;
        throw;
    }
}

void MergeIntentsWithConversionsTables(NYT::IClientBasePtr client,
                                       const TString& intentsTable, const TString& conversionsTable, const TString& outputTable) {
    const auto transaction = client->StartTransaction();

    try {
        NYT::TReduceOperationSpec spec;
        spec.AddInput<NAlice::TPersonalIntentsRecord>(NYT::TRichYPath{intentsTable}.Columns({"Url", "PersonalIntents"}));
        spec.AddInput<NAlice::TPersonalIntentsRecord>(NYT::TRichYPath{conversionsTable}.Columns({"Url", "PersonalIntents"}));

        spec.AddOutput<NAlice::TPersonalIntentsRecord>(
            NQualityTools::GetRichTablePath(outputTable, NYT::CreateTableSchema<NAlice::TPersonalIntentsRecord>()));
        spec.ReduceBy({"Url"});

        transaction->Reduce(spec, new TMergeIntetnsWithConversionsReducer, NYT::TOperationOptions{}.MountSandboxInTmpfs(true));
        transaction->Commit();
    } catch (...) {
        transaction->Abort();
        Cerr << CurrentExceptionMessage() << Endl;
        throw;
    }
}

void PrepareDates(TVector<TDate>& dates, ui32 dayCount) {
    dates.resize(dayCount);
    TDate startDate = ::TDate::Today() - dayCount;
    std::iota(dates.begin(), dates.end(), startDate);
}

} // namespace

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    NLastGetopt::TOpts opts;
    opts.AddHelpOption('h');

    TString proxyName;
    opts.AddLongOption("proxy", "YT proxy name")
        .DefaultValue("hahn")
        .RequiredArgument("PROXY")
        .StoreResult(&proxyName);

    NYT::TYPath inputTablePrefix;
    opts.AddLongOption("input-dir", "The YT directory with logs tables")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&inputTablePrefix);

    NYT::TYPath conversionsInputTablePrefix;
    opts.AddLongOption("conversions-input-dir", "The YT directory with applied conditions tables")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&conversionsInputTablePrefix);

    NYT::TYPath intentsDailyTablePrefix;
    opts.AddLongOption("work-dir", "The YT directory with processed logs tables")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&intentsDailyTablePrefix);

    NYT::TYPath conversionsDailyTablePrefix;
    opts.AddLongOption("conversions-work-dir", "The YT directory with processed applied conditions tables")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&conversionsDailyTablePrefix);

    TVector<TDate> dates;
    opts.AddLongOption("dates", "date strings (YYYYMMDD-YYYYMMDD,YYYYMMDD,YYYYMMDD...)")
        .RequiredArgument("DATES")
        .RangeSplitHandler(&dates, ',', '-');

    ui32 dayCount = 0;
    opts.AddLongOption("days-count", "The number of days to prepare user intents for")
        .DefaultValue(30)
        .RequiredArgument("NUM")
        .StoreResult(&dayCount);

    ui32 possibleMissingTables = 0;
    opts.AddLongOption("possible-missing-tables", "The number of input tables that can miss")
        .DefaultValue(0)
        .RequiredArgument("NUM")
        .StoreResult(&possibleMissingTables);

    NYT::TYPath intentsOutputTable;
    opts.AddLongOption("intents-output", "Intents output table")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&intentsOutputTable);

    NYT::TYPath conversionsOutputTable;
    opts.AddLongOption("conversions-output", "Conversions output table")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&conversionsOutputTable);

    NYT::TYPath outputTable;
    opts.AddLongOption("output", "Output table")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&outputTable);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult parseOpts{&opts, argc, argv};

    if (dates.empty()) {
        PrepareDates(dates, dayCount);
    }

    const auto client = NYT::CreateClient(proxyName);
    Cerr << "Prepare intents daily tables" << Endl;
    const auto intentsDailyTables = PrepareDailyTables(client, inputTablePrefix, intentsDailyTablePrefix,
        new TExtractIntentsMapper, NYT::TSortColumns("form_name", "uuid", "analytics_info", "client_time"), dates, possibleMissingTables);

    Cerr << "Prepare conversions daily tables" << Endl;
    const auto conversionsDailyTables = PrepareDailyTables(client, conversionsInputTablePrefix, conversionsDailyTablePrefix,
        new TExtractConversionsMapper, NYT::TSortColumns("uuid", "server_time_ms", "condition_ids"), dates, possibleMissingTables);

    Cerr << "Aggregate daily tables" << Endl;
    CombineTables(client, intentsDailyTables, intentsOutputTable);
    CombineTables(client, conversionsDailyTables, conversionsOutputTable);

    Cerr << "Merge intents and conversions" << Endl;
    MergeIntentsWithConversionsTables(client, intentsOutputTable, conversionsOutputTable, outputTable);
    return 0;
}
