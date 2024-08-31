package query

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type IController interface {
	UpdateDevicesState(ctx context.Context, devices model.Devices, origin model.Origin) (model.Devices, model.DeviceStatusMap, error)
	GetProviderDevicesState(ctx context.Context, skillID string, userDevices model.Devices, origin model.Origin) ([]adapter.DeviceStateView, error)
}
