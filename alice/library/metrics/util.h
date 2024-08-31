#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NMetrics {

inline const TVector<double> TIME_INTERVALS = {
    1, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100,
    110, 120, 130, 140, 150, 160, 170, 180, 190, 200,
    250, 300, 350, 400, 450, 500, 600, 700, 800, 900, 1000,
    1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000,
    3000, 4000, 5000, 6000, 10'000, 100'000, 1'000'000
};

inline const TVector<double> SMALL_SIZE_INTERVALS = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    30, 40, 50, 60, 70, 80, 90, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 5000, 10000
};

inline const TVector<double> SIZE_INTERVALS = {
    100, 200, 300, 400, 500, 600, 700, 800, 900,
    1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
    10'000, 11'000, 12'000, 13'000, 14'000, 15'000, 16'000, 17'000, 18'000, 19'000,
    20'000, 30'000, 50'000, 100'000, 200'000, 500'000, 1'000'000
};

/**
 * Golovan does not accept repeated '__', ony one '_' at a time is allowed.
 * It chops first and last all '_' and converts all s/_+/_/g.
 */
TString NormalizeSensorNameForGolovan(TStringBuf name);

} // namespace NAlice::NMetrics
