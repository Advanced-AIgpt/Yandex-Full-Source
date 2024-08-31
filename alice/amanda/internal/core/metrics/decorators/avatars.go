package decorators

import (
	"a.yandex-team.ru/alice/amanda/internal/avatars"
	"a.yandex-team.ru/alice/amanda/internal/sensors"
	"a.yandex-team.ru/library/go/core/metrics"
)

type avatarsDecorator struct {
	service  avatars.Service
	registry metrics.Registry
}

func (a *avatarsDecorator) Upload(image []byte) (url string, err error) {
	onFinish := sensors.PushServiceMetrics(a.registry)
	url, err = a.service.Upload(image)
	onFinish(err)
	return
}

func NewAvatars(service avatars.Service, registry metrics.Registry) avatars.Service {
	return &avatarsDecorator{
		service:  service,
		registry: sensors.AddServiceTag(registry, "avatars"),
	}
}
