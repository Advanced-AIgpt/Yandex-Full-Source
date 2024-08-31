#include "queries.h"

#include "table.h"

namespace NYdbHelpers {

TQueryOptions::TQueryOptions()
    : TxControl{SerializableRW()} {
}

TQueryOptions::TQueryOptions(NYdb::NTable::TTxControl txControl)
    : TxControl{txControl} {
}

TQueryOptions::TQueryOptions(NYdb::NTable::TRetryOperationSettings retryPolicy)
    : TxControl{SerializableRW()}
    , RetryPolicy{std::move(retryPolicy)} {
}

TQueryOptions::TQueryOptions(const NYdb::NTable::TTxControl& txControl,
                             const NYdb::NTable::TRetryOperationSettings& retryPolicy)
    : TxControl{txControl}
    , RetryPolicy{retryPolicy} {
}

} // namespace NYdbHelpers
