#include <alice/library/intent_stats/proto/intent_stats.pb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/langs/langs.h>

#include <mapreduce/yt/client/init.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/util/ypath_join.h>
#include <quality/tools/top/top_reducer/top_reducer.h>
#include <quality/tools/util/yt_helpers.h>
#include <util/draft/date.h>
#include <util/string/join.h>

#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

namespace {

TVector<TStringBuf> GenerateNgrams(TStringBuf normalizedQuery, const TVector<ui32>& ngrams) {
    TVector<TStringBuf> ngramStrings;
    for (const auto ngramSize : ngrams) {
        if (Count(normalizedQuery, ' ') < (ngramSize - 1)) {
            continue;
        }
        size_t begin = 0;
        size_t end = 0;
        for (size_t i = 0; i < ngramSize; ++i) {
            end = normalizedQuery.find(' ', end + 1);
        }

        while (end != TString::npos) {
            ngramStrings.push_back(normalizedQuery.substr(begin, end - begin));
            begin = normalizedQuery.find(' ', begin + 1) + 1;
            end = normalizedQuery.find(' ', end + 1);
        }
        ngramStrings.push_back(normalizedQuery.substr(begin, end));
    }

    return ngramStrings;
}

void MergeIntentMaps(const THashMap<TString, NYT::TNode>& row, THashMap<TString, NYT::TNode>& stats) {
    for (const auto& [intent, value] : row) {
        if (stats.contains(intent)) {
            stats[intent] = stats[intent].AsUint64() + value.AsUint64();
        } else {
            stats[intent] = value;
        }
    }
}

void CombineTokenStats(NYT::TTableReader<NYT::TNode>* input, THashMap<TString, NYT::TNode>& scenarios, THashMap<TString, NYT::TNode>& clients) {
    if (!input->IsValid()) {
        return;
    }

    input->Next();
    for (; input->IsValid(); input->Next()) {
        const NYT::TNode row = input->MoveRow();
        MergeIntentMaps(row["Scenarios"].AsMap(), scenarios);
        for (const auto& [client, value] : row["Clients"].AsMap()) {
            if (clients.contains(client)) {
                MergeIntentMaps(value.AsMap(), clients[client].AsMap());
            } else {
                clients[client] = value;
            }
        }
    }
}

bool FillIntentStatsProto(const THashMap<TString, NYT::TNode>& intents, ui64 minCount, NAlice::TIntentsStatRecord_TIntentsStat& proto) {
    ui64 count = 0;
    for (const auto& [intent, value] : intents) {
        auto* intentCount = proto.AddIntents();
        intentCount->SetIntent(intent);
        intentCount->SetCount(value.AsUint64());
        count += value.AsUint64();
    }
    proto.SetTotalCount(count);

    if (count < minCount) {
        proto.Clear();
        return false;
    }
    return true;
}

class TExtractMapper
    : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>>
{
public:
    TExtractMapper() = default;

    explicit TExtractMapper(const TVector<ui32>& ngrams)
        : Ngrams(ngrams)
    {
    }

    void Do(TReader* input, TWriter* output) override {
        for (; input->IsValid(); input->Next()) {
            const NYT::TNode row = input->MoveRow();

            const auto& app = row["app"];
            if (app.IsNull() || !app.IsString()) {
                continue;
            }
            const auto& appString = app.AsString();

            const auto& query = row["query"];
            if (query.IsNull() || !query.IsString()) {
                continue;
            }
            const auto lang = GetLanguage(row["lang"]);

            const auto& scenario = row["mm_scenario"];
            if (scenario.IsNull() || !scenario.IsString()) {
                continue;
            }

            const auto& queryString = query.AsString();
            const auto& scenarioString = scenario.AsString();

            if (queryString.Empty() || scenarioString.Empty()) {
                continue;
            }

            const auto normalizedQuery = NNlu::TRequestNormalizer::Normalize(lang, queryString);

            TVector<TStringBuf> ngramStrings = GenerateNgrams(normalizedQuery, Ngrams);

            for (const auto token : ngramStrings) {
                output->AddRow(NYT::TNode{}
                        ("Token", token)
                        ("Scenarios", NYT::TNode{}(scenarioString, 1ull))
                        ("Clients", NYT::TNode{}(appString, NYT::TNode{}(scenarioString, 1ull)))
                    );
            }
            
        }
    }

    Y_SAVELOAD_JOB(Ngrams);

private:
    static ELanguage GetLanguage(const NYT::TNode& lang) {
        if (lang.IsNull() || !lang.IsString()) {
            return ELanguage::LANG_UNK;
        }
        return LanguageByName(lang.AsString());
    }

private:
    TVector<ui32> Ngrams;
};
REGISTER_MAPPER(TExtractMapper);

class TAggregateCombiner
    : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>>
{
public:
    void Do(TReader* input, TWriter* output) override {
        if (!input->IsValid()) {
            return;
        }
        NYT::TNode row = input->MoveRow();
        CombineTokenStats(input, row["Scenarios"].AsMap(), row["Clients"].AsMap());

        output->AddRow(row);
    }
};
REGISTER_REDUCER(TAggregateCombiner);

class TAggregateReducer
    : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NAlice::TIntentsStatRecord>>
{
public:
    explicit TAggregateReducer(const ui64 minTotalCount = 0) noexcept
        : MinTotalCount{minTotalCount}
    {
    }

    void Do(TReader* input, TWriter* output) override {
        if (!input->IsValid()) {
            return;
        }
        NYT::TNode row = input->MoveRow();

        const TString& token = row["Token"].AsString();
        auto scenarios = row["Scenarios"].AsMap();
        auto clients = row["Clients"].AsMap();
        CombineTokenStats(input, scenarios, clients);

        NAlice::TIntentsStatRecord record;
        record.SetUrl(token);
        auto* intentsStat = record.MutableIntentsStat();
        if (!FillIntentStatsProto(scenarios, MinTotalCount, *intentsStat)) {
            return;
        }

        if (auto* clientsStatProto = record.MutableClientsIntentsStat()->MutableClientIntentsStat()) {
            for (const auto& [client, clientStat] : clients) {
                NAlice::TIntentsStatRecord_TIntentsStat clientStatProto;
                if (FillIntentStatsProto(clientStat.AsMap(), MinTotalCount, clientStatProto)) {
                    (*clientsStatProto)[client] = std::move(clientStatProto);
                }
            }
        }

        output->AddRow(record);
    }

    Y_SAVELOAD_JOB(MinTotalCount);

private:
    ui64 MinTotalCount;
};
REGISTER_REDUCER(TAggregateReducer);

} // namespace

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    NLastGetopt::TOpts opts;
    opts.AddHelpOption('h');

    TString proxyName;
    opts.AddLongOption('p', "proxy", "YT proxy name")
        .Required()
        .RequiredArgument("PROXY")
        .StoreResult(&proxyName);

    NYT::TYPath inputTable;
    opts.AddLongOption('i', "input", "input logs table prefix")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&inputTable);

    TDate dateStart;
    opts.AddLongOption('s', "start", "start date")
        .Required()
        .RequiredArgument("YYYYMMDD")
        .StoreResult(&dateStart);

    TDate dateEnd;
    opts.AddLongOption('e', "end", "end date")
        .Required()
        .RequiredArgument("YYYYMMDD")
        .StoreResult(&dateEnd);

    NYT::TYPath outputTable;
    opts.AddLongOption('o', "output", "output statistics table")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&outputTable);

    TVector<ui32> ngrams;
    opts.AddLongOption('n', "ngrams", "ngrams 1,2,3...")
        .DefaultValue("1")
        .RequiredArgument("NGRAMS")
        .RangeSplitHandler(&ngrams, ',', '-');

    ui32 minTokenCount = 0;
    opts.AddLongOption('m', "min-token-count", "minimum number of token occurencies")
        .RequiredArgument("COUNT")
        .DefaultValue(minTokenCount)
        .StoreResult(&minTokenCount);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult parseOpts{&opts, argc, argv};

    const auto client = NYT::CreateClient(proxyName);
    const auto transaction = client->StartTransaction();

    try {
        NYT::TMapReduceOperationSpec spec;

        for (TDate date = dateStart; date <= dateEnd; ++date) {
            spec.AddInput<NYT::TNode>(
                NYT::TRichYPath{NYT::JoinYPaths(inputTable, date.ToStroka("%Y-%m-%d"))}
                    .Columns({"query", "app", "lang", "mm_scenario"}));
        }

        spec.AddOutput<NAlice::TIntentsStatRecord>(
            NQualityTools::GetRichTablePath(
                outputTable,
                NYT::CreateTableSchema<NAlice::TIntentsStatRecord>())
        );

        spec.ReduceBy({"Token"});

        transaction->MapReduce(
            spec,
            new TExtractMapper(ngrams),
            new TAggregateCombiner,
            new TAggregateReducer(minTokenCount),
            NYT::TOperationOptions{}.MountSandboxInTmpfs(true)
        );
        transaction->Commit();
    } catch (...) {
        transaction->Abort();
        Cerr << CurrentExceptionMessage() << Endl;
        PrintBackTrace();
        exit(EXIT_FAILURE);
    }
    return 0;
}
