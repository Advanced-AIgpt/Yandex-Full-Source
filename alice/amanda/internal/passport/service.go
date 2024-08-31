package passport

import (
	"a.yandex-team.ru/alice/amanda/pkg/passport"
)

type Service interface {
	GetAuthURL() string
	GetAccessToken(code string) (string, error)
	GetUsername(accessToken string) (string, error)
	GetUserID(accessToken string) (string, error)
}

type service struct {
	api *passport.API
}

func New(clientID, clientSecret string) Service {
	return &service{
		api: passport.New(clientID, clientSecret),
	}
}

func (s *service) GetAuthURL() string {
	return s.api.GenerateCodeURL()
}

func (s *service) GetAccessToken(code string) (string, error) {

	r, err := s.api.GetOAuthToken(code)
	if err != nil {
		return "", err
	}
	return r.AccessToken, nil
}

func (s *service) GetUsername(accessToken string) (string, error) {
	r, err := s.api.GetUserInfo(accessToken)
	if err != nil {
		return "", err
	}
	return r.Username, nil
}

func (s *service) GetUserID(accessToken string) (string, error) {
	r, err := s.api.GetUserInfo(accessToken)
	if err != nil {
		return "", err
	}
	return r.UserID, nil
}
