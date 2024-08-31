package linker

import (
	"fmt"
	"net/url"

	"a.yandex-team.ru/alice/amanda/internal/hash"
)

type Service interface {
	GetEditDeviceStateURL(id string) *url.URL
}

type service struct {
	tokenHash string
	env       string
	fqdn      string
}

func New(token, env, fqdn string) Service {
	return &service{
		tokenHash: hash.MD5(token),
		env:       env,
		fqdn:      fqdn,
	}
}

func (s *service) GetEditDeviceStateURL(id string) *url.URL {
	return &url.URL{
		Scheme: "https",
		Host:   s.fqdn,
		Path:   fmt.Sprintf("/%s/%s/%s/device", s.env, s.tokenHash, id),
	}
}
