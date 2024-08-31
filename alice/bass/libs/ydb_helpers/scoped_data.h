#pragma once

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

namespace NYdbHelpers {

struct TScopedDriver final {
    explicit TScopedDriver(const NYdb::TDriverConfig& config)
        : Driver{config}
    {
    }

    ~TScopedDriver() {
        Driver.Stop();
    }

    NYdb::TDriver Driver;
};

}
 // namespace NYdbHelpers
