package unlink

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type Controller interface {
	DeleteChangedOwnerDevices(ctx context.Context, newOwnerUID uint64, skillID string, devices model.Devices) error
	// UnlinkProvider removes all connections with external device provider
	UnlinkProvider(ctx context.Context, skillID string, origin model.Origin, saveDevices bool) error
	// DeleteDevices moves devices to archived and syncs local scenarios
	DeleteDevices(ctx context.Context, userID uint64, deviceIDs []string) error
}
