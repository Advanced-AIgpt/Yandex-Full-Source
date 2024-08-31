#include "constants.h"

#include <mapreduce/yt/client/init.h>
#include <mapreduce/yt/client/init.h>
#include <mapreduce/yt/interface/client.h>

#include <util/string/builder.h>
#include <util/string/printf.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/scheme/scheme.h>

enum ESourceTable {
    ST_Skills = 0,
    ST_ManualPhrases = 1,
    ST_Stats = 2,
    ST_HelpMessages = 3,
    ST_TolokaResultPositive = 4,
    ST_TolokaResultNegative = 5,
    ST_FirstMessages = 6,
    ST_LogsQueries = 7,
    ST_LogsReplies = 8,
    ST_ClicksQueries = 9,
    ST_PreClicksQueries = 10,
    ST_PreActivationQueries = 11
};

struct TSkillsMapperReducerConfig {
    ui32 SaasKps;
    ui32 MaxCountFirstMessages;
    ui32 MaxCountHelpMessages;
    ui32 MaxCountTolokaPositive;
    ui32 MaxCountTolokaNegative;
    ui32 MaxCountLogsQueries;
    ui32 MaxCountLogsReplies;
    ui32 MaxCountClicksQueries;
    ui32 MaxCountPreClicksQueries;
    ui32 MaxCountPreActivationQueries;

    Y_SAVELOAD_DEFINE(SaasKps, MaxCountFirstMessages, MaxCountHelpMessages, MaxCountTolokaPositive, MaxCountTolokaNegative, MaxCountLogsQueries, MaxCountLogsReplies, MaxCountClicksQueries, MaxCountPreClicksQueries, MaxCountPreActivationQueries);
};

struct TCmdArgs {
    TString SkillsInputTable;
    TString ManualPhrasesInputTable;
    TString StatsInputTable;
    TString HelpMessagesInputTable;
    TString TolokaPositiveResultInputTable;
    TString TolokaNegativeResultInputTable;
    TString FirstMessagesInputTable;
    TString LogsQueriesInputTable;
    TString LogsRepliesInputTable;
    TString ClicksInputTable;
    TString PreClicksInputTable;
    TString PreActivationInputTable;

    TString YtCluster;
    TString OutputTable;
    TString Channel;
    TSkillsMapperReducerConfig ReducerConfig;
};

class TSkillsMapper: public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TSkillsMapper() = default;

    TSkillsMapper(TStringBuf channel, THashMap<ui32, ui32>& tablesMapping)
        : Channel(channel)
        , TablesMapping(tablesMapping)
    {}

    void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            const auto sourceTable = static_cast<ESourceTable>(TablesMapping.at(reader->GetTableIndex()));

            switch (sourceTable) {
                case ST_Skills: {
                    if (row["channel"].AsString() != Channel)
                        continue;
                    if (row["isBanned"].AsBool())
                        continue;
                    if (row["hideInStore"].AsBool())
                        continue;
                    if (!row["deletedAt"].IsNull())
                        continue;
                    if (!row["onAir"].AsBool())
                        continue;
                    if (!row["isRecommended"].AsBool())
                        continue;

                    break;
                } case ST_TolokaResultPositive: {
                    if (!row["answer"].IsNull() && row["answer"].AsString() == "YES")
                        break;
                    if (!row["golden"].IsNull() && row["golden"].AsString() == "YES")
                        break;
                    continue;
                } case ST_TolokaResultNegative: {
                    if (!row["answer"].IsNull() && row["answer"].AsString() == "NO")
                        break;
                    if (!row["golden"].IsNull() && row["golden"].AsString() == "NO")
                        break;
                    if (!row["answer"].IsNull() && row["answer"].AsString() == "NONSENSE")
                        break;
                    if (!row["golden"].IsNull() && row["golden"].AsString() == "NONSENSE")
                        break;

                    continue;
                }
                case ST_LogsReplies: {
                    if (row["reply"].IsNull())
                        continue;
                    break;
                }
                case ST_LogsQueries:
                case ST_ClicksQueries:
                case ST_PreClicksQueries:
                case ST_PreActivationQueries: {
                    if (row["query"].IsNull())
                        continue;
                    break;
                }
                case ST_Stats:
                case ST_HelpMessages:
                case ST_FirstMessages:
                case ST_ManualPhrases:
                    break;

            }

            TString skill_id;
            switch (sourceTable) {
                case ST_TolokaResultPositive:
                case ST_TolokaResultNegative:
                case ST_HelpMessages:
                case ST_FirstMessages:
                case ST_LogsQueries:
                case ST_LogsReplies:
                case ST_ClicksQueries:
                case ST_PreClicksQueries:
                case ST_PreActivationQueries:
                    skill_id = row["skill_id"].AsString();
                    break;
                case ST_Skills:
                case ST_Stats:
                case ST_ManualPhrases:
                    skill_id = row["id"].AsString();
                    break;
            }

            NYT::TNode result(row);
            result["key"] = skill_id;
            result["source_table"] = sourceTable;
            writer->AddRow(result);
        }
    }

    Y_SAVELOAD_JOB(Channel, TablesMapping);

