package interceptor

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/amelie/pkg/sensor"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	"a.yandex-team.ru/library/go/core/metrics"
)

type typesRegistry struct {
	counter metrics.Counter
	timer   metrics.Timer
}

type SensorInterceptor struct {
	typesRegistries map[telegram.EventType]*typesRegistry
	registry        metrics.Registry
}

func (s *SensorInterceptor) getTypesRegistry(eventType telegram.EventType) *typesRegistry {
	if _, ok := s.typesRegistries[eventType]; !ok {
		types := withEventType(s.registry.WithPrefix("types."), eventType)
		s.typesRegistries[eventType] = &typesRegistry{
			counter: sensor.NewRatedCounter(withEventType(types, eventType), "count_per_second"),
			timer:   sensor.NewRatedHist(types, "processing_time_seconds", sensor.DefaultTimePolicy()),
		}
	}
	return s.typesRegistries[eventType]
}

func (s *SensorInterceptor) Intercept(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{},
	next telegram.NextInterceptorDelegate) error {
	types := s.getTypesRegistry(eventType)
	defer func(start time.Time) {
		types.timer.RecordDuration(time.Since(start))
	}(time.Now())
	types.counter.Inc()
	return next(ctx, bot, eventType, event)
}

func withEventType(registry metrics.Registry, eventType telegram.EventType) metrics.Registry {
	return registry.WithTags(map[string]string{"event_type": string(eventType)})
}

func NewSensorInterceptor(registry metrics.Registry) *SensorInterceptor {
	return &SensorInterceptor{
		typesRegistries: make(map[telegram.EventType]*typesRegistry),
		registry:        registry,
	}
}
