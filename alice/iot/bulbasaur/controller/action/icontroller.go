package action

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type IController interface {
	// SendActionsToDevices sends actions to devices via http providers
	// Deprecated: migrate to v2
	SendActionsToDevices(ctx context.Context, origin model.Origin, requestedDevices model.Devices) SideEffects

	// SendActionsToDevicesV2 sends actions to devices via http providers
	SendActionsToDevicesV2(ctx context.Context, origin model.Origin, actions []DeviceAction) SideEffects
}
