#include <alice/hollywood/library/scenarios/music/cache/common.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic::NCache {

Y_UNIT_TEST_SUITE(CommonTest) {

Y_UNIT_TEST(Hash) {
    TInputCacheMeta meta;
    THttpRequestInfo info;

    constexpr size_t hashValue = 15448026511483288311LLU;

    info.HttpRequest.SetPath("/users/77777777/likes/tracks");
    UNIT_ASSERT_VALUES_EQUAL(CalculateHash(meta, info), hashValue);

    // query parameters don't affect hash value
    info.HttpRequest.SetPath("/users/77777777/likes/tracks?__uid=77777777");
    UNIT_ASSERT_VALUES_EQUAL(CalculateHash(meta, info), hashValue);
}

}

} // namespace NAlice::NHollywood::NMusic::NCache
