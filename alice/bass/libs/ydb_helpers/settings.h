#pragma once

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

namespace NYdbHelpers {

const NYdb::NTable::TRetryOperationSettings& DefaultYdbRetrySettings();

inline NYdb::TStatus RetrySyncWithDefaultSettings(NYdb::NTable::TTableClient& tableClient,
                                                  const NYdb::NTable::TTableClient::TOperationSyncFunc& operation) {
    return tableClient.RetryOperationSync(operation, DefaultYdbRetrySettings());
}

} // namespace NYdbHelpers
