package unlink

import (
	"context"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/db"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/userapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	database      db.DB
	userAPIClient userapi.APIClient
}

func NewController(database db.DB, userAPIClient userapi.APIClient) *Controller {
	return &Controller{
		database:      database,
		userAPIClient: userAPIClient,
	}
}

func (c *Controller) Unlink(ctx context.Context, token string, userID uint64) error {
	userProfile, err := c.userAPIClient.GetUserProfile(ctx, token)
	if err != nil {
		return xerrors.Errorf("unable to get external user id: %w", err)
	}
	externalUserID := userProfile.Data.UnionID
	return c.database.DeleteExternalUser(ctxlog.WithFields(ctx, log.String("external_user_id", externalUserID)), externalUserID, userID)
}
