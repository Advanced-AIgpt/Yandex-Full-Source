package decorators

import (
	"a.yandex-team.ru/alice/amanda/internal/sensors"
	"a.yandex-team.ru/alice/amanda/internal/uaas"
	"a.yandex-team.ru/alice/amanda/pkg/uniproxy"
	"a.yandex-team.ru/library/go/core/metrics"
)

type uaasDecorator struct {
	service  uaas.Service
	registry metrics.Registry
}

type uniproxyClientDecorator struct {
	client   uniproxy.Client
	registry metrics.Registry
}

func (u *uniproxyClientDecorator) SendText(text string) (*uniproxy.Response, error) {
	return u.withMetrics(func() (*uniproxy.Response, error) {
		return u.client.SendText(text)
	})
}

func (u *uniproxyClientDecorator) SendVoice(data []byte) (*uniproxy.Response, error) {
	return u.withMetrics(func() (*uniproxy.Response, error) {
		return u.client.SendVoice(data)
	})
}

func (u *uniproxyClientDecorator) SendCallback(name string, payload interface{}) (*uniproxy.Response, error) {
	return u.withMetrics(func() (*uniproxy.Response, error) {
		return u.client.SendCallback(name, payload)
	})
}

func (u *uniproxyClientDecorator) SendImage(url string) (*uniproxy.Response, error) {
	return u.withMetrics(func() (*uniproxy.Response, error) {
		return u.client.SendImage(url)
	})
}

func (u *uniproxyClientDecorator) withMetrics(fn func() (*uniproxy.Response, error)) (*uniproxy.Response, error) {
	onFinish := sensors.PushServiceMetrics(u.registry)
	r, err := fn()
	onFinish(err)
	return r, err
}

func (u *uaasDecorator) NewClient(settings uniproxy.Settings) uniproxy.Client {
	return &uniproxyClientDecorator{
		client:   u.service.NewClient(settings),
		registry: u.registry,
	}
}

func NewUaas(service uaas.Service, registry metrics.Registry) uaas.Service {
	return &uaasDecorator{
		service:  service,
		registry: sensors.AddServiceTag(registry, "uniproxy"),
	}
}
