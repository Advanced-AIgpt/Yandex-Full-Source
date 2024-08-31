#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NMetricsAggregator {

const static TString BATCH_PATH = "/batch";

inline const TVector<double> BUCKETS = {
    1, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55,
    60, 70, 80, 90, 100,
    120, 140, 160, 180, 200,
    250, 300, 350, 400, 450, 500, 550,
    600, 700, 800, 900, 1000,
    1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000,
    3000, 4000, 5000, 6000, 10'000
};

} // namespace NMetricsAggregator
