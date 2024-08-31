#include "hist.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NSlowestScenario;

namespace {

const TIntervals INTERVALS{0, 2, 4, 8, 16};

Y_UNIT_TEST_SUITE(Hist) {
    Y_UNIT_TEST(Add) {
        THist hist{INTERVALS};
        hist.Add(3);
        hist.Add(4);
        hist.Add(7);
        hist.Add(14);
        hist.Add(15);
        hist.Add(16);
        TVector<size_t> expected{0, 0, 1, 2, 2};
        UNIT_ASSERT_VALUES_EQUAL(hist.Buckets, expected);
    }
    Y_UNIT_TEST(Percentiles) {
        {
            THist hist{INTERVALS};
            hist.Add(1);
            UNIT_ASSERT_DOUBLES_EQUAL(hist.ComputePercentile(0.5), 1, 1e-5);
            UNIT_ASSERT_DOUBLES_EQUAL(hist.ComputePercentile(0.75), 1.5, 1e-5);
            UNIT_ASSERT_DOUBLES_EQUAL(hist.ComputePercentile(0.9), 1.8, 1e-5);
            UNIT_ASSERT_DOUBLES_EQUAL(hist.ComputePercentile(1), 2, 1e-5);
        }
        {
            THist hist{INTERVALS};
            hist.Add(1);
            hist.Add(3);
            UNIT_ASSERT_DOUBLES_EQUAL(hist.ComputePercentile(0.5), 2, 1e-5);
            UNIT_ASSERT_DOUBLES_EQUAL(hist.ComputePercentile(0.75), 3, 1e-5);
            UNIT_ASSERT_DOUBLES_EQUAL(hist.ComputePercentile(0.9), 3.6, 1e-5);
            UNIT_ASSERT_DOUBLES_EQUAL(hist.ComputePercentile(1), 4, 1e-5);
        }
    }


}

} // namespace