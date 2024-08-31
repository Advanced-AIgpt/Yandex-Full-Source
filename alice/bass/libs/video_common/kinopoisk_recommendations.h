#pragma once

#include "defs.h"

#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/table.h>
#include <alice/bass/libs/ydb_helpers/settings.h>
#include <alice/bass/util/generic_error.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>

#include <library/cpp/timezone_conversion/civil.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>

#include <utility>

namespace NVideoContent::NProtos {
class TYDBKinopoiskSVODRow;
} // namespace NVideoContent::NProtos

namespace NVideoCommon {
struct TKinopoiskFilmInfo {
    void ToVideoItem(TVideoItemScheme item) const;

    bool AlmostEqual(const TKinopoiskFilmInfo& rhs) const {
        constexpr double kEps = 1e-3;

        return Id == rhs.Id && Abs(Rating - rhs.Rating) < kEps && Genres == rhs.Genres && Countries == rhs.Countries &&
               ContentType == rhs.ContentType && ReleaseDate == rhs.ReleaseDate;
    }

    static TString Ser(const NDatetime::TCivilDay& day);
    static NDatetime::TCivilDay Des(const TString& date);

    void Ser(NVideoContent::NProtos::TYDBKinopoiskSVODRowV2& row) const;
    void Des(const NVideoContent::NProtos::TYDBKinopoiskSVODRowV2& row);

    TString Id;
    double Rating = 0;
    TVector<EVideoGenre> Genres;
    TVector<TString> Countries;
    EContentType ContentType = EContentType::Null;
    TMaybe<NDatetime::TCivilDay> ReleaseDate;
};

class TKinopoiskFilmsInfosStatus : public NYdb::TStatus {
public:
    template <typename TStatus>
    explicit TKinopoiskFilmsInfosStatus(TStatus&& status)
        : NYdb::TStatus(std::forward<TStatus>(status)) {
    }

    template <typename TStatus, typename TInfos>
    TKinopoiskFilmsInfosStatus(TStatus&& status, TInfos&& infos)
        : NYdb::TStatus(std::forward<TStatus>(status))
        , Infos(std::forward<TInfos>(infos)) {
    }

    const TVector<TKinopoiskFilmInfo>& GetInfos() const {
        return Infos;
    }

    TVector<TKinopoiskFilmInfo>&& StealInfos() {
        return std::move(Infos);
    }

private:
    TVector<TKinopoiskFilmInfo> Infos;
};

// Throws TBadArgumentException in case of bad data.
TKinopoiskFilmsInfosStatus
GetFilmsInfos(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
              const NYdb::NTable::TRetryOperationSettings& settings = NYdb::NTable::TRetryOperationSettings{});
} // namespace NVideoCommon
