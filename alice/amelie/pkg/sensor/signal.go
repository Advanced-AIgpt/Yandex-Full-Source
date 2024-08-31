package sensor

import (
	"time"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

type RouteSignal struct {
	r1xx              metrics.Counter
	r2xx              metrics.Counter
	r3xx              metrics.Counter
	r4xx              metrics.Counter
	r5xx              metrics.Counter
	requests          metrics.Counter
	fails             metrics.Counter
	filtered          metrics.Counter
	canceled          metrics.Counter
	durationHistogram metrics.Timer
}

var (
	DefaultTimePolicy = quasarmetrics.ExponentialDurationBuckets(1.21, time.Millisecond, 50)
)

func withStatusCode(registry metrics.Registry, code string) metrics.Registry {
	return registry.WithTags(map[string]string{"status_code": code})
}

func NewRatedCounter(registry metrics.Registry, name string) metrics.Counter {
	counter := registry.Counter(name)
	solomon.Rated(counter)
	return counter
}

func newResponseCodes(registry metrics.Registry, code string) metrics.Counter {
	return NewRatedCounter(withStatusCode(registry, code), "response_codes_per_second")
}

func NewRouteSignal(registry metrics.Registry) *RouteSignal {
	signal := &RouteSignal{
		r1xx:              newResponseCodes(registry, "1xx"),
		r2xx:              newResponseCodes(registry, "2xx"),
		r3xx:              newResponseCodes(registry, "3xx"),
		r4xx:              newResponseCodes(registry, "4xx"),
		r5xx:              newResponseCodes(registry, "5xx"),
		requests:          NewRatedCounter(registry, "requests_per_second"),
		fails:             NewRatedCounter(registry, "fails_per_second"),
		canceled:          NewRatedCounter(registry, "cancels_per_second"),
		filtered:          NewRatedCounter(registry, "filtered_requests_per_seconds"),
		durationHistogram: NewRatedHist(registry, "response_time_seconds", DefaultTimePolicy()),
	}
	return signal
}

func NewRatedHist(registry metrics.Registry, name string, buckets metrics.DurationBuckets) metrics.Timer {
	hist := registry.DurationHistogram(name, buckets)
	solomon.Rated(hist)
	return hist
}

func (r *RouteSignal) Increment1xx() {
	r.r1xx.Inc()
}

func (r *RouteSignal) Increment2xx() {
	r.r2xx.Inc()
}

func (r *RouteSignal) Increment3xx() {
	r.r3xx.Inc()
}

func (r *RouteSignal) Increment4xx() {
	r.r4xx.Inc()
}

func (r *RouteSignal) Increment5xx() {
	r.r5xx.Inc()
}

func (r *RouteSignal) IncrementCanceled() {
	r.canceled.Inc()
}

func (r *RouteSignal) IncrementFails() {
	r.fails.Inc()
}

func (r *RouteSignal) IncrementFiltered() {
	r.filtered.Inc()
}

func (r *RouteSignal) RecordDuration(value time.Duration) {
	r.durationHistogram.RecordDuration(value)
}

func (r *RouteSignal) IncrementTotal() {
	r.requests.Inc()
}
