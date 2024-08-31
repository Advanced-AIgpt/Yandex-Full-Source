package metrics

import (
	"net/http"
	"strings"
	"time"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

type RouteSignals interface {
	Increment1xx()
	Increment2xx()
	Increment3xx()
	Increment4xx()
	Increment5xx()

	IncrementCanceled()
	IncrementFails()
	IncrementFiltered()

	RecordDuration(value time.Duration)
}

type RouteSignalsWithTotal interface {
	RouteSignals
	IncrementTotal()
}

type routeSignals struct {
	calls1xx metrics.Counter
	calls2xx metrics.Counter
	calls3xx metrics.Counter
	calls4xx metrics.Counter
	calls5xx metrics.Counter

	fails             metrics.Counter
	canceled          metrics.Counter
	filtered          metrics.Counter
	durationHistogram metrics.Timer
}

type routeSignalsWithTotal struct {
	routeSignals
	total metrics.Counter
}

func (rs *routeSignals) Increment1xx() {
	rs.calls1xx.Inc()
}

func (rs *routeSignals) Increment2xx() {
	rs.calls2xx.Inc()
}

func (rs *routeSignals) Increment3xx() {
	rs.calls3xx.Inc()
}

func (rs *routeSignals) Increment4xx() {
	rs.calls4xx.Inc()
}

func (rs *routeSignals) Increment5xx() {
	rs.calls5xx.Inc()
}

func (rs *routeSignals) IncrementFails() {
	rs.fails.Inc()
}

func (rs *routeSignals) IncrementCanceled() {
	rs.canceled.Inc()
}

func (rs *routeSignals) IncrementFiltered() {
	rs.filtered.Inc()
}

func (rs *routeSignals) RecordDuration(value time.Duration) {
	rs.durationHistogram.RecordDuration(value)
}

func (rs *routeSignalsWithTotal) IncrementTotal() {
	rs.total.Inc()
}

func RecordHTTPCode(rs RouteSignals, statusCode int) {
	switch {
	case statusCode < 200:
		rs.Increment1xx()
	case statusCode < 300:
		rs.Increment2xx()
	case statusCode < 400:
		rs.Increment3xx()
	case statusCode < 500:
		rs.Increment4xx()
	case statusCode < 600:
		rs.Increment5xx()
	}
}

func newRouteSignals(registry metrics.Registry, buckets metrics.DurationBuckets) routeSignals {
	signals := routeSignals{
		calls1xx:          registry.WithTags(map[string]string{"http_code": "1xx"}).Counter("calls"),
		calls2xx:          registry.WithTags(map[string]string{"http_code": "2xx"}).Counter("calls"),
		calls3xx:          registry.WithTags(map[string]string{"http_code": "3xx"}).Counter("calls"),
		calls4xx:          registry.WithTags(map[string]string{"http_code": "4xx"}).Counter("calls"),
		calls5xx:          registry.WithTags(map[string]string{"http_code": "5xx"}).Counter("calls"),
		fails:             registry.Counter("fails"),
		canceled:          registry.Counter("canceled"),
		filtered:          registry.Counter("filtered"),
		durationHistogram: registry.DurationHistogram("duration_buckets", buckets),
	}

	solomon.Rated(signals.calls1xx)
	solomon.Rated(signals.calls2xx)
	solomon.Rated(signals.calls3xx)
	solomon.Rated(signals.calls4xx)
	solomon.Rated(signals.calls5xx)
	solomon.Rated(signals.fails)
	solomon.Rated(signals.canceled)
	solomon.Rated(signals.filtered)
	solomon.Rated(signals.durationHistogram)
	return signals
}

func NewRouteSignals(registry metrics.Registry, buckets metrics.DurationBuckets) RouteSignals {
	signals := newRouteSignals(registry, buckets)
	return &signals
}

func newRouteSignalsWithTotal(registry metrics.Registry, buckets metrics.DurationBuckets) routeSignalsWithTotal {
	signals := routeSignalsWithTotal{
		routeSignals: newRouteSignals(registry, buckets),
		total:        registry.Counter("total"),
	}
	solomon.Rated(signals.total)
	return signals
}

func NewRouteSignalsWithTotal(registry metrics.Registry, buckets metrics.DurationBuckets) RouteSignalsWithTotal {
	signals := newRouteSignalsWithTotal(registry, buckets)
	return &signals
}

type ChiRouterRouteSignals map[string]map[string]RouteSignalsWithTotal

func (routeSignals ChiRouterRouteSignals) RegisterRouteSignals(registry metrics.Registry, router chi.Router, policy BucketsGenerationPolicy) error {
	routeWalker := func(method string, route string, handler http.Handler, _ ...func(http.Handler) http.Handler) error {
		if handler == nil {
			return nil
		}
		route = strings.Replace(route, "/*/", "/", -1)
		if _, ok := routeSignals[route]; !ok {
			routeSignals[route] = make(map[string]RouteSignalsWithTotal)
		}
		routeRegistry := registry.WithTags(map[string]string{"http_method": method, "path": route})
		routeSignals[route][method] = NewRouteSignalsWithTotal(routeRegistry, policy())
		return nil
	}
	return chi.Walk(router, routeWalker)
}

func (routeSignals ChiRouterRouteSignals) GetRouteSignals(r *http.Request) RouteSignalsWithTotal {
	routeContext := chi.RouteContext(r.Context())
	if routeContext == nil {
		return nil
	}

	pattern := routeContext.RoutePattern()
	if methods, ok := routeSignals[pattern]; ok {
		if signals, ok := methods[r.Method]; ok {
			return signals
		}
	}
	return nil
}
