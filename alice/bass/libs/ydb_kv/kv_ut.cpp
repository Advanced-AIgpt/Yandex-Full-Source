#include "kv.h"

#include <alice/bass/libs/ydb_kv/protos/kv.pb.h>

#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/table.h>

#include <alice/bass/libs/ydb_helpers/testing.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/system/yassert.h>

using namespace NYdbHelpers;
using namespace NYdbKV;

namespace {
class TYdbKVTest : public NUnitTest::TTestBase {
public:
    void SetUp() override {
        Db.Init();
        UNIT_ASSERT(Db.Driver);

        Client = MakeHolder<NYdb::NTable::TTableClient>(*Db.Driver);
        Path = MakeHolder<TTablePath>(Db.Database, "config");
        KV = MakeHolder<TKV>(*Client, *Path);

        KV->Drop();
    }

    void TearDown() override {
        KV.Reset();
        Path.Reset();
        Client.Reset();
    }

    UNIT_TEST_SUITE(TYdbKVTest);
    UNIT_TEST(Smoke);
    UNIT_TEST(Simple);
    UNIT_TEST(GetAll);
    UNIT_TEST(SetMany);
    UNIT_TEST(BatchedGetAll);
    UNIT_TEST_SUITE_END();

    void Smoke();
    void Simple();
    void GetAll();
    void SetMany();
    void BatchedGetAll();

private:
    TLocalDatabase Db;

