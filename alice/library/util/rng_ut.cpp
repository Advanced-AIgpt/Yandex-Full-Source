#include <alice/library/util/rng.h>

#include <library/cpp/statistics/statistics.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/digest/multi.h>

#include <cmath>

using namespace NAlice;

Y_UNIT_TEST_SUITE(NlgRng) {
    Y_UNIT_TEST(Integer) {
        ui64 randomValue = 0;
        TFakeRng rng(TFakeRng::TIntegerTag{}, [&randomValue] { return randomValue++; });

        THashSet<i64> actual;

        // 100 iterations is just a big enough number for the remainder op
        // to cycle several times over
        for (size_t i = 0; i < 100; ++i) {
            actual.insert(rng.RandomInteger(-5, 2));
        }

        THashSet<i64> expected = {-5, -4, -3, -2, -1, 0, 1};
        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }
    Y_UNIT_TEST(Integer2) {
        const int ITERATION_COUNT = 1000;
        TRng rng(12345);
        int values[ITERATION_COUNT];
        for (int i = 0; i < ITERATION_COUNT; i++) {
            values[i] = rng.RandomInteger(10, 100);
            UNIT_ASSERT(values[i] >= 10 and values[i] <= 100);
        }
        // Restart and check again
        rng = TRng{12345};
        for (int i = 0; i < ITERATION_COUNT; i++) {
            const auto value = rng.RandomInteger(10, 100);
            UNIT_ASSERT(value == values[i]);
        }
    }

    Y_UNIT_TEST(Integer3) {
        const TString scenario = "random_number";
        const TString node = "run";

        size_t seed = MultiHash(3258070966751497724UL, scenario, node);

        TRng rng{3258070966751497724UL};
        rng = TRng{seed};
        const auto value = rng.RandomInteger(1, 101);

        UNIT_ASSERT_VALUES_EQUAL(seed, 16318422770561496682UL);
        UNIT_ASSERT_VALUES_EQUAL(value, 93);
    }

    Y_UNIT_TEST(Double) {
        constexpr size_t numSamples = 1000;
        ui64 randomValue = 0;
        TFakeRng rng(TFakeRng::TDoubleTag{},
                     [&randomValue] { return randomValue++ / static_cast<double>(numSamples); });

        // expected histogram, sample counts
        TVector<double> expectedHist(Reserve(numSamples));
        for (size_t i = 0; i < numSamples; ++i) {
            expectedHist.push_back(1);
        }

        // actual samples (sorted)
        TVector<double> actualSamples(Reserve(numSamples));
        for (size_t i = 0; i < numSamples; ++i) {
            actualSamples.push_back(rng.RandomDouble(-0.5, 1.5));
        }
        Sort(actualSamples);

        // actual histogram, sample counts
        TVector<double> actualHist(Reserve(numSamples));
        auto current = actualSamples.begin();
        for (size_t i = 0; i < numSamples; ++i) {
            // given the bucket, count the number of samples in it
            double nextBucketBoundary = -0.5 + 2.0 * i / numSamples;
            auto next = std::find_if(current, actualSamples.end(),
                                     [nextBucketBoundary](double sample) { return sample >= nextBucketBoundary; });
            actualHist.push_back(next - current);
            current = next;
        }

        // Kolmogorov-Smirnov test with alpha = 0.1
        // https://en.wikipedia.org/wiki/Kolmogorov–Smirnov_test#Two-sample_Kolmogorov–Smirnov_test
        //
        // This version of the test works with histograms:
        // - same number of buckets
        // - any total number of samples
        double stat = NStatistics::KolmogorovSmirnovHistogramStatistics(expectedHist.begin(), expectedHist.end(),
                                                                        actualHist.begin(), actualHist.end());

        // statCrit = c(alpha) * sqrt((N + M) / (N * M)), where:
        // - c(0.1) = 1.073
        // - N and M are the bucket numbers (same in our case)
        double statCrit = 1.073 * std::sqrt(2.0 / numSamples);
        UNIT_ASSERT_C(stat < statCrit, "stat = " << stat << ", statCrit = " << statCrit);
    }
}
