package sup

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type IController interface {
	WifiIs5GhzPush(ctx context.Context, userID uint64, deviceType model.DeviceType) error
	DiscoveryErrorPush(ctx context.Context, userID uint64, deviceType model.DeviceType) error
	DiscoveryNotAllowedSpeakerPush(ctx context.Context, userID uint64, deviceType model.DeviceType) error
	DiscoveryNotAllowedClientPush(ctx context.Context, userID uint64, deviceType model.DeviceType) error
}
