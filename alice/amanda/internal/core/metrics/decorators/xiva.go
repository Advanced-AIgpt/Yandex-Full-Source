package decorators

import (
	"a.yandex-team.ru/alice/amanda/internal/sensors"
	"a.yandex-team.ru/alice/amanda/internal/xiva"
	"a.yandex-team.ru/library/go/core/metrics"
)

func NewXiva(service xiva.Service, registry metrics.Registry) xiva.Service {
	return &xivaDecorator{
		service:  service,
		registry: sensors.AddServiceTag(registry, "xiva"),
	}
}

type xivaDecorator struct {
	service  xiva.Service
	registry metrics.Registry
}

func (x xivaDecorator) PlayText(text, deviceID, userID string) error {
	onFinish := sensors.PushServiceMetrics(x.registry)
	err := x.service.PlayText(text, deviceID, userID)
	onFinish(err)
	return err
}