private:
    TString Channel;
    THashMap<ui32, ui32> TablesMapping;
};

REGISTER_MAPPER(TSkillsMapper);

class TSkillsReducer: public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TSkillsReducer() = default;

    TSkillsReducer(TSkillsMapperReducerConfig config)
        : Config_(config)
    {}

    void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) override {
        NSc::TValue doc;
        ui32 firstMessagesCount = 0;
        ui32 helpMessagesCount = 0;
        ui32 tolokaPositiveCount = 0;
        ui32 tolokaNegativeCount = 0;
        ui32 logsQueriesCount = 0;
        ui32 logsRepliesCount = 0;
        ui32 clicksQueriesCount = 0;
        ui32 preClicksQueriesCount = 0;
        ui32 preActivationQueriesCount = 0;
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            const auto sourceTable = static_cast<ESourceTable>(row["source_table"].AsInt64());
            switch (sourceTable) {
                case ST_Skills: {
                    doc[URL] = row["key"].AsString();

                    doc[Z_TITLE]["value"] = row["name"].IsNull() ? "" : row["name"].AsString();
                    doc[Z_TITLE]["type"] = "#z";

                    doc[Z_BODY]["value"] = row["description"].IsNull() ? "" : row["description"].AsString();
                    doc[Z_BODY]["type"] = "#z";

                    doc[Z_CATEGORY]["value"] = row["categoryLabel"].IsNull() ? "" : row["categoryLabel"].AsString();
                    doc[Z_CATEGORY]["type"] = "#z";

                    doc[Z_DEVELOPER]["value"] = row["developerName"].IsNull() ? "" : row["developerName"].AsString();
                    doc[Z_DEVELOPER]["type"] = "#z";

                    for (const auto& src : row["activationPhrases"].AsList()) {
                        auto v = doc[Z_ACTIVATIONS].Push();
                        v["value"] = src.AsString();
                        v["type"] = "#z";
                    }
                    for (const auto& prefix : ACTIVATION_PREFIXES) {
                        auto v = doc[Z_ACTIVATION_PREFIXES].Push();
                        v["value"] = prefix;
                        v["type"] = "#z";
                    }
                    if (!row["category"].IsNull() && IsIn(GAMES_CATEGORIES, row["category"].AsString())) {
                        for (const auto& prefix : ACTIVATION_PREFIXES_GAMES) {
                            auto v = doc[Z_ACTIVATION_PREFIXES].Push();
                            v["value"] = prefix;
                            v["type"] = "#z";
                        }
                    }

                    doc[F_IS_RECOMMENDED]["value"] = 0;
                    doc[F_IS_RECOMMENDED]["type"] = "#f";
                    doc[I_IS_RECOMMENDED]["value"] = 0;
                    doc[I_IS_RECOMMENDED]["type"] = "#pig";

                    break;
                } case ST_ManualPhrases: {
                    for (const auto& phrase : row["phrases"].AsList()) {
                        auto v = doc[Z_MANUAL_PHRASES].Push();
                        v["value"] = phrase.AsString();
                        v["type"] = "#z";
                    }
                    break;
                } case ST_Stats: {
                    if (!row["week_new_users"].IsNull()) {
                        doc[F_WEEK_NEW_USERS]["value"] = log(row["week_new_users"].AsInt64() + 1);
                        doc[F_WEEK_NEW_USERS]["type"] = "#f";
                        doc[I_WEEK_NEW_USERS]["value"] = row["week_new_users"].AsInt64();
                        doc[I_WEEK_NEW_USERS]["type"] = "#pig";
                    }

                    if (!row["mau"].IsNull()) {
                        doc[F_MAU]["value"] = log(row["mau"].AsInt64() + 1);
                        doc[F_MAU]["type"] = "#f";
                        doc[I_MAU]["value"] = row["mau"].AsInt64();
                        doc[I_MAU]["type"] = "#pig";
                    }

                    if (!row["wau"].IsNull()) {
                        doc[F_WAU]["value"] = log(row["wau"].AsInt64() + 1);
                        doc[F_WAU]["type"] = "#f";
                        doc[I_WAU]["value"] = row["wau"].AsInt64();
                        doc[I_WAU]["type"] = "#pig";
                    }

                    if (!row["returned"].IsNull()) {
                        doc[F_RETURNED]["value"] = log(row["returned"].AsInt64() + 1);
                        doc[F_RETURNED]["type"] = "#f";
                        doc[I_RETURNED]["value"] = row["returned"].AsInt64();
                        doc[I_RETURNED]["type"] = "#pig";
                    }

                    if (!row["rating_avg"].IsNull()) {
                        doc[F_RATING_AVG]["value"] = row["rating_avg"].AsDouble();
                        doc[F_RATING_AVG]["type"] = "#f";
                        doc[P_RATING_AVG]["value"] = row["rating_avg"].AsDouble();
                        doc[P_RATING_AVG]["type"] = "#p";
                    }

                    if (!row["retention"].IsNull()) {
                        doc[F_RETENTION]["value"] = row["retention"].AsDouble();
                        doc[F_RETENTION]["type"] = "#f";
                        doc[P_RETENTION]["value"] = row["retention"].AsDouble();
                        doc[P_RETENTION]["type"] = "#p";
                    }

                    if (!row["rating_count"].IsNull()) {
                        doc[F_RATING_COUNT]["value"] = log(row["rating_count"].AsDouble() + 1);
                        doc[F_RATING_COUNT]["type"] = "#f";
                        doc[P_RATING_COUNT]["value"] = row["rating_count"].AsDouble();
                        doc[P_RATING_COUNT]["type"] = "#p";
                    }

                    if (!row["s1"].IsNull()) {
                        doc[F_RATING_S1]["value"] = log(row["s1"].AsInt64() + 1);
                        doc[F_RATING_S1]["type"] = "#f";
                        doc[I_RATING_S1]["value"] = row["s1"].AsInt64();
                        doc[I_RATING_S1]["type"] = "#pig";
                    }

                    if (!row["s2"].IsNull()) {
                        doc[F_RATING_S2]["value"] = log(row["s2"].AsInt64() + 1);
                        doc[F_RATING_S2]["type"] = "#f";
                        doc[I_RATING_S2]["value"] = row["s2"].AsInt64();
                        doc[I_RATING_S2]["type"] = "#pig";
                    }

                    if (!row["s3"].IsNull()) {
                        doc[F_RATING_S3]["value"] = log(row["s3"].AsInt64() + 1);
                        doc[F_RATING_S3]["type"] = "#f";
                        doc[I_RATING_S3]["value"] = row["s3"].AsInt64();
                        doc[I_RATING_S3]["type"] = "#pig";
                    }

                    if (!row["s4"].IsNull()) {
                        doc[F_RATING_S4]["value"] = log(row["s4"].AsInt64() + 1);
                        doc[F_RATING_S4]["type"] = "#f";
                        doc[I_RATING_S4]["value"] = row["s4"].AsInt64();
                        doc[I_RATING_S4]["type"] = "#pig";
                    }

                    if (!row["s5"].IsNull()) {
                        doc[F_RATING_S5]["value"] = log(row["s5"].AsInt64() + 1);
                        doc[F_RATING_S5]["type"] = "#f";
                        doc[I_RATING_S5]["value"] = row["s5"].AsInt64();
                        doc[I_RATING_S5]["type"] = "#pig";
                    }

                    //"keywords" ?
                    break;
                } case ST_TolokaResultPositive: {
                    if (tolokaPositiveCount >= Config_.MaxCountTolokaPositive)
                        break;
                    auto v = doc[Z_TOLOKA_POSITIVE].Push();
                    v["value"] = row["query"].AsString();
                    v["type"] = "#z";
                    tolokaPositiveCount += 1;

                    break;
                } case ST_TolokaResultNegative: {
                    if (tolokaNegativeCount >= Config_.MaxCountTolokaNegative)
                        break;
                    auto v = doc[Z_TOLOKA_NEGATIVE].Push();
                    v["value"] = row["query"].AsString();
                    v["type"] = "#z";
                    tolokaNegativeCount += 1;

                    break;
                } case ST_HelpMessages: {
                    if (helpMessagesCount >= Config_.MaxCountHelpMessages)
                        break;
                    auto v = doc[Z_HELP_MESSAGES].Push();
                    v["value"] = row["reply"].AsString();
                    v["type"] = "#z";
                    helpMessagesCount += 1;

                    break;
                } case ST_FirstMessages: {
                    if (firstMessagesCount >= Config_.MaxCountFirstMessages)
                        break;
                    auto v = doc[Z_FIRST_MESSAGES].Push();
                    v["value"] = row["reply"].AsString();
                    v["type"] = "#z";
                    firstMessagesCount += 1;

                    break;
                } case ST_LogsQueries: {
                    if (logsQueriesCount >= Config_.MaxCountLogsQueries)
                        break;
                    auto v = doc[Z_LOGS_QUERIES].Push();
                    v["value"] = row["query"].AsString();
                    v["type"] = "#z";
                    logsQueriesCount += 1;

                    break;
                } case ST_LogsReplies: {
                    if (logsRepliesCount >= Config_.MaxCountLogsReplies)
                        break;
                    auto v = doc[Z_LOGS_REPLIES].Push();
                    v["value"] = row["reply"].AsString();
                    v["type"] = "#z";
                    logsRepliesCount += 1;

                    break;
                } case ST_ClicksQueries: {
                    if (clicksQueriesCount >= Config_.MaxCountClicksQueries)
                        break;
                    auto v = doc[Z_CLICKS_QUERIES].Push();
                    v["value"] = row["query"].AsString();
                    v["type"] = "#z";
                    clicksQueriesCount += 1;

                    break;
                } case ST_PreClicksQueries: {
                    if (preClicksQueriesCount >= Config_.MaxCountPreClicksQueries)
                        break;
                    auto v = doc[Z_PRE_CLICKS_QUERIES].Push();
                    v["value"] = row["query"].AsString();
                    v["type"] = "#z";
                    preClicksQueriesCount += 1;

                    break;
                } case ST_PreActivationQueries: {
                    if (preActivationQueriesCount >= Config_.MaxCountPreActivationQueries)
                        break;
                    auto v = doc[Z_PRE_ACTIVATION_QUERIES].Push();
                    v["value"] = row["query"].AsString();
                    v["type"] = "#z";
                    preActivationQueriesCount += 1;

                    break;
                }


            }
        }

        if (doc[URL].IsNull())
            return;

        NSc::TValue options;
        options["mime_type"] = "plain/text";
        options["charset"] = "utf8";
        options["language"] = "ru";
        options["language2"] = "ru";
        options["language_default"] = "ru";
        options["language_default2"] = "ru";
        options["modification_timestamp"] = ToString(TInstant::Now().Seconds());
        doc["options"] = options;

        NSc::TValue value;
        value["action"] = "modify";
        value["prefix"] = Config_.SaasKps;
        value["docs"].Push(doc);

        NYT::TNode result;
        result["JsonMessage"] = value.ToJson();
        writer->AddRow(result);
    }

    Y_SAVELOAD_JOB(Config_);

