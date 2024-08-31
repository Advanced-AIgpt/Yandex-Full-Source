package discovery

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
)

type IController interface {
	ProviderDiscovery(ctx context.Context, origin model.Origin, skillID string) (model.Devices, error)
	StartDiscoveryFromSurface(ctx context.Context, origin model.Origin, targetSurface model.SurfaceParameters, discoveryType DiscoveryType) error
	DiscoveryPostprocessing(ctx context.Context, skillInfo provider.SkillInfo, origin model.Origin, discoveryResult adapter.DiscoveryResult) (model.Devices, error)
	StoreDiscoveredDevices(ctx context.Context, user model.User, devices model.Devices) (model.DeviceStoreResults, error)

	// SendNewDevicesPush notifies user about changes in device list
	SendNewDevicesPush(ctx context.Context, user model.User, skillInfo provider.SkillInfo, devices []DeviceDiffInfo) error
	PushDiscovery(ctx context.Context, skillID string, origin model.Origin, discoveryResult adapter.DiscoveryResult) (model.DeviceStoreResults, error)
	CallbackDiscovery(ctx context.Context, skillID string, origin model.Origin, devices model.Devices, filter callback.IDiscoveryFilter) (model.DeviceStoreResults, error)
}
