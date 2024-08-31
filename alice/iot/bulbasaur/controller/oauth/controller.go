package oauth

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/oauth"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type Controller struct {
	logger      log.Logger
	oauthClient oauth.Client
	clients     Clients
}

func NewController(
	logger log.Logger,
	oauthClient oauth.Client,
	clients Clients,
) IController {
	return &Controller{
		logger:      logger,
		oauthClient: oauthClient,
		clients:     clients,
	}
}

func (c *Controller) IssueAuthCodeForYandexIO(
	ctx context.Context,
	user model.User,
	tokenType TokenType,
) (string, error) {
	ctxlog.Infof(ctx, c.logger, "start to issue auth code with token type %s", tokenType)

	var credentials ClientCredentials
	switch tokenType {
	case XTokenTokenType:
		credentials = c.clients.YandexIOXToken
	case OAuthTokenType:
		credentials = c.clients.YandexIOOAuth
	default:
		return "", xerrors.New(fmt.Sprintf("unknown token type %d for issue auth code", tokenType))
	}

	authInfo, err := c.oauthClient.IssueAuthorizationCode(
		ctx,
		oauth.UserData{
			UserTicket: user.Ticket,
			IP:         user.IP,
		},
		oauth.IssueAuthorizationCodeParams{
			ClientID:          credentials.ClientID,
			ClientSecret:      credentials.ClientSecret,
			CodeStrength:      oauth.LongAuthCodeStrength,
			RequireActivation: ptr.Bool(false), // do not require auth code activation on yandex station
		},
	)
	if err != nil {
		return "", xerrors.Errorf("failed to issue auth code: %w", err)
	}

	ctxlog.Infof(ctx, c.logger, "issued auth code type %s with len %d", tokenType.String(), len(authInfo.Code))
	return authInfo.Code, nil
}
