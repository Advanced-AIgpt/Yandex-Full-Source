#include "common.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/ydb_helpers/table.h>

#include <ydb/public/sdk/cpp/client/ydb_scheme/scheme.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/generic/vector.h>
#include <util/system/env.h>

#include <algorithm>
#include <cstdlib>

namespace {
const TStringBuf YDB_TOKEN_ENV = "YDB_TOKEN";

template <typename TList, typename TDrop>
bool DropOld(TStringBuf prefix, TStringBuf latest, size_t keep, const TList& list, const TDrop& drop) {
    TVector<TString> items;

    const auto status = list([&](const TString& item) {
        if (item == latest)
            return;
        if (!item.StartsWith(prefix))
            return;
        items.push_back(item);
    });
    if (!status.IsSuccess()) {
        LOG(ERR) << "Failed to list items: " << NYdbHelpers::StatusToString(status) << Endl;
        return false;
    }

    if (items.size() <= keep)
        return true;

    std::nth_element(items.begin(), items.begin() + keep, items.end(), std::greater<TString>{});

    bool success = true;
    for (auto it = items.begin() + keep; it != items.end(); ++it) {
        const auto& item = *it;

        try {
            drop(item);
        } catch (const yexception& e) {
            LOG(ERR) << "Failed to drop item " << item << ": " << e << Endl;
            success = false;
        }
    }

    return success;
}
} // namespace

namespace NVideoContent {

TString GetYDBToken() {
    const auto token = GetEnv(TString{YDB_TOKEN_ENV});
    if (token.empty()) {
        LOG(ERR) << "Please, set env variable " << YDB_TOKEN_ENV << Endl;
        exit(EXIT_FAILURE);
    }
    return token;
}

bool DropOldTables(NYdb::NScheme::TSchemeClient& schemeClient, NYdb::NTable::TTableClient& tableClient,
                   const NYdbHelpers::TPath& database, TStringBuf prefix, TStringBuf latest, size_t keep) {
    return DropOld(prefix, latest, keep,
                   [&](std::function<void(const TString&)> callback) {
                       return NYdbHelpers::ListTables(schemeClient, database, callback);
                   } /* list */,
                   [&](const TString& table) {
                       NYdbHelpers::DropTableOrFail(tableClient, NYdbHelpers::TTablePath{database, table});
                   } /* drop */);
}

bool DropOldDirectoriesWithTables(NYdb::NScheme::TSchemeClient& schemeClient, NYdb::NTable::TTableClient& tableClient,
                                  const NYdbHelpers::TPath& database, TStringBuf prefix, TStringBuf latest,
                                  size_t keep) {
    return DropOld(prefix, latest, keep,
                   [&](std::function<void(const TString&)> callback) {
                       return NYdbHelpers::ListDirectories(schemeClient, database, callback);
                   } /* list */,
                   [&](const TString& directory) {
                       NYdbHelpers::RemoveDirectoryWithTablesOrFail(tableClient, schemeClient,
                                                                    NYdbHelpers::TTablePath{database, directory});
                   } /* drop */);
}
} // namespace NVideoContent
