package callback

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

type IController interface {
	CallbackDiscovery(ctx context.Context, skillID string, payload callback.DiscoveryPayload) (err error)
	CallbackUpdateState(ctx context.Context, skillID string, payload callback.UpdateStatePayload, updatedTimestamp timestamp.PastTimestamp) (err error)
}
