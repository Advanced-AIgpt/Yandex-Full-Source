#pragma once

#include <alice/cachalot/library/config/application.cfgproto.pb.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>


namespace NCachalot {


/**
 *  @brief ydb storage base
 */
class TYdbContext : public TThrRefBase {
public:
    TYdbContext(const NCachalot::TYdbSettings& settings);

    virtual ~TYdbContext();

    inline NYdb::NTable::TTableClient* GetClient() {
        return Client.Get();
    }

    const NCachalot::TYdbSettings& GetSettings() const;

private:
    void InitDriver();

    TString GetAuthToken();

protected:
    NCachalot::TYdbSettings Settings;
    THolder<NYdb::TDriver> Driver;
    THolder<NYdb::NTable::TTableClient> Client;
};
}   // namespace NCachalot
