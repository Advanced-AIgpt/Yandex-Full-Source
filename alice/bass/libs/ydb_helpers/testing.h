#pragma once

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NYdbHelpers {
struct TLocalDatabase {
    ~TLocalDatabase();

    void Init();

    TString Database;
    TString Endpoint;
    THolder<NYdb::TDriver> Driver;
};
} // namespace NYdbHelpers
