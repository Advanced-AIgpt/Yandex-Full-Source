package staff

import (
	"fmt"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/pkg/staff"
)

type Service interface {
	IsYandexoid(ctx app.Context) (bool, error)
}

type service struct {
	api staff.API
}

func New(token string) Service {
	return &service{
		api: staff.New(token),
	}
}

func (s *service) IsYandexoid(ctx app.Context) (bool, error) {
	user, err := s.api.GetUserByTelegramUsername(ctx.GetUsername())
	if err != nil {
		return false, fmt.Errorf("unable to obtain information about user: %w", err)
	}
	return user != nil && user.Official != nil && !user.Official.IsDismissed, nil
}
