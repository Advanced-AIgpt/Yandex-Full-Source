#include "kinopoisk_recommendations.h"

#include <alice/bass/libs/video_content/protos/rows.pb.h>
#include <alice/bass/libs/ydb_helpers/queries.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <contrib/libs/cctz/include/cctz/civil_time_detail.h>

#include <util/stream/output.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <util/system/yassert.h>

#include <cstdio>

namespace NVideoCommon {
namespace {
constexpr char YYYY_MM_DD_FORMAT[] = "%04d-%02d-%02d";
constexpr TStringBuf DELIM = ",";

template <typename TCont>
TString Join(const TCont& cont) {
    return JoinSeq(DELIM, cont);
}

template <typename TFn>
void ForEachToken(TStringBuf line, TFn&& fn) {
    TStringBuf token;
    while (line.NextTok(DELIM, token))
        fn(token);
}
} // namespace

// TKinopoiskFilmInfo ----------------------------------------------------------
// static
TString TKinopoiskFilmInfo::Ser(const NDatetime::TCivilDay& day) {
    return Sprintf(YYYY_MM_DD_FORMAT, static_cast<int>(day.year()), static_cast<int>(day.month()),
                   static_cast<int>(day.day()));
}

// static
NDatetime::TCivilDay TKinopoiskFilmInfo::Des(const TString& date) {
    int year;
    int month;
    int day;

    if (sscanf(date.c_str(), YYYY_MM_DD_FORMAT, &year, &month, &day) != 3)
        ythrow TBadArgumentException() << "Input string is not in the YYYY-MM-DD format: " << date;
    if (month < 1 || month > 12)
        ythrow TBadArgumentException() << "Invalid month value: " << month;
    if (day < 1 || day > cctz::detail::impl::days_per_month(year, month)) {
        ythrow TBadArgumentException() << "Invalid day value: " << day << " for year: " << year
                                       << " and month: " << month;
    }
    return NDatetime::TCivilDay{year, month, day};
}

void TKinopoiskFilmInfo::ToVideoItem(TVideoItemScheme item) const {
    item.ProviderName() = PROVIDER_KINOPOISK;
    item.MiscIds().Kinopoisk() = Id;
}

void TKinopoiskFilmInfo::Ser(NVideoContent::NProtos::TYDBKinopoiskSVODRowV2& row) const {
    row.SetKinopoiskId(Id);
    row.SetKinopoiskRating(Rating);
    row.SetGenres(Join(Genres));
    row.SetCountries(Join(Countries));
    if (ContentType != EContentType::Null)
        row.SetContentType(ToString(ContentType));
    if (ReleaseDate)
        row.SetReleaseDate(Ser(*ReleaseDate));
}

void TKinopoiskFilmInfo::Des(const NVideoContent::NProtos::TYDBKinopoiskSVODRowV2& row) {
    TKinopoiskFilmInfo info;

    info.Id = row.GetKinopoiskId();
    info.Rating = row.GetKinopoiskRating();
    ForEachToken(row.GetGenres(), [&info](TStringBuf token) {
        EVideoGenre genre;
        if (!TryFromString<EVideoGenre>(token, genre))
            ythrow TBadArgumentException() << "Can't parse genre from: " << token;
        info.Genres.push_back(genre);
    });
    ForEachToken(row.GetCountries(), [&info](TStringBuf country) { info.Countries.push_back(TString{country}); });
    if (row.HasContentType()) {
        if (!TryFromString<EContentType>(row.GetContentType(), info.ContentType))
            ythrow TBadArgumentException() << "Can't parse content type from: " << row.GetContentType();
    }

    if (row.HasReleaseDate())
        info.ReleaseDate = Des(row.GetReleaseDate());

    *this = std::move(info);
}

// -----------------------------------------------------------------------------
TKinopoiskFilmsInfosStatus GetFilmsInfos(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
                                         const NYdb::NTable::TRetryOperationSettings& settings) {
    const TString queryHead = Sprintf(R"(
                                      PRAGMA TablePathPrefix("%s");
                                      SELECT * FROM [%s] ORDER BY KinopoiskId;
                                      )",
                                      path.Database.c_str(), path.Name.c_str());
    const TString queryTail = Sprintf(R"(
                                      PRAGMA TablePathPrefix("%s");
                                      DECLARE $lastKinopoiskId AS String;
                                      SELECT * FROM [%s] WHERE KinopoiskId > $lastKinopoiskId ORDER BY KinopoiskId;
                                      )",
                                      path.Database.c_str(), path.Name.c_str());

    TVector<TKinopoiskFilmInfo> infos;

    auto append = [&](const NVideoContent::NProtos::TYDBKinopoiskSVODRowV2& row) {
        TKinopoiskFilmInfo info;
        info.Des(row);
        infos.push_back(std::move(info));
    };

    auto status = client.RetryOperationSync(
        [&](NYdb::NTable::TSession session) -> NYdb::TStatus {
            NYdbHelpers::TPreparedQuery qh(queryHead, session);
            if (!qh.IsPrepared())
                return qh.GetPrepareResult();

            auto result = qh.Execute(NYdbHelpers::SerializableRW());
            if (!result.IsSuccess())
                return result;

            Y_ASSERT(result.GetResultSets().size() == 1);
            size_t read = NYdbHelpers::Deserialize<NVideoContent::NProtos::TYDBKinopoiskSVODRowV2>(
                result.GetResultSet(0), append);

            NYdbHelpers::TPreparedQuery qt(queryTail, session);
            if (!qt.IsPrepared())
                return qt.GetPrepareResult();

            while (read != 0) {
                Y_ASSERT(!infos.empty());
                result = qt.Execute(NYdbHelpers::SerializableRW(), [&](NYdb::NTable::TSession& session) {
                    return session.GetParamsBuilder()
                        .AddParam("$lastKinopoiskId")
                        .String(infos.back().Id)
                        .Build()
                        .Build();
                });
                if (!result.IsSuccess())
                    return result;

                Y_ASSERT(result.GetResultSets().size() == 1);
                read = NYdbHelpers::Deserialize<NVideoContent::NProtos::TYDBKinopoiskSVODRowV2>(result.GetResultSet(0),
                                                                                                append);
            }

            return result;
        },
        settings);

    return TKinopoiskFilmsInfosStatus{std::move(status), std::move(infos)};
}
} //  namespace NVideoCommon

template <>
void Out<NVideoCommon::TKinopoiskFilmInfo>(IOutputStream& os, const NVideoCommon::TKinopoiskFilmInfo& info) {
    os << "TKinopoiskFilmInfo [";
    os << info.Id << ", ";
    os << info.Rating << ", ";
    os << "[" << JoinSeq(", ", info.Genres) << "], ";
    os << "[" << JoinSeq(", ", info.Countries) << "], ";
    os << info.ContentType;
    os << "]";
}
