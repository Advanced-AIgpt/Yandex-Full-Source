#pragma once

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>

#include <util/generic/string.h>
#include <util/generic/yexception.h>

namespace NYdbHelpers {

class TYdbErrorException : public yexception {
public:
    explicit TYdbErrorException(const NYdb::TStatus& status);

    // std::exception overrides:
    const char* what() const noexcept override;

    NYdb::TStatus Status;
    TString Message;
};

NYdb::TStatus ThrowOnError(const NYdb::TStatus& status);
TString StatusToString(const NYdb::TStatus& status);

} // namespace NYdbHelpers
