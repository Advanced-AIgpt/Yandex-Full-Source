#include <alice/wonderlogs/library/parsers/internal/utils.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice::NWonderlogs;

Y_UNIT_TEST_SUITE(Utils) {
    Y_UNIT_TEST(NotEmpty) {
        TMaybe<TString> uuid;
        UNIT_ASSERT(!NotEmpty(uuid));
        uuid = "lolkek";
        UNIT_ASSERT(NotEmpty(uuid));
    }
}

} // namespace
