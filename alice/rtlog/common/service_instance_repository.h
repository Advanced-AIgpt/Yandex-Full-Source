#pragma once

#include <alice/rtlog/common/ydb_helpers.h>
#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

namespace NRTLog {
    class TServiceInstanceRepository {
    public:
        explicit TServiceInstanceRepository(NYdb::NTable::TTableClient tableClient, const TString& database);

        explicit TServiceInstanceRepository(const TYdbSettings& ydbSettings);

        ui64 Resolve(const NRTLogEvents::TInstanceDescriptor& instance);

    private:
        NYdb::NTable::TTableClient TableClient;
        TString Query;
    };
}
