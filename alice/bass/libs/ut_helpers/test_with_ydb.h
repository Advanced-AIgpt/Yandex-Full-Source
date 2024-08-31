#pragma once

#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/testing.h>

#include <ydb/public/sdk/cpp/client/ydb_scheme/scheme.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NVideoCommon {
namespace NTesting {

// Base class for tests that need a single YDB table.
class TestWithYDB : public NUnitTest::TTestBase {
public:
    // NUnitTest::TTestBase overrides:
    void SetUp() override;
    void TearDown() override;

protected:
    NYdbHelpers::TLocalDatabase Db;
    THolder<NYdb::NTable::TTableClient> TableClient;
    THolder<NYdb::NScheme::TSchemeClient> SchemeClient;
};

} // namespace NTesting
} // namespace NVideoCommon
