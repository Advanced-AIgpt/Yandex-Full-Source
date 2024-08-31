#include "experiments.h"

#include <library/cpp/testing/unittest/registar.h>

namespace {

THashMap<TString, TMaybe<TString>> EXPERIMENTS_HASHMAP = {
    {
        "exp_1",
        Nothing()
    },
    {
        "exp_2=expval_2",
        Nothing()
    },
    {
        "exp_3",
        "expval_3"
    }
};

} // namespace

Y_UNIT_TEST_SUITE(ExperimentsTestSuite) {
    Y_UNIT_TEST(HasExpTest) {
        UNIT_ASSERT(NAlice::HasExpFlag(EXPERIMENTS_HASHMAP, "exp_1"));
    }

    Y_UNIT_TEST(GetExperimentValueWithPrefixTest) {
        auto flagValue = NAlice::GetExperimentValueWithPrefix(EXPERIMENTS_HASHMAP, "exp_2=");
        UNIT_ASSERT(flagValue.Defined());
        UNIT_ASSERT_EQUAL_C(*flagValue, "expval_2", *flagValue);
    }

    Y_UNIT_TEST(GetExperimentValue) {
        auto flagValue = NAlice::GetExperimentValue(EXPERIMENTS_HASHMAP, "exp_3");
        UNIT_ASSERT(flagValue.Defined());
        UNIT_ASSERT_EQUAL_C(*flagValue, "expval_3", *flagValue);
    }
}
