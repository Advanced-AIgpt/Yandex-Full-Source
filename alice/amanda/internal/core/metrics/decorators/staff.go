package decorators

import (
	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/sensors"
	"a.yandex-team.ru/alice/amanda/internal/staff"
	"a.yandex-team.ru/library/go/core/metrics"
)

type staffDecorator struct {
	service  staff.Service
	registry metrics.Registry
}

func (s *staffDecorator) IsYandexoid(ctx app.Context) (bool, error) {
	onFinish := sensors.PushServiceMetrics(s.registry)
	r, err := s.service.IsYandexoid(ctx)
	onFinish(err)
	return r, err
}

func NewStaff(service staff.Service, registry metrics.Registry) staff.Service {
	return &staffDecorator{
		service:  service,
		registry: sensors.AddServiceTag(registry, "staff"),
	}
}
