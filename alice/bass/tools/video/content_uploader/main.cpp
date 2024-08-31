#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/bass/libs/video_common/utils.h>

#include <alice/bass/libs/video_content/common.h>
#include <alice/bass/libs/video_content/protos/rows.pb.h>

#include <alice/bass/libs/ydb_config/config.h>
#include <alice/bass/libs/ydb_helpers/exception.h>
#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/scoped_data.h>
#include <alice/bass/libs/ydb_helpers/table.h>
#include <alice/bass/libs/ydb_kv/kv.h>
#include <alice/bass/libs/ydb_kv/protos/kv.pb.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/init.h>
#include <mapreduce/yt/util/ypath_join.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/scope.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>

#include <cstddef>

namespace {
const size_t VIDEO_SNAPSHOTS_TO_KEEP = 3;

template <typename T>
struct TVoid {};

template <typename TInput, typename TFn>
void Transform(const TInput& input, TVoid<TInput>, TFn&& fn) {
    fn(input);
}

class TCopyTablesTransaction final {
public:
    TCopyTablesTransaction(NYT::IClient& ytClient, NYdb::NTable::TTableClient& ydbTableClient,
                           NYdb::NScheme::TSchemeClient& ydbSchemeClient, const TString& ytRoot,
                           const NYdbHelpers::TTablePath& ydbRoot)
        : YTClient(ytClient)
        , YDBTableClient(ydbTableClient)
        , YDBSchemeClient(ydbSchemeClient)
        , YTRoot(ytRoot)
        , YDBRoot(ydbRoot) {
    }

    void MakeDirectory(const NYdbHelpers::TTablePath& path) {
        LOG(INFO) << "Making directory: " << path << Endl;
        CreatedDirectories.emplace_back(NYdbHelpers::TScopedDirectory::Make(YDBSchemeClient, path));
    }

    template <typename TTableTraits>
    void CopyTable() {
        using TInput = typename TTableTraits::TYTScheme;
        using TOutput = typename TTableTraits::TScheme;

        const auto ytTable = NYT::JoinYPaths(YTRoot, TTableTraits::YT_NAME);

        const i64 rowsCount = YTClient.Get(ytTable + "/@row_count").AsInt64();
        LOG(INFO) << "Table " << TTableTraits::NAME << " has " << rowsCount << " rows" << Endl;
        if (rowsCount == 0) {
            ythrow yexception() << "Failed because table " << TTableTraits::YT_NAME << "is empty";
        }

        auto reader = YTClient.CreateTableReader<TInput>(ytTable);
        auto writer = MakeTableWriter<TTableTraits>();

        LOG(INFO) << "Filling table " << TTableTraits::NAME << "..." << Endl;
        for (; reader->IsValid(); reader->Next())
            Transform(reader->GetRow(), TVoid<TOutput>{}, [&](const TOutput& output) { writer.AddRow(output); });
    }

    template <typename TTableTraits, typename TConverter>
    void CopyTableAndCreateIds(TConverter&& converter) {
        using TInput = typename TTableTraits::TYTScheme;
        using TOutput = typename TTableTraits::TScheme;

        const auto ytTable = NYT::JoinYPaths(YTRoot, TTableTraits::YT_NAME);

        auto reader = YTClient.CreateTableReader<TInput>(ytTable);
        auto writer = MakeTableWriter<TTableTraits>();

        auto piids = MakeTableWriter<NVideoContent::TProviderItemIdIndexTableTraits>();
        auto hrids = MakeTableWriter<NVideoContent::THumanReadableIdIndexTableTraits>();
        auto kpids = MakeTableWriter<NVideoContent::TKinopoiskIdIndexTableTraits>();

        LOG(INFO) << "Filling table " << TTableTraits::NAME << " with indexes..." << Endl;
        for (ui64 id = 0; reader->IsValid(); reader->Next(), ++id) {
            const auto input = reader->GetRow();
            TOutput output;
            converter(id, input, output);
            writer.AddRow(output);

            NVideoCommon::TVideoItem item;
            if (!NVideoCommon::Des(input.GetContent(), item))
                continue;

            NVideoContent::TProviderItemIdIndexTableTraits::TScheme ppid;
            if (NVideoCommon::Ser(item, id, ppid))
                piids.AddRow(ppid);

            NVideoContent::THumanReadableIdIndexTableTraits::TScheme hrid;
            if (NVideoCommon::Ser(item, id, hrid))
                hrids.AddRow(hrid);

            NVideoContent::TKinopoiskIdIndexTableTraits::TScheme kpid;
            if (NVideoCommon::Ser(item, id, kpid))
                kpids.AddRow(kpid);
        }
    }

    template <>
    void CopyTable<NVideoContent::TVideoItemsV5TableTraits>() {
        CopyTableAndCreateIds<NVideoContent::TVideoItemsV5TableTraits>(NVideoCommon::VideoItemRowV5YTToV5YDb);
    }

    void Commit() {
        for (auto& directory : CreatedDirectories)
            directory.Release();
        for (auto& table : CreatedTables)
            table.Release();
    }

private:
    void DropTable(const NYdbHelpers::TTablePath& ydbTable) {
        LOG(INFO) << "Dropping table " << ydbTable << "..." << Endl;
        NYdbHelpers::DropTable(YDBTableClient, ydbTable);
    }

