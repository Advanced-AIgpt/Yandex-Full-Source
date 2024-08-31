#include "kp_genres.h"

#include <alice/bass/libs/video_common/defs.h>
#include <alice/bass/libs/video_common/kinopoisk_recommendations.h>
#include <alice/bass/libs/video_common/kinopoisk_utils.h>
#include <alice/bass/libs/video_content/common.h>
#include <alice/bass/libs/video_content/protos/rows.pb.h>

#include <alice/bass/libs/ydb_config/config.h>
#include <alice/bass/libs/ydb_helpers/exception.h>
#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/table.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/protos/yamr.pb.h>
#include <mapreduce/yt/util/ypath_join.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash.h>
#include <util/generic/scope.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/system/env.h>
#include <util/system/yassert.h>

#include <cstdlib>

namespace {

const TStringBuf KP_ID_KEY = "kp_id";
const TString YDB_TOKEN = "YDB_TOKEN";
const TString YT_PROXY = "hahn";

const TStringBuf TABLE_FILM_CONTENT = "film_content";
const TStringBuf TABLE_FILM_GENRES = "film_genres";
const TStringBuf TABLE_RELEASE_DATES = "release_dates";
const TStringBuf TABLE_YASTATION_ONLINES = "yastation_onlines";

enum EKinopoiskFilmContentType { Film = 0, TvShow = 1 };

using TFilmInfo = NVideoCommon::TKinopoiskFilmInfo;
using TFilmsInfo = THashMap<TString, TFilmInfo>;

template <typename TRow>
struct TKinopoiskSVODTraits;

template <>
struct TKinopoiskSVODTraits<NVideoContent::NProtos::TYDBKinopoiskSVODRowV2> {
    static TStringBuf GetTablePrefix() {
        return NVideoCommon::KINOPOISK_SVOD_V2_TABLE_PREFIX;
    }

    static TStringBuf GetTableKey() {
        return NYdbConfig::KEY_KINOPOISK_SVOD_LATEST_V2;
    }
};

template <typename TRow>
void AnnotateFilmsInfoCommon(const TFilmInfo& info, TRow& row) {
    row.SetKinopoiskRating(info.Rating);
    row.SetCountries(JoinSeq(",", info.Countries));
    row.SetGenres(JoinSeq(",", info.Genres));
    if (info.ContentType != NVideoCommon::EContentType::Null)
        row.SetContentType(ToString(info.ContentType));
}

void AnnotateFilmsInfo(const TFilmsInfo& filmsInfo, NVideoContent::NProtos::TYDBKinopoiskSVODRowV2& row) {
    const auto& kpId = row.GetKinopoiskId();
    if (const auto* info = filmsInfo.FindPtr(kpId)) {
        AnnotateFilmsInfoCommon(*info, row);
        if (info->ReleaseDate)
            row.SetReleaseDate(TFilmInfo::Ser(*info->ReleaseDate));
    }
}

TFilmsInfo DownloadFilmsInfo(NYT::IClientPtr ytClient, const NYT::TYPath& ytKinopoiskDir) {
    TFilmsInfo films;

    {
        const auto ytFilmContentPath = NYT::JoinYPaths(ytKinopoiskDir, TABLE_FILM_CONTENT);
        LOG(INFO) << "Downloading table " << ytFilmContentPath << " ..." << Endl;

        auto reader = ytClient->CreateTableReader<NYT::TYamrNoSubkey>(ytFilmContentPath);
        if (!reader) {
            ythrow yexception() << "Failed to create YT table reader for path " << ytFilmContentPath;
        }
        for (; reader->IsValid(); reader->Next()) {
            const NYT::TYamrNoSubkey& input = reader->GetRow();

            NSc::TValue content;
            if (!NSc::TValue::FromJson(content, input.GetValue())) {
                ythrow yexception() << "Unable to parse content for " << input.GetKey();
            }

            TFilmInfo& output = films[input.GetKey()];
            output.Id = input.GetKey();
            output.Rating = content["rating"].GetNumber();

            ui32 type = content["type"].GetIntNumber();
            if (type == EKinopoiskFilmContentType::TvShow) {
                output.ContentType = NVideoCommon::EContentType::TvShow;
            }
        }
    }

    {
        const auto ytFilmGenresPath = NYT::JoinYPaths(ytKinopoiskDir, TABLE_FILM_GENRES);
        LOG(INFO) << "Downloading table " << ytFilmGenresPath << " ..." << Endl;

        auto reader = ytClient->CreateTableReader<NVideoContent::NProtos::TYtKinopoiskFilmGenresRow>(ytFilmGenresPath);
        if (!reader) {
            ythrow yexception() << "Failed to create YT table reader for path " << ytFilmGenresPath;
        }
        for (; reader->IsValid(); reader->Next()) {
            const NVideoContent::NProtos::TYtKinopoiskFilmGenresRow& input = reader->GetRow();

            TFilmInfo& output = films[ToString(input.GetFilmId())];
            if (!output.Countries) {
                for (TStringBuf buf = input.GetCountriesRus(); buf;) {
                    output.Countries.push_back(TString{buf.NextTok(',')});
                }
            }
            const TString& genreStr = input.GetGenreRus();
            if (TMaybe<NVideoCommon::EVideoGenre> genre = ParseKinopoiskGenre(genreStr)) {
                output.Genres.push_back(*genre);
            }

            if (output.ContentType == NVideoCommon::EContentType::Null) {
                output.ContentType = ParseKinopoiskContentType(genreStr);
            }
        }
    }

    {
        const auto ytReleaseDatesPath = NYT::JoinYPaths(ytKinopoiskDir, TABLE_RELEASE_DATES);
        LOG(INFO) << "Downloading table " << ytReleaseDatesPath << " ..." << Endl;

        auto reader =
            ytClient->CreateTableReader<NVideoContent::NProtos::TYtKinopoiskReleaseDatesRow>(ytReleaseDatesPath);
        if (!reader) {
            ythrow yexception() << "Failed to create YT table reader for path " << ytReleaseDatesPath;
        }
        for (; reader->IsValid(); reader->Next()) {
            const auto& input = reader->GetRow();
            if (!input.HasWorldReleaseDate() || input.GetWorldReleaseDate().empty())
                continue;

            TFilmInfo& output = films[ToString(input.GetFilmId())];
            try {
                output.ReleaseDate = TFilmInfo::Des(input.GetWorldReleaseDate());
            } catch (const yexception& e) {
                LOG(ERR) << "Failed to parse release date: " << e << Endl;
            }
        }
    }

    return films;
}

template <typename TRow>
class TSVODBuilder {
public:
    using TTraits = TKinopoiskSVODTraits<TRow>;

