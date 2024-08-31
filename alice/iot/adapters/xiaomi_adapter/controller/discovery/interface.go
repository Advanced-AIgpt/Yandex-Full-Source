package discovery

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
)

type IController interface {
	Discovery(ctx context.Context, token string, userID uint64) (externalUserID string, deviceInfoView []adapter.DeviceInfoView, err error)
	DiscoverDevicesByID(ctx context.Context, token, externalUserID string, deviceID string) ([]adapter.DeviceInfoView, error)
}
