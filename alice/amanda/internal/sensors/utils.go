package sensors

import (
	"time"

	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

var (
	_defaultDurationBuckets = metrics.MakeExponentialDurationBuckets(
		time.Millisecond, 1.21, 50,
	)
	_defaultBuckets = metrics.NewBuckets(
		100, 200, 300, 400, 500, 600, 700, 800, 900,
		1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
		10000, 11000, 12000, 13000, 14000, 15000, 16000, 17000, 18000, 19000,
		20000, 30000, 50000, 100000, 200000, 500000, 1000000,
	)
)

func PushServiceMetrics(registry metrics.Registry) (onFinish func(err error)) {
	MakeRatedCounter(registry, RequestsCountPerSecond).Inc()
	start := time.Now()
	return func(err error) {
		MakeDurationHistogram(registry, ResponseTimeSeconds).RecordDuration(time.Since(start))
		if err != nil {
			MakeRatedCounter(registry, ErrorsCountPerSecond).Inc()
		}
	}
}

func AddServiceTag(registry metrics.Registry, service string) metrics.Registry {
	return registry.WithTags(map[string]string{
		ServiceName: service,
	})
}

func MakeRatedCounter(registry metrics.Registry, name string) metrics.Counter {
	counter := registry.Counter(name)
	solomon.Rated(counter)
	return counter
}

func MakeDurationHistogram(registry metrics.Registry, name string, buckets ...metrics.DurationBuckets) metrics.Timer {
	buckets = append(buckets, _defaultDurationBuckets)
	hist := registry.DurationHistogram(name, buckets[0])
	solomon.Rated(hist)
	return hist
}

func MakeSizeHistogram(registry metrics.Registry, name string, buckets ...metrics.Buckets) metrics.Histogram {
	buckets = append(buckets, _defaultBuckets)
	hist := registry.Histogram(name, buckets[0])
	solomon.Rated(hist)
	return hist
}
