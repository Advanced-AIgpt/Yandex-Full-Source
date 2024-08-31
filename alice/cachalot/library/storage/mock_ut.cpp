#include <alice/cachalot/library/storage/mock.h>
#include <alice/cachalot/library/modules/cache/storage.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(CachalotStorageMock) {

Y_UNIT_TEST(SetGet) {
    NCachalot::TYdbSettings settings;
    settings.SetIsFake(true);
    auto storage = NCachalot::MakeSimpleYdbStorage(settings, NCachalot::TCacheYdbOperationSettings());

    storage->Set("aaa", "AAA");

    {
        auto future = storage->GetSingleRow("aaa");
        auto result = future.GetValue();
        UNIT_ASSERT(result.Status == NCachalot::EResponseStatus::OK);
        UNIT_ASSERT(result.Data);
        UNIT_ASSERT_VALUES_EQUAL(result.Data.GetRef(), "AAA");
    }

    {
        auto future = storage->Get("aaa");
        auto result = future.GetValue();
        UNIT_ASSERT(result.Status == NCachalot::EResponseStatus::OK);
        UNIT_ASSERT_VALUES_EQUAL(result.Rows.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(result.Rows.at(0), "AAA");
    }
}


Y_UNIT_TEST(SetGetWithTtl) {
    NCachalot::TYdbSettings settings;
    settings.SetIsFake(true);
    auto storage = NCachalot::MakeSimpleYdbStorage(settings, NCachalot::TCacheYdbOperationSettings());

    storage->Set("bbb", "BBB", /*ttlSeconds = */ 2);

    {
        auto future = storage->GetSingleRow("bbb");
        auto result = future.GetValue();
        UNIT_ASSERT(result.Status == NCachalot::EResponseStatus::OK);
        UNIT_ASSERT(result.Data);
        UNIT_ASSERT_VALUES_EQUAL(result.Data.GetRef(), "BBB");
    }

    Sleep(TDuration::Seconds(1));

    {
        auto future = storage->GetSingleRow("bbb");
        auto result = future.GetValue();
        UNIT_ASSERT(result.Status == NCachalot::EResponseStatus::OK);
        UNIT_ASSERT(result.Data);
        UNIT_ASSERT_VALUES_EQUAL(result.Data.GetRef(), "BBB");
    }

    Sleep(TDuration::Seconds(2));

    {
        auto future = storage->GetSingleRow("bbb");
        auto result = future.GetValue();
        UNIT_ASSERT(result.Status == NCachalot::EResponseStatus::NO_CONTENT);
        UNIT_ASSERT(!result.Data);
    }
}

Y_UNIT_TEST(SetDelGet) {
    NCachalot::TYdbSettings settings;
    settings.SetIsFake(true);
    auto storage = NCachalot::MakeSimpleYdbStorage(settings, NCachalot::TCacheYdbOperationSettings());

    storage->Set("ccc", "CCC");

    {
        auto future = storage->GetSingleRow("ccc");
        auto result = future.GetValue();
        UNIT_ASSERT(result.Status == NCachalot::EResponseStatus::OK);
        UNIT_ASSERT(result.Data);
        UNIT_ASSERT_VALUES_EQUAL(result.Data.GetRef(), "CCC");
    }

    storage->Del("ccc");

    {
        auto future = storage->GetSingleRow("ccc");
        auto result = future.GetValue();
        UNIT_ASSERT(result.Status == NCachalot::EResponseStatus::NO_CONTENT);
        UNIT_ASSERT(!result.Data);
    }
}

};
