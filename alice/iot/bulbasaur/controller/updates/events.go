package updates

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

type UpdateStatesEvent struct {
	mobile.UpdateStatesEvent
	Source Source `json:"source"`
}

func (e UpdateStatesEvent) source() Source {
	return e.Source
}

func (e UpdateStatesEvent) id() EventID {
	return UpdateStatesEventID
}

func (e UpdateStatesEvent) keys() map[EventKey]string {
	return nil
}

func (e UpdateStatesEvent) UpdatesCount() int {
	return len(e.UpdatedDevices) + len(e.UpdateGroups)
}

func (e UpdateStatesEvent) HasUpdates() bool {
	return e.UpdatesCount() != 0
}

type UpdateDeviceStateEvent struct {
	mobile.DevicePartialStateView
}

func (e UpdateDeviceStateEvent) source() Source {
	return CallbackSource
}

func (e UpdateDeviceStateEvent) id() EventID {
	return UpdateDeviceStateEventID
}

func (e UpdateDeviceStateEvent) keys() map[EventKey]string {
	return map[EventKey]string{
		DeviceIDKey: e.ID,
	}
}

func (e UpdateDeviceStateEvent) UpdatesCount() int {
	return len(e.Capabilities) + len(e.Properties)
}

func (e UpdateDeviceStateEvent) HasUpdates() bool {
	/* todo: when status is sent, check that
	/ event.UpdatesCount() == 0 && event.Status != model.OnlineStatus
	/ is considered as updates status
	*/
	return e.UpdatesCount() != 0
}

type UpdateDeviceListEvent struct {
	Households []mobile.HouseholdWithDevicesViewV3 `json:"households"`
	Favorites  mobile.FavoriteListView             `json:"favorites"`
	Source     Source                              `json:"source"`
}

func (e UpdateDeviceListEvent) source() Source {
	return e.Source
}

func (e UpdateDeviceListEvent) id() EventID {
	return UpdateDeviceListEventID
}

func (e UpdateDeviceListEvent) keys() map[EventKey]string {
	return nil
}

func (e *UpdateDeviceListEvent) From(ctx context.Context, userInfo model.UserInfo, source Source) {
	var favoriteListView mobile.FavoriteListView
	favoriteListView.From(ctx, userInfo)
	e.Favorites = favoriteListView
	e.Households = mobile.NewHouseholdWithDevicesViews(ctx, userInfo)
	e.Source = source
}

type UpdateScenarioListEvent struct {
	Scenarios          []mobile.ScenarioListView          `json:"scenarios"`
	ScheduledScenarios []mobile.ScheduledLaunchesListView `json:"scheduled_scenarios"`
	Source             Source                             `json:"source"`
}

func (e UpdateScenarioListEvent) source() Source {
	return e.Source
}

func (e UpdateScenarioListEvent) id() EventID {
	return UpdateScenarioListEventID
}

func (e UpdateScenarioListEvent) keys() map[EventKey]string {
	return nil
}

func (e *UpdateScenarioListEvent) From(scenarios model.Scenarios, devices model.Devices, launches model.ScenarioLaunches, now timestamp.PastTimestamp, source Source) {
	e.Scenarios = mobile.NewScenarioListViews(scenarios, devices)
	e.ScheduledScenarios = mobile.NewScheduledLaunchesListViews(launches, now)
	e.Source = source
}

type FinishDiscoveryEvent struct {
	Status              string                `json:"status"`         // "ok" or "error"
	Code                string                `json:"code,omitempty"` // "DEVICE_OFFLINE" or "INTERNAL_ERROR" or not present
	NewDevices          []mobile.ItemInfoView `json:"new_devices"`    // xxx(galecore): there must be a way to refactor this dependency
	UpdatedDevicesCount int                   `json:"updated_devices_count"`
	Source              Source                `json:"source"` // useful debug field
}

func (d FinishDiscoveryEvent) source() Source {
	return d.Source
}

func (d FinishDiscoveryEvent) id() EventID {
	return FinishDiscoveryEventID
}

func (d FinishDiscoveryEvent) keys() map[EventKey]string {
	return nil
}

func (d *FinishDiscoveryEvent) FromStoreResults(ctx context.Context, storeResults model.DeviceStoreResults) {
	d.Status = "ok"
	d.Source = DiscoverySource
	d.NewDevices = make([]mobile.ItemInfoView, 0, len(storeResults))
	for _, storeResult := range storeResults {
		switch storeResult.Result {
		case model.StoreResultNew:
			var newDevice mobile.ItemInfoView
			newDevice.FromDevice(ctx, storeResult.Device)
			d.NewDevices = append(d.NewDevices, newDevice)
		case model.StoreResultUpdated:
			d.UpdatedDevicesCount++
		}
	}
}

func (d *FinishDiscoveryEvent) FromError(errorCode model.ErrorCode) {
	d.Status = "ok"
	d.Source = DiscoverySource
	d.Code = string(errorCode)
}

type AddVoiceprintEvent struct {
	Status    string          `json:"status"`               // "ok" or "error"
	ErrorCode model.ErrorCode `json:"error_code,omitempty"` // some voiceprint-specific error codes
	DeviceID  string          `json:"device_id"`            // smart home device id of speaker
}

func (e AddVoiceprintEvent) source() Source {
	return VoiceprintSource
}

func (e AddVoiceprintEvent) id() EventID {
	return AddVoiceprintEventID
}

func (e AddVoiceprintEvent) keys() map[EventKey]string {
	return nil
}

func NewAddVoiceprintSuccessEvent(deviceID string) AddVoiceprintEvent {
	return AddVoiceprintEvent{
		Status:   "ok",
		DeviceID: deviceID,
	}
}

func NewAddVoiceprintErrorEvent(deviceID string, errorCode model.ErrorCode) AddVoiceprintEvent {
	return AddVoiceprintEvent{
		Status:    "error",
		ErrorCode: errorCode,
		DeviceID:  deviceID,
	}
}

type RemoveVoiceprintEvent struct {
	Status    string          `json:"status"`               // "ok" or "error"
	ErrorCode model.ErrorCode `json:"error_code,omitempty"` // some voiceprint-specific error codes
	DeviceID  string          `json:"device_id"`            // smart home device id of speaker
}

func (e RemoveVoiceprintEvent) source() Source {
	return VoiceprintSource
}

func (e RemoveVoiceprintEvent) id() EventID {
	return RemoveVoiceprintEventID
}

func (e RemoveVoiceprintEvent) keys() map[EventKey]string {
	return nil
}

func NewRemoveVoiceprintSuccessEvent(deviceID string) RemoveVoiceprintEvent {
	return RemoveVoiceprintEvent{
		Status:   "ok",
		DeviceID: deviceID,
	}
}

func NewRemoveVoiceprintErrorEvent(deviceID string, errorCode model.ErrorCode) RemoveVoiceprintEvent {
	return RemoveVoiceprintEvent{
		Status:    "error",
		ErrorCode: errorCode,
		DeviceID:  deviceID,
	}
}
