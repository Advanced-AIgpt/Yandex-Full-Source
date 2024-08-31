package updates

const searchAppClientName = "searchapp"

type EventID string

const (
	UpdateDeviceStateEventID  EventID = "update_device_state"
	UpdateStatesEventID       EventID = "update_states"
	UpdateDeviceListEventID   EventID = "update_device_list"
	UpdateScenarioListEventID EventID = "update_scenario_list"
	FinishDiscoveryEventID    EventID = "finish_discovery"
	AddVoiceprintEventID      EventID = "add_voiceprint"
	RemoveVoiceprintEventID   EventID = "remove_voiceprint"
)

type EventKey string

const DeviceIDKey EventKey = "device_id"

type Source string

const (
	UpdateDeviceSource              Source = "update_device"
	DeleteDeviceSource              Source = "delete_device"
	DeleteChangedOwnerDevicesSource Source = "delete_changed_owner_devices"

	CreateGroupSource Source = "create_group"
	UpdateGroupSource Source = "update_group"
	DeleteGroupSource Source = "delete_group"

	CreateScenarioSource Source = "create_scenario"
	UpdateScenarioSource Source = "update_scenario"
	DeleteScenarioSource Source = "delete_scenario"

	CreateScenarioLaunchSource Source = "create_scenario_launch"
	UpdateScenarioLaunchSource Source = "update_scenario_launch"

	CreateHouseholdSource     Source = "create_household"
	UpdateHouseholdSource     Source = "update_household"
	SetCurrentHouseholdSource Source = "set_current_household"
	DeleteHouseholdSource     Source = "delete_household"

	CreateRoomSource Source = "create_room"
	UpdateRoomSource Source = "update_room"
	RenameRoomSource Source = "rename_room"
	DeleteRoomSource Source = "delete_room"

	UpdateFavoritesSource Source = "update_favorites"

	CreateStereopairSource Source = "create_stereopair"
	DeleteStereopairSource Source = "delete_stereopair"

	CreateTandemSource Source = "create_tandem"
	DeleteTandemSource Source = "delete_tandem"

	DiscoverySource Source = "discovery"
	QuerySource     Source = "query"
	CallbackSource  Source = "callback"
	ActionSource    Source = "action"
	UnlinkSource    Source = "unlink"

	VoiceprintSource Source = "voiceprint"
)
