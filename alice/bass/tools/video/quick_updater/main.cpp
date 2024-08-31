#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/content_db.h>
#include <alice/bass/libs/video_common/defs.h>
#include <alice/bass/libs/video_common/providers.h>
#include <alice/bass/libs/video_common/tvm_cache_delegate.h>
#include <alice/bass/libs/video_common/utils.h>
#include <alice/bass/libs/video_content/common.h>
#include <alice/bass/libs/video_tools/quick_updater/quick_updater_utils.h>
#include <alice/bass/libs/ydb_config/config.h>
#include <alice/bass/libs/ydb_helpers/exception.h>
#include <alice/bass/libs/ydb_helpers/queries.h>
#include <alice/bass/libs/ydb_helpers/table.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_scheme/scheme.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/algorithm.h>
#include <util/generic/scope.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/printf.h>
#include <util/system/compiler.h>

#include <cstdlib>
#include <utility>

using namespace NQuickUpdater;
using namespace NVideoCommon;
using namespace NYdbHelpers;

namespace {

constexpr TStringBuf SNAPSHOT_DELIM = "_q";

const TString ENV_TVM2_BASS_ID = "TVM2_BASS_ID";
const TString ENV_TVM2_BASS_SECRET = "TVM2_BASS_SECRET";
constexpr TStringBuf KINOPOISK_TVM_ID = "2009799";

TInstant TryParseTimestampOpt(TStringBuf timestamp) {
    TInstant result;
    if (!TInstant::TryParseIso8601(timestamp, result)) {
        Cerr << "Failed to parse timestamp in ISO8601: " << timestamp << ", exiting..." << Endl;
        exit(EXIT_FAILURE);
    }
    return result;
}

TMaybe<TString> GetNextSnapshotName(TStringBuf prev) {
    TStringBuf prefix, suffix;
    if (!prev.TrySplit(SNAPSHOT_DELIM, prefix, suffix))
        return TStringBuilder() << prev << SNAPSHOT_DELIM << '0';
    ui64 version;
    if (!TryFromString(suffix, version)) {
        LOG(ERR) << "Failed to parse version: " << suffix << Endl;
        return Nothing();
    }
    if (version == Max<ui64>()) {
        LOG(ERR) << "Version is too high: " << version << Endl;
        return Nothing();
    }
    ++version;
    return TStringBuilder() << prefix << SNAPSHOT_DELIM << version;
}

int Main(int argc, const char* argv[]) {
    TString database;
    TString endpoint;
    TString providerList;
    TInstant timestamp;

    TString availableProviders = Join(", ", PROVIDER_AMEDIATEKA, PROVIDER_IVI, PROVIDER_KINOPOISK);
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts.AddLongOption("database", "YDB database.").StoreResult(&database).RequiredArgument("YDB_DATABASE");
    opts.AddLongOption("endpoint", "YDB endpoint.").StoreResult(&endpoint).RequiredArgument("YDB_ENDPOINT");
    opts.AddLongOption("timestamp", "Current timestamp.")
        .StoreMappedResultT<TStringBuf>(&timestamp, &TryParseTimestampOpt)
        .DefaultValue(TInstant::Now().ToString())
        .RequiredArgument("TIMESTAMP");
    opts.AddLongOption("enabled-providers",
                       "Comma-separated list of providers to download content from. Available providers: " +
                           availableProviders)
        .StoreResult(&providerList)
        .DefaultValue(availableProviders);

    NLastGetopt::TOptsParseResult result(&opts, argc, argv);

    const auto ydbToken = NVideoContent::GetYDBToken();
    const auto ydbConfig = NYdb::TDriverConfig{}.SetEndpoint(endpoint).SetDatabase(database).SetAuthToken(ydbToken);

    TVector<TStringBuf> providerNames;
    Split(TStringBuf(providerList), ",", providerNames);

    NYdb::TDriver driver{ydbConfig};
    Y_SCOPE_EXIT(&driver) {
        driver.Stop(true /* wait */);
    };

    NYdb::NScheme::TSchemeClient schemeClient{driver};
    NYdb::NTable::TTableClient tableClient{driver};

    NYdbConfig::TConfig config{tableClient, database};

    const auto latestStatus = config.Get(NYdbConfig::KEY_VIDEO_LATEST);
    if (!latestStatus.IsSuccess()) {
        LOG(ERR) << "Failed to get info about latest db: " << StatusToString(latestStatus) << Endl;
        return EXIT_FAILURE;
    }

    const auto latest = latestStatus.GetValue();
    if (!latest.Defined()) {
        LOG(ERR) << "No " << NYdbConfig::KEY_VIDEO_LATEST << " key in the config" << Endl;
        return EXIT_FAILURE;
    }

    NYdbHelpers::TTablePath itemsPath{database,
                                      NYdbHelpers::Join(*latest, NVideoContent::TVideoItemsLatestTableTraits::NAME)};
    NYdbHelpers::TTablePath seasonsPath{database,
                                        NYdbHelpers::Join(*latest, NVideoContent::TVideoSeasonsTableTraits::NAME)};

    auto itemsStatus = GetSerialsForUpdate(tableClient, itemsPath, seasonsPath, timestamp);
    if (!itemsStatus.IsSuccess()) {
        LOG(ERR) << "Failed to get info about outdated serials: " << NYdbHelpers::StatusToString(itemsStatus) << Endl;
        return EXIT_FAILURE;
    }

    const auto& items = itemsStatus.GetItems();
    if (items.empty()) {
        LOG(INFO) << "Nothing to update, exiting..." << Endl;
        return EXIT_SUCCESS;
    }

    LOG(INFO) << "Items count to update: " << items.size() << Endl;

    const auto next = GetNextSnapshotName(*latest);
    if (!next.Defined()) {
        LOG(ERR) << "Failed to generate a name to the next snapshot" << Endl;
        return EXIT_FAILURE;
    }

    std::unique_ptr<TTransaction> copyTransaction;
    const auto status = CopyTables(tableClient, schemeClient, TTablePath{database, *latest},
                                   TTablePath{database, *next}, copyTransaction);
    if (!status.IsSuccess()) {
        LOG(ERR) << "Cannot create copy tables transaction, error: " << status.GetIssues().ToString() << Endl;
        return EXIT_FAILURE;
    }

    TIviGenresDelegate delegate;
    TIviGenres iviGenres{delegate};
    TRPSConfig rps;

    TString tvm2BassId = GetEnvOrThrow(ENV_TVM2_BASS_ID);
    TString tvm2BassSecret = GetEnvOrThrow(ENV_TVM2_BASS_SECRET);
    const auto ottTicket = GetSingleTvm2Ticket(tvm2BassId, tvm2BassSecret, KINOPOISK_TVM_ID);

    TUAPIDownloaderProviderFactory factory{ottTicket};
    TContentInfoProvidersCache providers{factory, iviGenres, providerNames, rps};

    const auto infos = DownloadAll(items, providers);

    TTablePath snapshot{database, *next};

    TYdbContentDb db{tableClient, NVideoCommon::TVideoTablesPaths::MakeDefault(snapshot),
                     NVideoCommon::TIndexTablesPaths::MakeDefault(snapshot)};
    UpdateContentDbTvShows(db, tableClient, snapshot, infos);

    copyTransaction->Commit();

    LOG(INFO) << "Updating config..." << Endl;
    NYdbHelpers::ThrowOnError(config.Set(TString{NYdbConfig::KEY_VIDEO_LATEST}, *next));

    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, const char* argv[]) {
    try {
        return Main(argc, argv);
    } catch (const yexception&) {
        LOG(ERR) << CurrentExceptionMessage() << Endl;
        return EXIT_FAILURE;
    }
}
