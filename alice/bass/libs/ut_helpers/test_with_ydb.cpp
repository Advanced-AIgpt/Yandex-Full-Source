#include "test_with_ydb.h"

#include <alice/bass/libs/ydb_helpers/table.h>

#include <util/system/yassert.h>

namespace NVideoCommon {
namespace NTesting {
void TestWithYDB::SetUp() {
    Db.Init();
    Y_ENSURE(Db.Driver);

    TableClient = MakeHolder<NYdb::NTable::TTableClient>(*Db.Driver);
    SchemeClient = MakeHolder<NYdb::NScheme::TSchemeClient>(*Db.Driver);
}

void TestWithYDB::TearDown() {
    TableClient.Reset();
    SchemeClient.Reset();
}
} // namespace NTesting
} // namespace NVideoCommon
