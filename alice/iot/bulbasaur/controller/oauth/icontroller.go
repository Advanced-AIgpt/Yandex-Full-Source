package oauth

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type IController interface {
	// IssueAuthCodeForYandexIO issues xcode for given user for yandexIO app
	IssueAuthCodeForYandexIO(ctx context.Context, user model.User, tokenType TokenType) (string, error)
}

type Clients struct {
	YandexIOXToken ClientCredentials
	YandexIOOAuth  ClientCredentials
}

// ClientCredentials describe client specific info for oauth
type ClientCredentials struct {
	ClientID     string
	ClientSecret string
}

type TokenType int

const (
	UnknownTokenType TokenType = iota
	OAuthTokenType
	XTokenTokenType
)

func (t TokenType) String() string {
	switch t {
	case OAuthTokenType:
		return "oauth"
	case XTokenTokenType:
		return "xtoken"
	default:
		return "unknown"
	}
}
