#include "common.h"

#include <alice/nlu/libs/fst/fst_custom_hierarchy.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NTestHelpers;

Y_UNIT_TEST_SUITE(TFstFioTests) {

    static const TTestCaseRunner<TFstCustomHierarchy>& T() {
        static const TTestCaseRunner<TFstCustomHierarchy> T{"FIO", LANG_RUS};
        return T;
    }

    static TEntity CE(size_t begin, size_t end, TStringBuf value) {
        return CreateEntity(begin, end, "FIO.NAME", value);
    }

    Y_UNIT_TEST(ParseEmpty) {
        const TTestCase testCases[] = {
            {"", {}}
        };
        T().Run(testCases);
    }

    Y_UNIT_TEST(Name) {
        const TVector<TEntity> result = {CE(1, 2, "маша")};
        const TTestCase testCases[] = {
            {"просто маша", result},
            {"просто машу", result},
            {"просто машей", result}
        };
        T().Run(testCases, "FIO.NAME");
    }

} // test suite
