package decorators

import (
	"a.yandex-team.ru/alice/amanda/internal/divrenderer"
	"a.yandex-team.ru/alice/amanda/internal/sensors"
	"a.yandex-team.ru/library/go/core/metrics"
)

type divRendererDecorator struct {
	service  divrenderer.Service
	registry metrics.Registry
}

func (d *divRendererDecorator) RenderDIVCard(body map[string]interface{}) (image []byte, err error) {
	onFinish := sensors.PushServiceMetrics(d.registry)
	image, err = d.service.RenderDIVCard(body)
	onFinish(err)
	return image, err
}

func NewDivRenderer(service divrenderer.Service, registry metrics.Registry) divrenderer.Service {
	return &divRendererDecorator{
		service:  service,
		registry: sensors.AddServiceTag(registry, "divrenderer"),
	}
}
