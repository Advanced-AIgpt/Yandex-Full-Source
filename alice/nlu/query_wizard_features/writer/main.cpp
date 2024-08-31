#include <alice/nlu/query_wizard_features/proto/features.pb.h>

#include <mapreduce/yt/client/init.h>
#include <mapreduce/yt/interface/client.h>
#include <quality/trailer/suggest/data_structs/blob_reader.h>
#include <quality/trailer/suggest/data_structs/blob_writer.h>
#include <yweb/blender/lib/yql/yql.h>

#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <util/datetime/base.h>
#include <util/draft/datetime.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>
#include <util/string/builder.h>
#include <util/string/subst.h>

#include <functional>

static const TString BLENDER_SQUEEZE_DIR = "//home/blender/surplus_squeeze/production";

struct TYqlParams {
    const TString ApiUrl;
    const TString OAuthToken;
    const TString ClusterName;
    const TString YqlAuth;

    TYqlParams(const TString& apiUrl, const TString& oauthToken, const TString& clusterName, const TString& yqlAuth)
        : ApiUrl(apiUrl)
        , OAuthToken(oauthToken)
        , ClusterName(clusterName)
        , YqlAuth(yqlAuth)
    {}
};

TString PrepareScript(const TYqlParams& yqlParams, const TString& scriptBodyResource) {
    const auto mergeSurpluses = NResource::Find("/merge_surpluses.yql");
    const auto scriptBody = NResource::Find(scriptBodyResource);

    TString result = TStringBuilder()
        << "USE $CLUSTER_NAME;\n"
        << "PRAGMA yt.Auth = '$YQL_AUTH';\n"
        << mergeSurpluses << "\n"
        << scriptBody << "\n";

    SubstGlobal(result, "$CLUSTER_NAME", yqlParams.ClusterName);
    SubstGlobal(result, "$YQL_AUTH", yqlParams.YqlAuth);
    return result;
}

void RunYQLScriptsOrDie(const TVector<TString>& scripts, const TString& yqlApiUrl, const TString& oauthToken) {
    TVector<TYqlOp> ops;
    for (const auto& script : scripts) {
        ops.push_back(RunYQLScript(script, oauthToken, yqlApiUrl));
    }

    Cerr << "Running " << ops.size() << " operations" << Endl;

    while (true) {
        bool hasErrors = false;
        size_t runningOpCount = 0;
        for (auto& op : ops) {
            const auto res = op.CheckStatus();
            if (EqualToOneOf(res, YQL_OP_FAILED, YQL_OP_YQL_ERROR)) {
                Cerr << "Operation " << op.GetId() << " failed, reason: " << op.GetFailureReason() << Endl;
                Cerr << "Hint: use SHOW RESULTS " << op.GetId() << " in ya yql for more details" << Endl;
                hasErrors = true;
            }
            if (res != YQL_OP_COMPLETED) {
                ++runningOpCount;
            }
        }
        Y_ENSURE(!hasErrors, "Some operations have failed");
        if (runningOpCount == 0) {
            break;
        } else {
            Cerr << "Still waiting for " << runningOpCount << " operations" << Endl;
            Sleep(TDuration::Seconds(5));
        }
    }
}

TVector<TString> PrepareDailyTables(NYT::IClientPtr& ytClient, const TString& ytWorkDir, ui32 dayCount, const TYqlParams& yqlParams) {
    const auto moscowTz = NDatetime::GetTimeZone("Europe/Moscow");
    auto currentDay = NDatetime::ToCivilTime(TInstant::Now(), moscowTz);

    TVector<TString> inputTables;
    TVector<TString> outputTables;
    TVector<TString> resultTables;

    size_t triedDays = 0;
    while (resultTables.size() < dayCount && triedDays < 2 * dayCount) {
        currentDay.Add(NDatetime::TSimpleTM::F_DAY, -1);
        const auto tableName = currentDay.ToString("%Y%m%d");

        const auto blenderTable = BLENDER_SQUEEZE_DIR + "/" + tableName;
        const auto outputTable = ytWorkDir + "/" + tableName;
        bool addTableToResults = true;

        if (!ytClient->Exists(blenderTable)) {
            Cerr << "Skipping missing table: " << blenderTable << Endl;
            addTableToResults = false;
        } else if (ytClient->Exists(outputTable)) {
            Cerr << "Output table " << outputTable << " already exists; will not re-compute it" << Endl;
        } else {
            inputTables.push_back(blenderTable);
            outputTables.push_back(outputTable);
        }

        if (addTableToResults) {
            resultTables.push_back(outputTable);
        }
        ++triedDays;
    }

    Y_ENSURE(resultTables.size() == dayCount, "Too many missing tables, bailing out");

    TVector<TString> scripts;
    for (size_t i : xrange(inputTables.size())) {
        const auto& input = inputTables[i];
        auto currentScript = PrepareScript(yqlParams, "/process_daily_tables.yql");
        SubstGlobal(currentScript, "$INPUT_TABLE", input);
        SubstGlobal(currentScript, "$OUTPUT_TABLE", outputTables[i]);
        scripts.push_back(currentScript);
    }
    RunYQLScriptsOrDie(scripts, yqlParams.ApiUrl, yqlParams.OAuthToken);

    return resultTables;
}

