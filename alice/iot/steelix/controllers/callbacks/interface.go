package callbacks

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
)

type IController interface {
	// SendCallback sends external callback to bulbasaur
	SendCallback(ctx context.Context, skillID string, stateCallback callback.UpdateStateRequest) error
}