    THolder<NYdb::NTable::TTableClient> Client;
    THolder<TTablePath> Path;
    THolder<TKV> KV;
};
UNIT_TEST_SUITE_REGISTRATION(TYdbKVTest);

void TYdbKVTest::Smoke() {
    UNIT_ASSERT(KV);
    auto& kv = *KV;

    UNIT_ASSERT(!kv.Exists().IsSuccess());
    UNIT_ASSERT(!kv.Get("key1").IsSuccess());
    UNIT_ASSERT(!kv.Set("key1", "value1").IsSuccess());
    UNIT_ASSERT(!kv.Delete("key1").IsSuccess());
}

void TYdbKVTest::Simple() {
    UNIT_ASSERT(KV);
    auto& kv = *KV;

    UNIT_ASSERT(!kv.Exists().IsSuccess());
    UNIT_ASSERT(kv.Create().IsSuccess());
    UNIT_ASSERT(kv.Exists().IsSuccess());

    {
        const auto status = kv.Get("key1");
        UNIT_ASSERT(status.IsSuccess());
        UNIT_ASSERT(!status.GetValue().Defined());
    }

    UNIT_ASSERT(kv.Set("key1", "value1").IsSuccess());
    UNIT_ASSERT(kv.Set("key2", "value2").IsSuccess());

    {
        const auto status = kv.Get("key1");
        UNIT_ASSERT(status.IsSuccess());
        UNIT_ASSERT_VALUES_EQUAL(status.GetValue(), "value1");
    }

    {
        const auto status = kv.Get("key2");
        UNIT_ASSERT(status.IsSuccess());
        UNIT_ASSERT_VALUES_EQUAL(status.GetValue(), "value2");
    }

    {
        const auto statusDelete1 = kv.Delete("key1");
        UNIT_ASSERT(statusDelete1.IsSuccess());

        const auto status1 = kv.GetAll();
        ThrowOnError(status1);
        auto kvs = status1.GetKeyValues();

        UNIT_ASSERT_VALUES_EQUAL(kvs.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(kvs[0].GetKey(), "key2");
        UNIT_ASSERT(kvs[0].HasValue());
        UNIT_ASSERT_VALUES_EQUAL(kvs[0].GetValue(), "value2");

        const auto statusDelete2 = kv.Delete("key2");
        UNIT_ASSERT(statusDelete2.IsSuccess());

        const auto status2 = kv.GetAll();
        ThrowOnError(status2);
        kvs = status2.GetKeyValues();

        UNIT_ASSERT_VALUES_EQUAL(kvs.size(), 0);

        const auto statusDelete3 = kv.Delete("key2");
        UNIT_ASSERT(statusDelete3.IsSuccess());
    }
}

void TYdbKVTest::GetAll() {
    UNIT_ASSERT(KV);
    auto& kv = *KV;

    UNIT_ASSERT(!kv.Exists().IsSuccess());

    UNIT_ASSERT(kv.Create().IsSuccess());
    UNIT_ASSERT(kv.Exists().IsSuccess());

    {
        const auto status = kv.GetAll();
        UNIT_ASSERT(status.IsSuccess());
        UNIT_ASSERT(status.GetKeyValues().empty());
    }

    UNIT_ASSERT(kv.Set("key1", "value1").IsSuccess());
    UNIT_ASSERT(kv.Set("key2", "value2").IsSuccess());
    UNIT_ASSERT(kv.Set("key3", "value3").IsSuccess());

    {
        const auto status = kv.GetAll();
        UNIT_ASSERT(status.IsSuccess());

        auto kvs = status.GetKeyValues();
        Sort(kvs, [&](const TKeyValue& lhs, const TKeyValue& rhs) { return lhs.GetKey() < rhs.GetKey(); });

        UNIT_ASSERT_VALUES_EQUAL(kvs.size(), 3);

        for (size_t i = 0; i < kvs.size(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(kvs[i].GetKey(), TStringBuilder() << "key" << (i + 1));
            UNIT_ASSERT(kvs[i].HasValue());
            UNIT_ASSERT_VALUES_EQUAL(kvs[i].GetValue(), TStringBuilder() << "value" << (i + 1));
        }
    }
}

void TYdbKVTest::SetMany() {
    UNIT_ASSERT(KV);
    auto& kv = *KV;

    ThrowOnError(kv.Create());

    ThrowOnError(kv.SetMany({}));

    TKeyValue a;
    a.SetKey("keyA");
    a.SetValue("valueA");

    TKeyValue b;
    b.SetKey("keyB");

    ThrowOnError(kv.SetMany({a, b}));

    const auto status = kv.GetAll();
    ThrowOnError(status);

    auto kvs = status.GetKeyValues();
    Sort(kvs, [&](const TKeyValue& lhs, const TKeyValue& rhs) { return lhs.GetKey() < rhs.GetKey(); });

    UNIT_ASSERT_VALUES_EQUAL(kvs.size(), 2);

    UNIT_ASSERT_VALUES_EQUAL(kvs[0].GetKey(), a.GetKey());
    UNIT_ASSERT(kvs[0].HasValue());
    UNIT_ASSERT_VALUES_EQUAL(kvs[0].GetValue(), a.GetValue());

    UNIT_ASSERT_VALUES_EQUAL(kvs[1].GetKey(), b.GetKey());
    UNIT_ASSERT(!kvs[1].HasValue());
}

void TYdbKVTest::BatchedGetAll() {
    UNIT_ASSERT(KV);
    auto& kv = *KV;

    UNIT_ASSERT(!kv.Exists().IsSuccess());
    ThrowOnError(kv.Create());

    TVector<TKeyValue> kvs;
    for (size_t i = 0; i < 5000; ++i) {
        const TString key = TStringBuilder() << "Key" << i;
        const TString value = TStringBuilder() << "Value" << i;

        kvs.push_back(MakeKeyValue(key, value));
    }

    ThrowOnError(kv.SetMany(kvs));

    const auto status = kv.GetAll();
    ThrowOnError(status);

    const auto actual = status.GetKeyValues();

    Sort(kvs, [&](const TKeyValue& lhs, const TKeyValue& rhs) { return lhs.GetKey() < rhs.GetKey(); });

    UNIT_ASSERT_VALUES_EQUAL(actual.size(), kvs.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        UNIT_ASSERT_VALUES_EQUAL(actual[i].GetKey(), kvs[i].GetKey());
        UNIT_ASSERT_VALUES_EQUAL(actual[i].GetValue(), kvs[i].GetValue());
    }
}
} // namespace