    template <typename TTableTraits>
    void CreateTable(const NYdbHelpers::TTablePath& ydbTable) {
        LOG(INFO) << "Creating table " << ydbTable << "..." << Endl;
        CreatedTables.emplace_back(NYdbHelpers::TScopedTable::Make<TTableTraits>(YDBTableClient, ydbTable));
    }

    template <typename TTableTraits>
    NYdbHelpers::TTableWriter<typename TTableTraits::TScheme> MakeTableWriter() {
        const auto path = NYdbHelpers::Join(YDBRoot, TTableTraits::NAME);

        DropTable(path);
        CreateTable<TTableTraits>(path);
        return NYdbHelpers::TTableWriter<typename TTableTraits::TScheme>(YDBTableClient, path);
    }

    NYT::IClient& YTClient;
    NYdb::NTable::TTableClient& YDBTableClient;
    NYdb::NScheme::TSchemeClient& YDBSchemeClient;

    const TString YTRoot;
    const NYdbHelpers::TTablePath YDBRoot;

    TVector<NYdbHelpers::TScopedTable> CreatedTables;
    TVector<NYdbHelpers::TScopedDirectory> CreatedDirectories;
};

int Main(int argc, const char* argv[]) {
    NYT::Initialize(argc, argv);

    TString ytRoot;

    TString database;
    TString endpoint;
    TString suffix;
    bool needsConfigUpdate;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    opts.AddLongOption("yt-root", "YT video root directory.").StoreResult(&ytRoot).RequiredArgument("YT_ROOT");

    opts.AddLongOption("database", "YDB database.").StoreResult(&database).RequiredArgument("YDB_DATABASE");
    opts.AddLongOption("endpoint", "YDB endpoint.").StoreResult(&endpoint).RequiredArgument("YDB_ENDPOINT");
    opts.AddLongOption("suffix", "YDB tables suffix.").StoreResult(&suffix).RequiredArgument("YDB_SUFFIX");
    opts.AddLongOption("update-config", "Does the program run need to update config.")
        .StoreResult(&needsConfigUpdate)
        .DefaultValue(true);

    NLastGetopt::TOptsParseResult result(&opts, argc, argv);

    const TString directory = TStringBuilder() << NVideoContent::VIDEO_TABLES_PREFIX << suffix;

    auto ytClient = NYT::CreateClient(TString{NVideoContent::YT_PROXY});
    Y_ASSERT(ytClient);

    const auto ydbToken = NVideoContent::GetYDBToken();
    const auto config = NYdb::TDriverConfig().SetEndpoint(endpoint).SetDatabase(database).SetAuthToken(ydbToken);

    NYdbHelpers::TScopedDriver driver{config};

    NYdb::NScheme::TSchemeClient ydbSchemeClient(driver.Driver);
    NYdb::NTable::TTableClient ydbTableClient(driver.Driver);

    TCopyTablesTransaction transaction(*ytClient, ydbTableClient, ydbSchemeClient, ytRoot,
                                       NYdbHelpers::TTablePath{database, directory});

    transaction.MakeDirectory(NYdbHelpers::TTablePath(database, directory));

    transaction.CopyTable<NVideoContent::TVideoKeysTableTraits>();
    transaction.CopyTable<NVideoContent::TVideoItemsV5TableTraits>();
    transaction.CopyTable<NVideoContent::TVideoSerialsTableTraits>();
    transaction.CopyTable<NVideoContent::TVideoSeasonsTableTraits>();
    transaction.CopyTable<NVideoContent::TVideoEpisodesTableTraits>();
    transaction.CopyTable<NVideoContent::TProviderUniqueItemsTableTraitsV2>();

    NYdbConfig::TConfig ydbConfig(ydbTableClient, database);
    if (!ydbConfig.Exists().IsSuccess()) {
        // This call may fail, for example when config has been
        // created concurrently. So it's okay to skip errors here.
        ydbConfig.Create();
    }

    if (needsConfigUpdate) {
        LOG(INFO) << "Updating config..." << Endl;
        NYdbHelpers::ThrowOnError(ydbConfig.Set(TString{NYdbConfig::KEY_VIDEO_LATEST}, directory));
    } else {
        LOG(INFO) << "--update-config was set to 'false', skipping config update..." << Endl;
    }

    // If we get to the point, everything is fine and we don't
    // need to drop created tables.
    transaction.Commit();

    LOG(INFO) << "Dropping old tables..." << Endl;
    if (!NVideoContent::DropOldDirectoriesWithTables(ydbSchemeClient, ydbTableClient, database,
                                                     NVideoContent::VIDEO_TABLES_PREFIX, directory /* latest */,
                                                     VIDEO_SNAPSHOTS_TO_KEEP)) {
        LOG(ERR) << "Failed to remove old video snapshots" << Endl;
    }

    return EXIT_SUCCESS;
}
} // namespace

int main(int argc, const char* argv[])
try {
    return Main(argc, argv);
} catch (const yexception& e) {
    LOG(ERR) << e << Endl;
    return EXIT_FAILURE;
}
