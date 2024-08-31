#pragma once

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

namespace NAlice::NShooter {

class TYdb {
public:
    TYdb(const TString& endpoint, const TString& database, const TString& authToken);
    ~TYdb();

    TString ObtainConfig(const TString& table, const TString& id);

private:
    const TString& Endpoint_;
    const TString& Database_;
    const TString& AuthToken_;

    NYdb::TDriver Driver_;
};

} // namespace NAlice::NShooter
