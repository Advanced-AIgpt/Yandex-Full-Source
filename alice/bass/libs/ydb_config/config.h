#pragma once

#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/settings.h>
#include <alice/bass/libs/ydb_kv/kv.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/generic/strbuf.h>

namespace NYdbConfig {

inline constexpr TStringBuf KEY_KINOPOISK_SVOD_LATEST_V1 = "KinopoiskSVODLatest";
inline constexpr TStringBuf KEY_KINOPOISK_SVOD_LATEST_V2 = "KinopoiskSVODLatestV2";
inline constexpr TStringBuf KEY_VIDEO_LATEST = "VideoLatest";

using TValueStatus = NYdbKV::TValueStatus;
using TKeyValuesStatus = NYdbKV::TKeyValuesStatus;

class TConfig : public NYdbKV::TKV {
public:
    TConfig(NYdb::NTable::TTableClient& client, const NYdbHelpers::TPath& database,
            const NYdb::NTable::TRetryOperationSettings& settings = NYdbHelpers::DefaultYdbRetrySettings());
};
} // namespace NYdbConfig
