package updates

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

// todo: use userID everywhere instead of user - IOT-1233

type IController interface {
	UserInfoUpdatesWebsocketURL(ctx context.Context, userID uint64, sessionID string) (string, error)
	DeviceStateUpdatesWebsocketURL(ctx context.Context, userID uint64, deviceID string, sessionID string) (string, error)

	SendUpdateDeviceStateEvent(ctx context.Context, userID uint64, event UpdateDeviceStateEvent) error
	SendUpdateDeviceListEvent(ctx context.Context, userID uint64, event UpdateDeviceListEvent) error
	SendUpdateScenarioListEvent(ctx context.Context, userID uint64, event UpdateScenarioListEvent) error
	SendUpdateStatesEvent(ctx context.Context, userID uint64, event UpdateStatesEvent) error
	SendFinishDiscoveryEvent(ctx context.Context, userID uint64, event FinishDiscoveryEvent) error

	NotifyAboutStateUpdates(ctx context.Context, userID uint64, stateUpdates StateUpdates) error
	NotifyAboutDeviceListUpdates(ctx context.Context, user model.User, source Source) error
	NotifyAboutError(ctx context.Context, origin model.Origin) error

	AsyncNotifyAboutStateUpdates(ctx context.Context, userID uint64, stateUpdates StateUpdates)
	AsyncNotifyAboutDeviceListUpdates(ctx context.Context, user model.User, source Source)
	AsyncNotifyAboutError(ctx context.Context, origin model.Origin)

	SendAddVoiceprintEvent(ctx context.Context, userID uint64, event AddVoiceprintEvent) error
	SendRemoveVoiceprintEvent(ctx context.Context, userID uint64, event RemoveVoiceprintEvent) error

	sendEvent(ctx context.Context, userID uint64, event event) error

	// HasActiveMobileSubscriptions returns true if exists at least one active xiva subscriber
	HasActiveMobileSubscriptions(ctx context.Context, userID uint64) bool

	// HasActiveNotificatorSubscriptions returns true if exists at least one active notifier subscriber
	HasActiveNotificatorSubscriptions(ctx context.Context, userID uint64) bool
}

type event interface {
	source() Source
	id() EventID
	keys() map[EventKey]string
}
