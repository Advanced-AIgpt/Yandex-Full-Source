package libapphost

import (
	"fmt"
	"time"

	libmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

type RouteSignals interface {
	IncrementSuccess()
	IncrementFails()
	RecordDuration(duration time.Duration)
}

type RouterSignalsRepository interface {
	RegisterRouteSignals(path string) RouteSignals
	GetRouteSignals(path string) RouteSignals
}

func NewRouterSignalsRepository(registry metrics.Registry, buckets libmetrics.BucketsGenerationPolicy) RouterSignalsRepository {
	return &routerSignalsRepository{
		signals:  make(map[string]RouteSignals),
		registry: registry,
		buckets:  buckets,
	}
}

func RegisterRouteSignals(signalsRepository RouterSignalsRepository, apphostMux *apphost.ServeMux) error {
	metricsWalker := apphost.WalkFunc(func(path string, handler apphost.Handler) error {
		if handler == nil {
			return nil
		}
		signalsRepository.RegisterRouteSignals(path)
		return nil
	})
	return apphost.Walk(apphostMux, metricsWalker)
}

func MetricsTrackerMiddleware(signals RouterSignalsRepository) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(ctx apphost.Context) error {
			defer func() {
				if r := recover(); r != nil {
					if signals := signals.GetRouteSignals(ctx.Path()); signals != nil {
						signals.IncrementFails()
					}
					panic(r)
				}
			}()

			start := time.Now()
			err := next.ServeAppHost(ctx)
			duration := time.Since(start)

			if signals := signals.GetRouteSignals(ctx.Path()); signals != nil {
				if err == nil {
					signals.IncrementSuccess()
				} else {
					signals.IncrementFails()
				}
				signals.RecordDuration(duration)
			}
			return err
		})
	}
}

// RouteSignals and RouterSignalsRepository implementation

type routerSignalsRepository struct {
	signals map[string]RouteSignals

	registry metrics.Registry
	buckets  libmetrics.BucketsGenerationPolicy
}

func (r *routerSignalsRepository) RegisterRouteSignals(path string) RouteSignals {
	routeSignals := newRouteSignals(r.registry.WithTags(map[string]string{"path": path}), r.buckets)
	r.signals[path] = routeSignals
	return routeSignals
}

func (r *routerSignalsRepository) GetRouteSignals(path string) RouteSignals {
	s, ok := r.signals[path]
	if !ok {
		panic(fmt.Sprintf("route signals not stored for path %s", path))
	}
	return s.(routeSignals)
}

type routeSignals struct {
	success           metrics.Counter
	fails             metrics.Counter
	total             metrics.Counter
	durationHistogram metrics.Timer
}

func newRouteSignals(registry metrics.Registry, buckets libmetrics.BucketsGenerationPolicy) routeSignals {
	signals := routeSignals{
		success:           registry.Counter("success"),
		fails:             registry.Counter("fails"),
		total:             registry.Counter("total"),
		durationHistogram: registry.DurationHistogram("duration_buckets", buckets()),
	}

	solomon.Rated(signals.success)
	solomon.Rated(signals.fails)
	solomon.Rated(signals.total)
	solomon.Rated(signals.durationHistogram)
	return signals
}

func (r routeSignals) IncrementSuccess() {
	r.success.Inc()
	r.total.Inc()
}

func (r routeSignals) IncrementFails() {
	r.fails.Inc()
	r.total.Inc()
}

func (r routeSignals) RecordDuration(d time.Duration) {
	r.durationHistogram.RecordDuration(d)
}