private:
    TSkillsMapperReducerConfig Config_;
};

REGISTER_REDUCER(TSkillsReducer);

void AddInputTable(const TString& table, ESourceTable source, NYT::TMapReduceOperationSpec& spec, NYT::ITransactionPtr& transaction, THashMap<ui32, ui32>& tablesMapping, bool mandatory) {
    if (!mandatory && !table) {
        return;
    }
    if (!table)
        ythrow yexception() << "source '" << static_cast<size_t>(source) << "' is mandatory";
    if (!transaction->Exists(table)) {
        ythrow yexception() << "table '" << table << "' does not exist";
    }

    Cerr << "Added input: " << table << " as " << tablesMapping.size() << " with source " << static_cast<size_t>(source) << Endl;
    spec.SetInput<NYT::TNode>(tablesMapping.size(), table);
    tablesMapping.insert({tablesMapping.size(), source});
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    TCmdArgs args;
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(0);

    opts.AddLongOption("cluster")
        .Help("YT cluster name")
        .Optional()
        .DefaultValue("hahn")
        .StoreResult(&args.YtCluster);
    opts.AddLongOption("kps")
        .Help("Saas kps (stable=82, prestable=2018, testing=2465)")
        .Optional()
        .DefaultValue(2465)
        .StoreResult(&args.ReducerConfig.SaasKps);
    opts.AddLongOption("channel")
        .Help("Paskills channel")
        .Optional()
        .DefaultValue("aliceSkill")
        .StoreResult(&args.Channel);
    opts.AddLongOption("skills-table")
        .Help("YT table")
        .Optional()
        .DefaultValue("//home/paskills/skills/stable")
        .StoreResult(&args.SkillsInputTable);
    opts.AddLongOption("manual-phrases-table")
        .Help("YT table (//home/misspell/deemonasd/alisa/discovery/phrases)")
        .Optional()
        .StoreResult(&args.ManualPhrasesInputTable);
    opts.AddLongOption("stats-table")
        .Help("YT table (//home/paskills/stat/skills_sum)")
        .Optional()
        .StoreResult(&args.StatsInputTable);
    opts.AddLongOption("toloka-positive-results-table")
        .Help("YT table (//home/misspell/deemonasd/alisa/discovery/saas/markup/all_toloka_result)")
        .Optional()
        .StoreResult(&args.TolokaPositiveResultInputTable);
    opts.AddLongOption("toloka-negative-results-table")
        .Help("YT table (//home/misspell/deemonasd/alisa/discovery/saas/markup/all_toloka_result)")
        .Optional()
        .StoreResult(&args.TolokaNegativeResultInputTable);
    opts.AddLongOption("first-messages-table")
        .Help("YT table (//home/voice/nikitachizhov/skills_greeting)")
        .Optional()
        .StoreResult(&args.FirstMessagesInputTable);
    opts.AddLongOption("help-messages-table")
        .Help("YT table (//home/voice/nikitachizhov/skills_help_answers)")
        .Optional()
        .StoreResult(&args.HelpMessagesInputTable);
    opts.AddLongOption("logs-queries-table")
        .Help("YT table")
        .Optional()
        .StoreResult(&args.LogsQueriesInputTable);
    opts.AddLongOption("logs-replies-table")
        .Help("YT table")
        .Optional()
        .StoreResult(&args.LogsRepliesInputTable);
    opts.AddLongOption("clicks-queries-table")
        .Help("YT table")
        .Optional()
        .StoreResult(&args.ClicksInputTable);
    opts.AddLongOption("pre-clicks-queries-table")
        .Help("YT table")
        .Optional()
        .StoreResult(&args.PreClicksInputTable);
    opts.AddLongOption("pre-activation-queries-table")
        .Help("YT table")
        .Optional()
        .StoreResult(&args.PreActivationInputTable);

    opts.AddLongOption("max-count-first-messages")
        .Help("Max count first messages from first messages table")
        .Optional()
        .DefaultValue(100)
        .StoreResult(&args.ReducerConfig.MaxCountFirstMessages);
    opts.AddLongOption("max-count-help-messages")
        .Help("Max count help messages from help messages table")
        .Optional()
        .DefaultValue(100)
        .StoreResult(&args.ReducerConfig.MaxCountHelpMessages);
    opts.AddLongOption("max-count-toloka-positive")
        .Help("Max count help messages from toloka results table")
        .Optional()
        .DefaultValue(100)
        .StoreResult(&args.ReducerConfig.MaxCountTolokaPositive);
    opts.AddLongOption("max-count-toloka-negative")
        .Help("Max count help messages from toloka results table")
        .Optional()
        .DefaultValue(100)
        .StoreResult(&args.ReducerConfig.MaxCountTolokaNegative);
    opts.AddLongOption("max-count-logs-queries")
        .Help("Max count queries from logs table")
        .Optional()
        .DefaultValue(100)
        .StoreResult(&args.ReducerConfig.MaxCountLogsQueries);
    opts.AddLongOption("max-count-logs-replies")
        .Help("Max count replies from logs table")
        .Optional()
        .DefaultValue(100)
        .StoreResult(&args.ReducerConfig.MaxCountLogsReplies);
    opts.AddLongOption("max-count-clicks-queries")
        .Help("Max count queries from clicks table")
        .Optional()
        .DefaultValue(100)
        .StoreResult(&args.ReducerConfig.MaxCountClicksQueries);
    opts.AddLongOption("max-count-pre-clicks-queries")
        .Help("Max count queries from pre clicks table")
        .Optional()
        .DefaultValue(100)
        .StoreResult(&args.ReducerConfig.MaxCountPreClicksQueries);
    opts.AddLongOption("max-count-pre-activation-queries")
        .Help("Max count queries from pre activation table")
        .Optional()
        .DefaultValue(100)
        .StoreResult(&args.ReducerConfig.MaxCountPreActivationQueries);

    opts.AddLongOption("out")
        .Help("Output YT table")
        .Required()
        .StoreResult(&args.OutputTable);

    opts.SetFreeArgsNum(0);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    auto client = NYT::CreateClient(args.YtCluster);
    auto transaction = client->StartTransaction();

    THashMap<ui32, ui32> tablesMapping;
    NYT::TMapReduceOperationSpec spec;
    AddInputTable(args.SkillsInputTable, ST_Skills, spec, transaction, tablesMapping, true);
    AddInputTable(args.ManualPhrasesInputTable, ST_ManualPhrases, spec, transaction, tablesMapping, false);
    AddInputTable(args.StatsInputTable, ST_Stats, spec, transaction, tablesMapping, false);
    AddInputTable(args.TolokaPositiveResultInputTable, ST_TolokaResultPositive, spec, transaction, tablesMapping, false);
    AddInputTable(args.TolokaNegativeResultInputTable, ST_TolokaResultNegative, spec, transaction, tablesMapping, false);
    AddInputTable(args.HelpMessagesInputTable, ST_HelpMessages, spec, transaction, tablesMapping, false);
    AddInputTable(args.FirstMessagesInputTable, ST_FirstMessages, spec, transaction, tablesMapping, false);
    AddInputTable(args.LogsQueriesInputTable, ST_LogsQueries, spec, transaction, tablesMapping, false);
    AddInputTable(args.LogsRepliesInputTable, ST_LogsReplies, spec, transaction, tablesMapping, false);
    AddInputTable(args.ClicksInputTable, ST_ClicksQueries, spec, transaction, tablesMapping, false);
    AddInputTable(args.PreClicksInputTable, ST_PreClicksQueries, spec, transaction, tablesMapping, false);
    AddInputTable(args.PreActivationInputTable, ST_PreActivationQueries, spec, transaction, tablesMapping, false);

    spec.AddOutput<NYT::TNode>(args.OutputTable);
    spec.ReduceBy({"key"});

    transaction->MapReduce(
        spec,
        new TSkillsMapper(args.Channel, tablesMapping),
        new TSkillsReducer(args.ReducerConfig)
    );

    transaction->Commit();

    return 0;
}
