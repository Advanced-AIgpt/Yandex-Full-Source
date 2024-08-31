package metrics

import (
	"math"
	"time"

	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type BucketsGenerationPolicy func() metrics.DurationBuckets

func UniformDurationBuckets(bound time.Duration, numOfBins int) BucketsGenerationPolicy {
	return func() metrics.DurationBuckets {
		step := bound / time.Duration(numOfBins)

		buckets := make([]time.Duration, numOfBins)
		buckets[0] = step
		for i := 1; i < numOfBins; i++ {
			buckets[i] = buckets[i-1] + step
		}

		return metrics.NewDurationBuckets(buckets...)
	}
}

func ExponentialDurationBuckets(powBase float64, timeBase time.Duration, numOfBins int) BucketsGenerationPolicy {
	if powBase <= 1 {
		panic(xerrors.Errorf("Invalid powBase: expected powBase > 1, got %f", powBase))
	}
	return func() metrics.DurationBuckets {
		buckets := make([]time.Duration, numOfBins)
		buckets[0] = timeBase
		for i, deg := 1, 1; i < numOfBins; deg++ {
			bucket := time.Duration(math.Pow(powBase, float64(deg))) * timeBase
			if bucket > buckets[i-1] {
				buckets[i] = bucket
				i++
			}
		}
		return metrics.NewDurationBuckets(buckets...)
	}
}

var (
	DefaultExponentialBucketsPolicy = ExponentialDurationBuckets(1.25, time.Millisecond, 50)
)
