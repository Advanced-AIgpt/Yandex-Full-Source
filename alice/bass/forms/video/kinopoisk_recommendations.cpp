#include "kinopoisk_recommendations.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/ydb_helpers/exception.h>
#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/settings.h>

#include <alice/bass/ydb_config.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

namespace NBASS::NVideo {

TDuration TKinopoiskRecommendations::Update(TGlobalContextPtr ctx) {
    LOG(INFO) << "Updating kinopoisk films infos..." << Endl;

    if (!ctx) {
        LOG(ERR) << "Invalid global context" << Endl;
        return GetUpdatePeriod();
    }

    auto& client = ctx->YdbClient();
    const auto database = TString{*ctx->Config().YDb().DataBase()};

    const TMaybe<TString> table = ctx->YdbConfig().Get(NYdbConfig::KEY_KINOPOISK_SVOD_LATEST_V2);
    if (!table) {
        LOG(ERR) << "Failed to get name of the kinopoisk SVOD table" << Endl;
        return GetUpdatePeriod();
    }

    const NYdbHelpers::TTablePath path{database, *table};

    TVector<NVideoCommon::TKinopoiskFilmInfo> infos;

    try {
        auto status = NVideoCommon::GetFilmsInfos(client, path, NYdbHelpers::DefaultYdbRetrySettings());
        if (!status.IsSuccess()) {
            LOG(ERR) << "Failed to get kinopoisk films infos: " << NYdbHelpers::StatusToString(status) << Endl;
            return GetUpdatePeriod();
        }

        infos = status.StealInfos();
    } catch (const TBadArgumentException& e) {
        LOG(ERR) << "Failed to get kinopoisk films infos: " << e.what() << Endl;
        return GetUpdatePeriod();
    }

    LOG(INFO) << "Downloaded " << infos.size() << " films infos" << Endl;

    auto data = MakeAtomicShared<TData>(std::move(infos));

    TWriteGuard guard(&Mutex);
    Data = data;
    return UpdatePeriod;
}

TAtomicSharedPtr<TKinopoiskRecommendations::TData> TKinopoiskRecommendations::GetData() const {
    TReadGuard guard(&Mutex);
    return Data;
}

void TKinopoiskRecommendations::SetUpdatePeriod(const TDuration& updatePeriod) {
    TWriteGuard guard(&Mutex);
    UpdatePeriod = updatePeriod;
}

TDuration TKinopoiskRecommendations::GetUpdatePeriod() {
    TReadGuard guard(&Mutex);
    return UpdatePeriod;
}

} // namespace NBASS::NVideo