void CombineTables(const TVector<TString>& dailyTables, const TString& combinedTable, const TYqlParams& yqlParams) {
    auto script = PrepareScript(yqlParams, "/combine_tables.yql");

    TString inputTablesForConcat;
    for (const auto& table : dailyTables) {
        if (!inputTablesForConcat.empty()) {
            inputTablesForConcat += ",";
        }
        inputTablesForConcat += "[" + table + "]";
    }

    SubstGlobal(script, "$INPUT_TABLES", inputTablesForConcat);
    SubstGlobal(script, "$OUTPUT_TABLE", combinedTable);
    RunYQLScriptsOrDie({script}, yqlParams.ApiUrl, yqlParams.OAuthToken);
}

using NQueryWizardFeatures::TFeatures;
using NQueryWizardFeatures::TWizardData;

void SaveResult(NYT::IClientPtr& ytClient, const TString& combinedTable, const TString& outputDir, ui32 dayCount) {
    auto reader = ytClient->CreateTableReader<NYT::TNode>(combinedTable);

    TCompactTrieBuilder<char> trieBuilder(CTBF_PREFIX_GROUPED);
    TBlobWriter dataWriter;

    size_t done = 0;
    const auto total = ytClient->Get(combinedTable + "/@row_count").AsInt64();
    while (reader->IsValid()) {
        const auto& row = reader->GetRow();

        const auto query = row["key"].AsString();
        trieBuilder.Add(query, dataWriter.CurRecordId());

        TFeatures features;

        const auto requests = row["requests"].AsUint64();

        auto normalize = [&requests](double value) {
            return value / requests;
        };

        auto fillWizardData = [&normalize](TWizardData& wizard, ui64 countPerRequest, double surplus) {
            wizard.SetCountPerRequest(normalize(countPerRequest));
            wizard.SetSurplus(normalize(surplus));
        };

        features.SetUsersPerDay(row["users"].AsUint64() / static_cast<double>(dayCount));

        static const THashMap<TString, TWizardData* (TFeatures::*)()> wizardTypeToWizardDataGetter = {
            { "wiz-video", &TFeatures::MutableVideo },
            { "wiz-images", &TFeatures::MutableImages },
            { "wiz-weather", &TFeatures::MutableWeather },
            { "wiz-musicplayer", &TFeatures::MutableMusic },
            { "wiz-maps", &TFeatures::MutableMaps },
            { "wiz-companies", &TFeatures::MutableCompanies },
            { "src-BNO-fair", &TFeatures::MutableBno },
            { "wiz-market", &TFeatures::MutableMarket },
        };

        ui64 entityCount = 0;
        double entitySurplus = 0.0;

        for (const auto& dataForWizard : row["surplus_data"].AsList()) {
            const auto& wizardType = dataForWizard[0].AsString();
            const auto& numbers = dataForWizard[1].AsList();
            const auto count = numbers[0].AsInt64();
            const auto surplus = numbers[1].AsDouble();

            if (const auto* getWizardData = wizardTypeToWizardDataGetter.FindPtr(wizardType)) {
                fillWizardData(*(features.*(*getWizardData))(), count, surplus);
            } else if (EqualToOneOf(wizardType, "wiz-entity_search-all_serp", "wiz-entity_search")) {
                entityCount += count;
                entitySurplus += surplus;
            }
        }

        fillWizardData(*features.MutableEntity(), entityCount, entitySurplus);

        TString serializedFeatures;
        Y_ENSURE(features.SerializeToString(&serializedFeatures));
        dataWriter.AddStringBuf(serializedFeatures);
        dataWriter.NextRecord();

        reader->Next();

        ++done;
        if (done % 10000 == 0) {
            Cerr << done << "/" << total << Endl;
        }
    }

    trieBuilder.SaveToFile(outputDir + "/query_wizard_features.trie");
    dataWriter.SaveTo(outputDir + "/query_wizard_features.data");
}

