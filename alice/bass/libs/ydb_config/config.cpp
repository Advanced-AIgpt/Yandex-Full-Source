#include "config.h"

#include <util/generic/string.h>

namespace {
const TString TABLE_NAME{"config"};
} // namespace

namespace NYdbConfig {

TConfig::TConfig(NYdb::NTable::TTableClient& client, const NYdbHelpers::TPath& database,
                 const NYdb::NTable::TRetryOperationSettings& settings)
    : NYdbKV::TKV(client, NYdbHelpers::TTablePath{database, TABLE_NAME}, settings) {
}
} // namespace NYdbConfig
