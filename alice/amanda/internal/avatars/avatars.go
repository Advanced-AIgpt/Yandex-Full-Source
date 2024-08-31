package avatars

import (
	"a.yandex-team.ru/alice/amanda/pkg/avatars"
)

type Service interface {
	Upload(image []byte) (url string, err error)
}

type service struct{}

func New() Service {
	return new(service)
}

func (s *service) Upload(image []byte) (url string, err error) {
	r, err := avatars.Upload(image)
	if err != nil {
		return "", err
	}
	return r.URL, nil
}