void VerifyResults(const TString& outputDir) {
    using NSuggest::TBlobReader;
    TCompactTrie<char> trie(TBlob::FromFile(outputDir + "/query_wizard_features.trie"));
    TBlobReader reader(outputDir + "/query_wizard_features.data");

    using TWizardDataGetter = const TWizardData& (TFeatures::*)() const;

    auto getWizardDataForQuery = [&trie, &reader](const TString& query, TWizardDataGetter getter) {
        ui64 idx;
        Y_ENSURE(trie.Find(query, &idx));
        TBlobReader::TRec rec;
        TFeatures features;
        Y_ENSURE(reader.GetRecord(idx, &rec));
        Y_ENSURE(features.ParseFromArray(rec.Start, rec.End - rec.Start));

        return (features.*getter)();
    };

    auto checkQueryHasGoodWizard = [&getWizardDataForQuery](const TString& query, TWizardDataGetter getter) {
        const auto wizardData = getWizardDataForQuery(query, getter);
        const auto cpr = wizardData.GetCountPerRequest();
        Y_ENSURE(cpr, "Request count for " << query << " is too low: " << cpr);
        const auto surplus = wizardData.GetSurplus();
        Y_ENSURE(surplus, "Surplus for " << query << " is too low: " << surplus);
    };

    checkQueryHasGoodWizard("видео", &TFeatures::GetVideo);
    checkQueryHasGoodWizard("картинки", &TFeatures::GetImages);
    checkQueryHasGoodWizard("погода", &TFeatures::GetWeather);
    checkQueryHasGoodWizard("стас михайлов", &TFeatures::GetMusic);
    checkQueryHasGoodWizard("сергей есенин", &TFeatures::GetEntity);
    checkQueryHasGoodWizard("карты", &TFeatures::GetMaps);
    checkQueryHasGoodWizard("леруа мерлен", &TFeatures::GetCompanies);
    checkQueryHasGoodWizard("вконтакте", &TFeatures::GetBno);
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    NLastGetopt::TOpts opts;

    TString ytProxy;
    ui32 dayCount{};
    TString ytWorkDir;
    TString outputDir;
    TString yqlApiUrl;
    TString oauthToken;
    TString yqlAuth;
    TString yqlClusterName;
    bool verifyResults = false;

    opts.AddLongOption("yt-proxy", "The YT server to use")
        .DefaultValue("hahn")
        .StoreResult(&ytProxy);
    opts.AddLongOption("day-count", "The number of days to prepare the features for")
        .Required()
        .StoreResult(&dayCount);
    opts.AddLongOption("yt-work-dir", "The YT directory with processed blender tables")
        .Required()
        .StoreResult(&ytWorkDir);
    opts.AddLongOption("output-dir", "The resulting binary files will be saved to this directory on the local FS")
        .Required()
        .StoreResult(&outputDir);
    opts.AddLongOption("yql-api-url", "The url for YQL API (duh)")
        .DefaultValue("https://yql.yandex.net/api/v2")
        .StoreResult(&yqlApiUrl);
    opts.AddLongOption("oauth-token", "The OAuth token to use for YQL API")
        .Required()
        .StoreResult(&oauthToken);
    opts.AddLongOption("yql-auth", "The YQL token alias for YT (https://wiki.yandex-team.ru/yql/userguide/cli/#ukazanietokenovytikvotyamr)")
        .Required()
        .StoreResult(&yqlAuth);
    opts.AddLongOption("yql-cluster-name", "The short name of the cluster for YQL scripts (i.e. hahn)")
        .DefaultValue("hahn")
        .StoreResult(&yqlClusterName);
    opts.AddLongOption("verify-results", "Run some basic tests after generating results to ensure we don't output complete trash")
        .Optional()
        .NoArgument()
        .StoreValue(&verifyResults, true);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    auto ytClient = NYT::CreateClient(ytProxy);

    const auto combinedTable = ytWorkDir + "/combined";

    const TYqlParams yqlParams {
        yqlApiUrl,
        oauthToken,
        yqlClusterName,
        yqlAuth,
    };

    const auto dailyTables = PrepareDailyTables(ytClient, ytWorkDir, dayCount, yqlParams);

    CombineTables(dailyTables, combinedTable, yqlParams);
    SaveResult(ytClient, combinedTable, outputDir, dayCount);

    if (verifyResults) {
        VerifyResults(outputDir);
    }
}