    TSVODBuilder(NYT::IClientPtr ytClient, const NYT::TYPath& ytPath, NYdb::NTable::TTableClient& ydbTableClient,
                 NYdb::NScheme::TSchemeClient& ydbSchemeClient, NYdbConfig::TConfig& config, const TString& database,
                 const TString& suffix, bool& dropLatest)
        : YtClient(ytClient)
        , YtPath(ytPath)
        , YdbTableClient(ydbTableClient)
        , YdbSchemeClient(ydbSchemeClient)
        , YdbPath{database, TTraits::GetTablePrefix() + suffix}
        , Config(config)
        , DropLatest(dropLatest) {
    }

    ~TSVODBuilder() {
        if (!Built)
            return;

        if (DropLatest) {
            NYdbHelpers::DropTable(YdbTableClient, YdbPath);
            return;
        }

        if (!NVideoCommon::DropOldKinopoiskSVODTables(YdbSchemeClient, YdbTableClient, YdbPath.Database,
                                                      TTraits::GetTablePrefix(), YdbPath.Name /* latest */)) {
            LOG(ERR) << "Failed to remove YDB table " << YdbPath << Endl;
        }
    }

    void Build(const TFilmsInfo& filmsInfo) {
        LOG(INFO) << "Dropping table " << YdbPath << "..." << Endl;
        NYdbHelpers::DropTable(YdbTableClient, YdbPath);

        LOG(INFO) << "Creating table " << YdbPath << "..." << Endl;
        NYdbHelpers::CreateTableOrFail<TRow>(YdbTableClient, YdbPath,
                                             NVideoContent::VIDEO_TABLE_KINOPOISK_PRIMARY_KEYS);
        Built = true;

        CopyYtToYdb(filmsInfo);
        NYdbHelpers::ThrowOnError(Config.Set(TTraits::GetTableKey(), YdbPath.Name));
    }

private:
    void CopyYtToYdb(const TFilmsInfo& filmsInfo) {
        NYdbHelpers::TTableWriter<TRow> writer(YdbTableClient, YdbPath);

        LOG(INFO) << "Copying data from " << YtPath << " to " << YdbPath << " ..." << Endl;
        auto reader = YtClient->template CreateTableReader<NYT::TYamrNoSubkey>(YtPath);
        if (!reader)
            ythrow yexception() << "Failed to create YT table reader for path " << YtPath;

        for (; reader->IsValid(); reader->Next()) {
            NYT::TYamrNoSubkey input;
            reader->MoveRow(&input);

            NSc::TValue content;
            if (!NSc::TValue::FromJson(content, input.GetValue()))
                ythrow yexception() << "Unable to parse content for " << input.GetKey();

            if (!content.Has(KP_ID_KEY))
                ythrow yexception() << "Unable to find " << KP_ID_KEY << " for " << input.GetKey();

            const TString kpId = TString{content[KP_ID_KEY].GetString()};

            TRow output;
            output.SetKinopoiskId(kpId);
            AnnotateFilmsInfo(filmsInfo, output);

            writer.AddRow(output);
        }
    }

