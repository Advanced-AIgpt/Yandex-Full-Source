#pragma once

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

class TConfig;

namespace NBASS {
NYdb::TDriver ConstructYdbDriver(const TConfig& config);
} // namespace NBASS
