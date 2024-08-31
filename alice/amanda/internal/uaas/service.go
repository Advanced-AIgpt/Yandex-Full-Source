package uaas

import (
	"a.yandex-team.ru/alice/amanda/pkg/uniproxy"
)

type Service interface {
	NewClient(settings uniproxy.Settings) uniproxy.Client
}

type service struct{}

func (s service) NewClient(settings uniproxy.Settings) uniproxy.Client {
	return uniproxy.NewClient(settings)
}

func New() Service {
	return new(service)
}
