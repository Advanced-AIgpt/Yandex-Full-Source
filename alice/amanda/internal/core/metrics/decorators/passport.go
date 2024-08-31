package decorators

import (
	"a.yandex-team.ru/alice/amanda/internal/passport"
	"a.yandex-team.ru/alice/amanda/internal/sensors"
	"a.yandex-team.ru/library/go/core/metrics"
)

type passportDecorator struct {
	service  passport.Service
	registry metrics.Registry
}

func (p *passportDecorator) GetAuthURL() string {
	return p.service.GetAuthURL()
}

func (p *passportDecorator) GetAccessToken(code string) (string, error) {
	onFinish := sensors.PushServiceMetrics(p.registry)
	r, err := p.service.GetAccessToken(code)
	onFinish(err)
	return r, err
}

func (p passportDecorator) GetUsername(accessToken string) (string, error) {
	onFinish := sensors.PushServiceMetrics(p.registry)
	r, err := p.service.GetUsername(accessToken)
	onFinish(err)
	return r, err
}

func (p passportDecorator) GetUserID(accessToken string) (string, error) {
	onFinish := sensors.PushServiceMetrics(p.registry)
	r, err := p.service.GetUserID(accessToken)
	onFinish(err)
	return r, err
}

func NewPassport(service passport.Service, registry metrics.Registry) passport.Service {
	return &passportDecorator{
		service:  service,
		registry: sensors.AddServiceTag(registry, "passport"),
	}
}