    NYT::IClientPtr YtClient;
    NYT::TYPath YtPath;

    NYdb::NTable::TTableClient& YdbTableClient;
    NYdb::NScheme::TSchemeClient& YdbSchemeClient;
    NYdbHelpers::TTablePath YdbPath;

    NYdbConfig::TConfig& Config;

    bool Built = false;
    bool& DropLatest;
};

int DoMain(int argc, char* argv[]) {
    TString database;
    TString endpoint;
    TString suffix;

    TString ytKinopoiskDir;
    TString ytOttDir;

    auto opts = NLastGetopt::TOpts::Default();
    opts.AddLongOption("database", "YDB database").StoreResult(&database).RequiredArgument("YDB_PATH").Required();
    opts.AddLongOption("endpoint", "YDB endpoint").StoreResult(&endpoint).RequiredArgument("YDB_ENDPOINT").Required();
    opts.AddLongOption("suffix", "YDB table suffix.").StoreResult(&suffix).RequiredArgument("YDB_SUFFIX").Required();

    opts.AddLongOption("yt-kinopoisk-dir", "Path to the kinopoisk YT dir.")
        .StoreResult(&ytKinopoiskDir)
        .RequiredArgument("YT_DIR")
        .DefaultValue("//home/kinopoisk/ext/production/");
    opts.AddLongOption("yt-ott-dir", "Path to the OTT YT dir.")
        .StoreResult(&ytOttDir)
        .RequiredArgument("OTT_DIR")
        .DefaultValue("//home/ott/ext/production/");

    NLastGetopt::TOptsParseResult result(&opts, argc, argv);

    const auto ytPath = NYT::JoinYPaths(ytOttDir, TABLE_YASTATION_ONLINES);

    auto ytClient = NYT::CreateClient(YT_PROXY);
    if (!ytClient) {
        LOG(ERR) << "Failed to create YT client" << Endl;
        return EXIT_FAILURE;
    }

    const auto token = GetEnv(YDB_TOKEN);
    if (!token) {
        LOG(ERR) << "Please, set env variable " << YDB_TOKEN << Endl;
        return EXIT_FAILURE;
    }

    const auto config = NYdb::TDriverConfig().SetEndpoint(endpoint).SetDatabase(database).SetAuthToken(token);
    NYdb::TDriver driver(config);
    NYdb::NTable::TTableClient ydbTableClient(driver);
    NYdb::NScheme::TSchemeClient ydbSchemeClient(driver);

    NYdbConfig::TConfig ydbConfig(ydbTableClient, database);

    if (!ydbConfig.Exists().IsSuccess()) {
        // This call may fail, for example if config is created
        // concurrently. So it's okay to skip errors here.
        ydbConfig.Create();
    }

    const auto filmsInfo = DownloadFilmsInfo(ytClient, ytKinopoiskDir);

    // This guard drops created tables in case of any failure.
    bool dropLatest = true;

    TSVODBuilder<NVideoContent::NProtos::TYDBKinopoiskSVODRowV2> builderV2(
        ytClient, ytPath, ydbTableClient, ydbSchemeClient, ydbConfig, database, suffix, dropLatest);

    builderV2.Build(filmsInfo);

    // Okay, all important transactions have been completed. It's safe
    // to cancel latest table creation.
    dropLatest = false;

    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        return DoMain(argc, argv);
    } catch (const yexception& e) {
        LOG(ERR) << "Exception: " << e.what() << Endl;
        return EXIT_FAILURE;
    }
}
