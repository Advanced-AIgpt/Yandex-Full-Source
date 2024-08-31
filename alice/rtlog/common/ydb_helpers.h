#pragma once

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/generic/vector.h>

namespace NRTLog {
    struct TYdbSettings {
        TString Endpoint;
        TString Database;
        TString AuthToken;
    };

    NYdb::TDriverConfig ToDriverConfig(const TYdbSettings& ydbSettings);

    template <typename TContainer>
    TString ReplaceAll(const TString& queryTemplate, const TContainer& parameters) {
        TString result = queryTemplate;
        for (const auto& p: parameters) {
            SubstGlobal(result, std::get<0>(p), std::get<1>(p));
        }
        return result;
    }
}
