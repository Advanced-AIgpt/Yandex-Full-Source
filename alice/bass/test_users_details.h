#pragma once

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/ydb_helpers/visitors.h>
#include <alice/bass/libs/ydb_helpers/table.h>

#include <library/cpp/http/misc/httpcodes.h>

#include <util/generic/variant.h>

namespace NBASS {
namespace NTestUsersDetails {

class TUserManager {
public:
    struct TErrorResult {
        TErrorResult(HttpCodes httpCode, TStringBuf msg);
        TErrorResult(HttpCodes httpCode, TStringBuf type, TStringBuf msg, TStringBuf request);

        bool operator==(const TErrorResult& rhs) const {
            return HttpCode == rhs.HttpCode && Json == rhs.Json;
        }

        HttpCodes HttpCode;
        NSc::TValue Json;
    };
    using TSuccessResult = NSc::TValue;
    using TResult = std::variant<TSuccessResult, TErrorResult>;

public:
    explicit TUserManager(TConfig::TYdbScheme config);

    TResult GetUser(const NSc::TValue& tags, ui64 now, ui64 releaseAt);
    TResult ReleaseUser(TStringBuf login);

private:
    const NYdb::TDriver YDbDriver;
    NYdb::NTable::TTableClient YDbClient;
};

} // namespace NTestUsersDetails
} // namespace NBASS
