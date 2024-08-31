#include "alice/bass/libs/video_common/content_db_check_utils.h"

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

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/string_utils/scan/scan.h>

#include <util/generic/scope.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>

#include <cstddef>

using namespace NVideoCommon;
using namespace NYdbHelpers;

namespace {

int Main(int argc, const char* argv[]) {
    TString database;
    TString endpoint;
    TString candidateSuffix;

    TString checkListFile;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    opts.AddLongOption("database", "YDB database to verify the candidate")
        .StoreResult(&database)
        .RequiredArgument("YDB_DATABASE");

    opts.AddLongOption("endpoint", "YDB database endpoint").StoreResult(&endpoint).RequiredArgument("YDB_ENDPOINT");

    opts.AddLongOption("candidate-suffix", "YDB candidate tables suffix")
        .StoreResult(&candidateSuffix)
        .RequiredArgument("YDB_SUFFIX");

    opts.AddLongOption("check-json-file").StoreResult(&checkListFile).RequiredArgument("JSON_FILE");

    NLastGetopt::TOptsParseResult result(&opts, argc, argv);

    TDbCheckList dbCheckList = LoadCheckListFromFileOrThrow(checkListFile);

    const auto ydbToken = NVideoContent::GetYDBToken();
    const NYdb::TDriverConfig config =
        NYdb::TDriverConfig().SetEndpoint(endpoint).SetDatabase(database).SetAuthToken(ydbToken);

    TScopedDriver driver{config};
    NYdb::NTable::TTableClient ydbTableClient{driver.Driver};

    NYdbConfig::TConfig ydbConfig{ydbTableClient, database};
    if (!ydbConfig.Exists().IsSuccess()) {
        LOG(WARNING) << "YDB config has not been found, assuming an empty reference DB!" << Endl;
        // Comparison against an empty db is always successful.
        return EXIT_SUCCESS;
    }

    const auto snapshot = ydbConfig.Get(NYdbConfig::KEY_VIDEO_LATEST);
    if (!snapshot.IsSuccess() || !snapshot.GetValue()) {
        LOG(WARNING) << "Cannot find the last snapshot, assuming an empty reference DB!" << Endl;
        return EXIT_SUCCESS;
    }

    const TString candidateDir = TStringBuilder{} << NVideoContent::VIDEO_TABLES_PREFIX << candidateSuffix;
    const TString referenceDir = TStringBuilder{} << *snapshot.GetValue();

    TDbChecker dbChecker{ydbTableClient, database, candidateDir, referenceDir};
    if (const auto error = dbChecker.CheckContentDb(dbCheckList)) {
        LOG(ERR) << error->Msg << Endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, const char* argv[]) try { return Main(argc, argv); } catch (const yexception& e) {
    LOG(ERR) << e << Endl;
    return EXIT_FAILURE;
}
