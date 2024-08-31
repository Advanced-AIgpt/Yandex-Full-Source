#include <alice/library/cachalot_cache/cachalot_cache.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NAppHostServices;

Y_UNIT_TEST_SUITE(TCacheClient) {

    Y_UNIT_TEST(MakeGetRequestTest) {
        NCachalotProtocol::TGetRequest request = TCachalotCache::MakeGetRequest("TEST_KEY", "TEST_STORAGE_TAG");

        UNIT_ASSERT_STRINGS_EQUAL("TEST_KEY", request.GetKey());
        UNIT_ASSERT_STRINGS_EQUAL("TEST_STORAGE_TAG", request.GetStorageTag());
    }

    Y_UNIT_TEST(MakeSetRequestTest) {
        NCachalotProtocol::TSetRequest request = TCachalotCache::MakeSetRequest("TEST_KEY", "TEST_DATA", "TEST_STORAGE_TAG");

        UNIT_ASSERT_STRINGS_EQUAL("TEST_KEY", request.GetKey());
        UNIT_ASSERT_STRINGS_EQUAL("TEST_DATA", request.GetData());
        UNIT_ASSERT_STRINGS_EQUAL("TEST_STORAGE_TAG", request.GetStorageTag());
        UNIT_ASSERT_EQUAL(0, request.GetTTL());
    }

    Y_UNIT_TEST(MakeSetRequestTestWithTtl) {
        NCachalotProtocol::TSetRequest request = TCachalotCache::MakeSetRequest("TEST_KEY", "TEST_DATA", "TEST_STORAGE_TAG", 88005353535);

        UNIT_ASSERT_STRINGS_EQUAL("TEST_KEY", request.GetKey());
        UNIT_ASSERT_STRINGS_EQUAL("TEST_DATA", request.GetData());
        UNIT_ASSERT_STRINGS_EQUAL("TEST_STORAGE_TAG", request.GetStorageTag());
        UNIT_ASSERT_EQUAL(88005353535, request.GetTTL());
    }

};

