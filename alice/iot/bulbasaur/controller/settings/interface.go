package settings

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	memento "a.yandex-team.ru/alice/memento/proto"
)

type IController interface {
	UpdateUserSettings(ctx context.Context, user model.User, settings UserSettings) error
	GetUserSettings(ctx context.Context, user model.User) (UserSettings, error)
	ExtractUserSettingsFromRequest(ctx context.Context, request *scenarios.TScenarioBaseRequest) (UserSettings, error)

	// VoiceprintDeviceConfigs retrieves voiceprint device configs for quasar devices
	// not quasar devices are ignored
	VoiceprintDeviceConfigs(ctx context.Context, user model.User, devices model.Devices) (VoiceprintDeviceConfigs, error)
}

type IUserConfig interface {
	From(configPair *memento.TConfigKeyAnyPair) error
	// if any field in settings set for this config - return true, else false
	ExtractFrom(settings UserSettings) (bool, error)
	ExtractTo(settings UserSettings) (UserSettings, error)
	Build() (*memento.TConfigKeyAnyPair, error)
	Key() memento.EConfigKey
}
