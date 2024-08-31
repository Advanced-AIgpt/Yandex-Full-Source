#pragma once

#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/settings.h>
#include <alice/bass/libs/ydb_kv/protos/kv.pb.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NYdbKV {
class TValueStatus : public NYdb::TStatus {
public:
    template <typename TStatus>
    explicit TValueStatus(TStatus&& status)
        : NYdb::TStatus(std::forward<TStatus>(status)) {
    }

    template <typename TStatus>
    TValueStatus(TStatus&& status, const TMaybe<TString>& value)
        : NYdb::TStatus(std::forward<TStatus>(status))
        , Value(value) {
    }

    const TMaybe<TString>& GetValue() const {
        return Value;
    }

private:
    TMaybe<TString> Value;
};

TKeyValue MakeKeyValue(const TString& key, const TMaybe<TString>& value);

class TKeyValuesStatus : public NYdb::TStatus {
public:
    template <typename TStatus>
    explicit TKeyValuesStatus(TStatus&& status)
        : NYdb::TStatus(std::forward<TStatus>(status)) {
    }

    template <typename TStatus, typename TKeyValues>
    TKeyValuesStatus(TStatus&& status, TKeyValues&& keyValues)
        : NYdb::TStatus(std::forward<TStatus>(status))
        , KeyValues(std::forward<TKeyValues>(keyValues)) {
    }

    const TVector<TKeyValue>& GetKeyValues() const {
        return KeyValues;
    }

private:
    TVector<TKeyValue> KeyValues;
};

class TKV {
public:
    TKV(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
        const NYdb::NTable::TRetryOperationSettings& settings = NYdbHelpers::DefaultYdbRetrySettings());

    virtual ~TKV() = default;

    NYdb::TStatus Create();
    NYdb::TStatus Drop();

    NYdb::TStatus Exists();

    TValueStatus Get(TStringBuf key);
    TKeyValuesStatus GetAll();

    NYdb::TStatus Set(TStringBuf key, TStringBuf value);
    NYdb::TStatus SetMany(const TVector<TKeyValue>& keyValues);

    NYdb::TStatus Delete(TStringBuf key);

private:
    NYdb::NTable::TTableClient& Client;
    NYdbHelpers::TTablePath Path;
    NYdb::NTable::TRetryOperationSettings Settings;

    TString GetQuery;
    TString GetAllQuery;
    TString GetAllContQuery;
    TString SetQuery;
    TString SetManyQuery;
    TString DeleteQuery;
};
} // namespace NYdbKV
