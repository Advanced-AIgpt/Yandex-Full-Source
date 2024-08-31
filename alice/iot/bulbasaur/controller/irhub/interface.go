package irhub

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type Controller interface {
	IRHubRemotes(ctx context.Context, origin model.Origin, hub model.Device) (model.Devices, error)
}
