package sup

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type IController interface {
	SendPushToUser(ctx context.Context, user model.User, pushInfo PushInfo) error
}
