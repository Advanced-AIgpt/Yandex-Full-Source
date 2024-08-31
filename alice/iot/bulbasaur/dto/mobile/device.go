package mobile

import (
	"context"
	"encoding/json"
	"fmt"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/settings"
	"github.com/mitchellh/mapstructure"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	btuya "a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/xiaomi"
	"a.yandex-team.ru/alice/library/go/recorder"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type DeviceShortInfoView struct {
	ID           string              `json:"id"`
	Name         string              `json:"name"`
	Type         model.DeviceType    `json:"type"`
	IconURL      string              `json:"icon_url"`
	Capabilities []model.ICapability `json:"capabilities"`
	Properties   []PropertyStateView `json:"properties"`
}

func (d *DeviceShortInfoView) FromDevice(device model.Device) {
	d.ID = device.ID
	d.Name = device.Name
	d.Type = device.Type
	d.IconURL = device.Type.IconURL(model.OriginalIconFormat)

	capabilities := make([]model.ICapability, 0)
	if onOffCapability, exists := device.GetCapabilityByTypeAndInstance(model.OnOffCapabilityType, string(model.OnOnOffCapabilityInstance)); exists {
		capabilities = append(capabilities, onOffCapability)
	}
	d.Capabilities = capabilities

	properties := make([]PropertyStateView, 0, len(device.Properties))
	for _, property := range device.Properties {
		var propertyStateView PropertyStateView
		propertyStateView.FromProperty(property)
		properties = append(properties, propertyStateView)
	}
	d.Properties = properties
}

type DeviceInfoView struct {
	DeviceShortInfoView
	SkillID    string           `json:"skill_id"`
	QuasarInfo *QuasarInfo      `json:"quasar_info,omitempty"`
	RenderInfo *RenderInfoView  `json:"render_info,omitempty"`
	ItemType   ItemInfoViewType `json:"item_type"`
	Stereopair *StereopairView  `json:"stereopair,omitempty"`
	Groups     []string         `json:"groups"`
}

func (d *DeviceInfoView) FromDevice(device model.Device) {
	d.DeviceShortInfoView.FromDevice(device)
	d.ItemType = DeviceItemInfoViewType

	d.SkillID = device.SkillID

	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		d.QuasarInfo = &quasarInfo
	}
	d.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)

	groups := make([]string, 0, len(device.Groups))
	for _, group := range device.Groups {
		groups = append(groups, group.Name)
	}
	d.Groups = groups
}

func (d *DeviceInfoView) FromStereopair(ctx context.Context, stereopair model.Stereopair) {
	leaderDevice := stereopair.GetLeaderDevice()
	d.FromDevice(leaderDevice)
	d.Name = stereopair.Name
	d.ItemType = StereopairItemInfoViewType
	d.Stereopair = &StereopairView{}
	d.Stereopair.From(ctx, stereopair)
}

type DeviceListView struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	DeviceListInfo
}

type DeviceListInfo struct {
	Rooms               []RoomInfoView   `json:"rooms"`
	Groups              []GroupInfoView  `json:"groups"`
	UnconfiguredDevices []DeviceInfoView `json:"unconfigured_devices"`
	Speakers            []DeviceInfoView `json:"speakers"`
}

func (d *DeviceListInfo) FromDevices(ctx context.Context, devices []model.Device, forScenarios bool, stereopairs model.Stereopairs) {
	d.Rooms = make([]RoomInfoView, 0)
	rooms := make(map[string]RoomInfoView)
	d.Groups = make([]GroupInfoView, 0)
	d.UnconfiguredDevices = make([]DeviceInfoView, 0)
	d.Speakers = make([]DeviceInfoView, 0)

	for _, device := range devices {
		deviceInfoView := DeviceInfoView{}

		// TODO: IOT-271: temporary hack to prevent showing cars in searchapp
		if device.SkillID == model.REMOTECAR {
			continue
		}

		if forScenarios && !device.IsAvailableForScenarios() {
			continue
		}

		switch stereopairs.GetDeviceRole(device.ID) {
		case model.LeaderRole:
			stereopair, _ := stereopairs.GetByDeviceID(device.ID)
			deviceInfoView.FromStereopair(ctx, stereopair)
		case model.FollowerRole:
			// skip device
			continue
		default:
			deviceInfoView.FromDevice(device)
		}

		// Speakers without rooms
		if device.Type.IsSmartSpeaker() && device.Room == nil {
			d.Speakers = append(d.Speakers, deviceInfoView)
			continue
		}

		// Unconfigured devices
		if err := device.AssertSetup(); err != nil {
			d.UnconfiguredDevices = append(d.UnconfiguredDevices, deviceInfoView)
			continue // Skip Room Grouping cause unconfigured devices should not be displayed at Rooms section
		}

		// Devices grouped by Rooms
		if room, exists := rooms[device.Room.Name]; !exists {
			rooms[device.Room.Name] = RoomInfoView{
				ID:      device.Room.ID,
				Name:    device.Room.Name,
				Devices: []DeviceInfoView{deviceInfoView},
			}
		} else {
			room.Devices = append(room.Devices, deviceInfoView)
			rooms[device.Room.Name] = room
		}
	}

	for key := range rooms {
		sort.Sort(deviceInfoViewByName(rooms[key].Devices))
	}

	// Devices grouped by group (kek)
	d.Groups = GroupInfoViewFromDevices(devices)

	// Sort rooms by name
	roomsList := make([]RoomInfoView, 0, len(rooms))
	for _, room := range rooms {
		roomsList = append(roomsList, room)
	}
	sort.Sort(roomInfoViewByName(roomsList))

	d.Rooms = roomsList

	// Sort speakers by name
	sort.Sort(deviceInfoViewByName(d.Speakers))

	// Sort unconfigured devices by name
	sort.Sort(deviceInfoViewByName(d.UnconfiguredDevices))
}

type DeviceListViewV2 struct {
	Status     string                     `json:"status"`
	RequestID  string                     `json:"request_id"`
	Households []HouseholdWithDevicesView `json:"households"`
	UpdatesURL string                     `json:"updates_url,omitempty"`
}

type HouseholdWithDevicesView struct {
	HouseholdInfoView
	DeviceListInfo
}

func (d *DeviceListViewV2) From(ctx context.Context, currentHouseholdID string, households []model.Household, devices model.Devices, forScenarios bool, stereopairs model.Stereopairs, updatesURL string) {
	d.UpdatesURL = updatesURL
	devicesPerHouseholds := devices.GroupByHousehold()
	d.Households = make([]HouseholdWithDevicesView, 0, len(households))
	for _, h := range households {
		var view HouseholdWithDevicesView
		view.FromHousehold(h, currentHouseholdID)
		view.FromDevices(ctx, devicesPerHouseholds[view.ID], forScenarios, stereopairs)
		d.Households = append(d.Households, view)
	}
	sort.Sort(HouseholdWithDevicesInfoViewSorting(d.Households))
}

type DeviceTriggerInfoView struct {
	ID         string              `json:"id"`
	Name       string              `json:"name"`
	Type       model.DeviceType    `json:"type"`
	IconURL    string              `json:"icon_url"`
	Properties []PropertyStateView `json:"properties"`
	Groups     []string            `json:"groups"`
	SkillID    string              `json:"skill_id"`
	Reportable bool                `json:"reportable"`
}

func (v *DeviceTriggerInfoView) FromDevice(device model.Device) {
	v.ID = device.ID
	v.Name = device.Name
	v.Type = device.Type
	v.IconURL = device.Type.IconURL(model.OriginalIconFormat)
	v.SkillID = device.SkillID

	properties := make([]PropertyStateView, 0, len(device.Properties))

	for _, property := range device.Properties {
		var propertyStateView PropertyStateView
		propertyStateView.FromProperty(property)
		properties = append(properties, propertyStateView)
	}

	v.Properties = properties
	v.Reportable = device.Properties.HaveReportableState()

	groups := make([]string, 0, len(device.Groups))
	for _, group := range device.Groups {
		groups = append(groups, group.Name)
	}
	v.Groups = groups
}

type DeviceTriggerListViewV2 struct {
	Status     string                            `json:"status"`
	RequestID  string                            `json:"request_id"`
	Households []HouseholdWithDeviceTriggersView `json:"households"`
}

type HouseholdWithDeviceTriggersView struct {
	HouseholdInfoView
	DeviceTriggerListView
}

func (v *DeviceTriggerListViewV2) From(currentHouseholdID string, households []model.Household, devices model.Devices) {
	devicesPerHouseholds := devices.GroupByHousehold()
	v.Households = make([]HouseholdWithDeviceTriggersView, 0, len(households))
	for _, h := range households {
		var view HouseholdWithDeviceTriggersView
		view.FromHousehold(h, currentHouseholdID)
		if devicesForHousehold, exists := devicesPerHouseholds[view.ID]; exists {
			view.FromDevices(devicesForHousehold)
		} else {
			view.FromDevices([]model.Device{})
		}
		v.Households = append(v.Households, view)
	}
	sort.Sort(HouseholdWithDeviceTriggersInfoViewSorting(v.Households))
}

type DeviceTriggerListResponse struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	DeviceTriggerListView
}

type DeviceTriggerListView struct {
	Rooms               []DeviceTriggerRoomInfoView `json:"rooms"`
	UnconfiguredDevices []DeviceTriggerInfoView     `json:"unconfigured_devices"`
}

type DeviceTriggerRoomInfoView struct {
	ID      string                  `json:"id"`
	Name    string                  `json:"name"`
	Devices []DeviceTriggerInfoView `json:"devices"`
}

func (v *DeviceTriggerListView) FromDevices(devices []model.Device) {
	v.Rooms = make([]DeviceTriggerRoomInfoView, 0)
	v.UnconfiguredDevices = make([]DeviceTriggerInfoView, 0)

	rooms := make(map[string]DeviceTriggerRoomInfoView)

	for _, device := range devices {
		if !device.IsAvailableForTrigger() {
			continue
		}

		var deviceInfoView DeviceTriggerInfoView
		deviceInfoView.FromDevice(device)

		if err := device.AssertSetup(); err != nil {
			v.UnconfiguredDevices = append(v.UnconfiguredDevices, deviceInfoView)
			continue
		}

		room, exists := rooms[device.Room.Name]
		if !exists {
			rooms[device.Room.Name] = DeviceTriggerRoomInfoView{
				ID:      device.Room.ID,
				Name:    device.Room.Name,
				Devices: make([]DeviceTriggerInfoView, 0),
			}
			room = rooms[device.Room.Name]
		}
		room.Devices = append(room.Devices, deviceInfoView)
		rooms[device.Room.Name] = room
	}

	for key := range rooms {
		sort.Sort(deviceTriggerInfoViewByName(rooms[key].Devices))
	}

	roomsList := make([]DeviceTriggerRoomInfoView, 0, len(rooms))
	for _, room := range rooms {
		roomsList = append(roomsList, room)
	}
	sort.Sort(deviceTriggerRoomInfoViewByName(roomsList))
	v.Rooms = roomsList

	sort.Sort(deviceTriggerInfoViewByName(v.UnconfiguredDevices))
}

type DeviceStateResult struct {
	Status     string              `json:"status,omitempty"`
	RequestID  string              `json:"request_id,omitempty"`
	UpdatesURL string              `json:"updates_url,omitempty"`
	DebugInfo  *recorder.DebugInfo `json:"debug,omitempty"`
	DeviceStateView
}

type DeviceStateView struct {
	ID           string                `json:"id"`
	Name         string                `json:"name"`
	Names        []string              `json:"names"`
	Type         model.DeviceType      `json:"type"`
	IconURL      string                `json:"icon_url"`
	State        model.DeviceStatus    `json:"state"`
	Groups       []string              `json:"groups"`
	Room         *string               `json:"room,omitempty"`
	Capabilities []CapabilityStateView `json:"capabilities"`
	Properties   []PropertyStateView   `json:"properties"`
	InfoMessage  string                `json:"info_message,omitempty"`
	SkillID      string                `json:"skill_id,omitempty"`    // not used in scenarios
	ExternalID   string                `json:"external_id,omitempty"` // not used in scenarios
	RenderInfo   *RenderInfoView       `json:"render_info,omitempty"`
	QuasarInfo   *QuasarInfo           `json:"quasar_info,omitempty"`
	SharingInfo  *SharingInfoView      `json:"sharing_info,omitempty"`
	Favorite     bool                  `json:"favorite"`
}

func (d *DeviceStateView) FromDevice(device model.Device, state model.DeviceStatus) {
	d.ID = device.ID
	d.Name = device.Name
	d.Names = append([]string{device.Name}, device.Aliases...)
	d.Type = device.Type
	d.IconURL = device.Type.IconURL(model.OriginalIconFormat)
	d.State = state
	d.SkillID = device.SkillID
	d.ExternalID = device.ExternalID

	if device.Room != nil {
		d.Room = &device.Room.Name
	}

	groups := make([]string, 0, len(device.Groups))
	for _, group := range device.Groups {
		groups = append(groups, group.Name)
	}
	d.Groups = groups
	d.InfoMessage = getInfoMessage(device)

	capabilities := make([]CapabilityStateView, 0, len(device.Capabilities))
	for _, capability := range device.Capabilities {
		// IOT-438: do not show quasar server action capabilities on device state
		if model.KnownQuasarCapabilityTypes.Contains(capability.Type()) {
			continue
		}
		var capabilityStateView CapabilityStateView
		capabilityStateView.FromCapability(capability)
		capabilities = append(capabilities, capabilityStateView)
	}
	sort.Sort(CapabilityStateViewSorting(capabilities))
	d.Capabilities = capabilities

	properties := make([]PropertyStateView, 0, len(device.Properties))
	for _, property := range device.Properties {
		var propertyStateView PropertyStateView
		propertyStateView.FromProperty(property)

		if state != model.OnlineDeviceStatus {
			propertyStateView.State = nil
		}
		properties = append(properties, propertyStateView)
	}
	d.Properties = properties
	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		d.QuasarInfo = &quasarInfo
	}
	d.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	d.SharingInfo = NewSharingInfoView(device.SharingInfo)
	d.Favorite = device.Favorite
}

func (d *DeviceStateView) FromDeviceForScenarios(device model.Device, state model.DeviceStatus) {
	d.ID = device.ID
	d.Name = device.Name
	d.Names = append([]string{device.Name}, device.Aliases...)
	d.Type = device.Type
	d.IconURL = device.Type.IconURL(model.OriginalIconFormat)
	d.State = state
	d.SkillID = device.SkillID
	d.ExternalID = device.ExternalID

	if device.Room != nil {
		d.Room = &device.Room.Name
	}

	groups := make([]string, 0, len(device.Groups))
	for _, group := range device.Groups {
		groups = append(groups, group.Name)
	}
	d.Groups = groups
	d.InfoMessage = getInfoMessage(device)

	capabilities := make([]CapabilityStateView, 0, len(device.Capabilities))
	for _, capability := range device.Capabilities {
		var capabilityStateView CapabilityStateView
		capabilityStateView.FromCapability(capability)
		capabilities = append(capabilities, capabilityStateView)
	}
	sort.Sort(CapabilityStateViewSorting(capabilities))
	d.Capabilities = capabilities

	properties := make([]PropertyStateView, 0, len(device.Properties))
	for _, property := range device.Properties {
		var propertyStateView PropertyStateView
		propertyStateView.FromProperty(property)

		if state != model.OnlineDeviceStatus {
			propertyStateView.State = nil
		}
		properties = append(properties, propertyStateView)
	}
	d.Properties = properties
	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		d.QuasarInfo = &quasarInfo
	}
	d.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	d.Favorite = device.Favorite
}

type StereopairView struct {
	Devices StereopairInfoItemViews `json:"devices"`
}

func (stereopairView *StereopairView) From(ctx context.Context, stereopair model.Stereopair) {
	for _, stereopairDeviceConfig := range stereopair.Config.Devices {
		device, ok := stereopair.Devices.GetDeviceByID(stereopairDeviceConfig.ID)
		if !ok {
			continue
		}

		var itemView ItemInfoView
		itemView.FromDevice(ctx, device)
		deviceView := StereopairInfoItemView{
			ItemInfoView:      itemView,
			StereopairRole:    stereopairDeviceConfig.Role,
			StereopairChannel: stereopairDeviceConfig.Channel,
		}
		stereopairView.Devices = append(stereopairView.Devices, deviceView)
	}
}

type StereopairInfoItemViews []StereopairInfoItemView

type StereopairInfoItemView struct {
	ItemInfoView
	StereopairRole    model.StereopairDeviceRole    `json:"stereopair_role"`
	StereopairChannel model.StereopairDeviceChannel `json:"stereopair_channel"`
}

type DeviceCapabilitiesResult struct {
	Status       string                `json:"status,omitempty"`
	RequestID    string                `json:"request_id,omitempty"`
	ID           string                `json:"id"`
	Name         string                `json:"name"`
	Type         model.DeviceType      `json:"type"`
	QuasarInfo   *QuasarInfo           `json:"quasar_info,omitempty"`
	RenderInfo   *RenderInfoView       `json:"render_info,omitempty"`
	Capabilities []CapabilityStateView `json:"capabilities"`
}

func (d *DeviceCapabilitiesResult) FromDevice(ctx context.Context, device model.Device) {
	d.ID = device.ID
	d.Name = device.Name
	d.Type = device.Type
	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		d.QuasarInfo = &quasarInfo
	}
	d.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	capabilities := make([]CapabilityStateView, 0, len(device.Capabilities))

	deviceCapabilities := device.Capabilities.Clone()
	if experiments.DeduplicateQuasarCapabilities.IsEnabled(ctx) {
		_, hasPhraseAction := deviceCapabilities.GetCapabilityByTypeAndInstance(model.QuasarServerActionCapabilityType, string(model.PhraseActionCapabilityInstance))
		_, hasTTS := deviceCapabilities.GetCapabilityByTypeAndInstance(model.QuasarCapabilityType, string(model.TTSCapabilityInstance))
		if hasPhraseAction && hasTTS {
			deviceCapabilities = deviceCapabilities.DropByTypeAndInstance(model.QuasarServerActionCapabilityType, string(model.PhraseActionCapabilityInstance))
		}
	}
	for _, capability := range deviceCapabilities {
		var capabilityStateView CapabilityStateView
		capability.SetState(capability.DefaultState())
		capabilityStateView.FromCapability(capability)
		capabilities = append(capabilities, capabilityStateView)
	}
	sort.Sort(CapabilityStateViewSorting(capabilities))
	d.Capabilities = capabilities
}

func getInfoMessage(device model.Device) string {
	if device.SkillID == model.XiaomiSkill {
		var cd xiaomi.CustomData
		if err := mapstructure.Decode(device.CustomData, &cd); err == nil {
			if cd.Region == "china" {
				return model.SlowConnection
			}
		}
	}
	return ""
}

type ActionResult struct {
	RequestID string                   `json:"request_id"`
	Status    string                   `json:"status"`
	Debug     *recorder.DebugInfo      `json:"debug,omitempty"`
	Devices   []DeviceActionResultView `json:"devices,omitempty"`
}

type DeviceActionResultView struct {
	ID           string                       `json:"id"`
	Capabilities []CapabilityActionResultView `json:"capabilities"`
}

func NewDeviceActionResultView(id string, carvMap map[string]adapter.CapabilityActionResultView) DeviceActionResultView {
	darv := DeviceActionResultView{
		ID:           id,
		Capabilities: make([]CapabilityActionResultView, 0, len(carvMap)),
	}
	for _, capability := range carvMap {
		darv.Capabilities = append(darv.Capabilities, CapabilityActionResultView{
			Type: capability.Type,
			State: CapabilityStateActionResultView{
				Instance: capability.State.Instance,
				Value:    capability.State.Value,
				ActionResult: StateActionResult{
					Status:       capability.State.ActionResult.Status,
					ErrorCode:    capability.State.ActionResult.ErrorCode,
					ErrorMessage: capability.State.ActionResult.ErrorMessage,
				},
			},
		})
	}
	return darv
}

type CapabilityActionResultView struct {
	Type  model.CapabilityType            `json:"type"`
	State CapabilityStateActionResultView `json:"state"`
}

type CapabilityStateActionResultView struct {
	Instance     string            `json:"instance"`
	Value        interface{}       `json:"value,omitempty"`
	ActionResult StateActionResult `json:"action_result"`
}

type StateActionResult struct {
	Status       adapter.ActionStatus `json:"status"`
	ErrorCode    adapter.ErrorCode    `json:"error_code,omitempty"`
	ErrorMessage string               `json:"error_message,omitempty"`
}

type ActionRequest struct {
	Actions []CapabilityActionView
}

func (ar *ActionRequest) ToCapabilities(d model.Device) model.Capabilities {
	capabilities := make([]model.ICapability, 0, len(ar.Actions))
	for _, action := range ar.Actions {
		if capability := action.ToCapability(d); capability != nil {
			capabilities = append(capabilities, capability)
		}
	}
	return capabilities
}

type CapabilityActionView struct {
	Type  model.CapabilityType `json:"type"`
	State struct {
		Instance string      `json:"instance"`
		Relative *bool       `json:"relative,omitempty"`
		Value    interface{} `json:"value"`
	} `json:"state"`
}

type ColorSettingCapabilityStateActionView struct {
	Instance string `json:"instance"`
	Value    string `json:"value"`
}

func (c *CapabilityActionView) UnmarshalJSON(b []byte) (err error) {
	cRaw := struct {
		Type  model.CapabilityType
		State json.RawMessage
	}{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}

	c.Type = cRaw.Type
	switch c.Type {
	case model.OnOffCapabilityType:
		s := model.OnOffCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		c.State.Instance = string(s.Instance)
		c.State.Value = s.Value
		c.State.Relative = s.Relative
	case model.ColorSettingCapabilityType:
		s := ColorSettingCapabilityStateActionView{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		c.State.Instance = s.Instance
		if c.State.Instance == string(model.SceneCapabilityInstance) {
			c.State.Value = model.ColorSceneID(s.Value)
		} else {
			c.State.Value = model.ColorID(s.Value)
		}

	case model.ModeCapabilityType:
		s := model.ModeCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		c.State.Instance = string(s.Instance)
		c.State.Value = s.Value

	case model.RangeCapabilityType:
		s := model.RangeCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		c.State.Instance = string(s.Instance)
		c.State.Value = s.Value
		c.State.Relative = s.Relative

	case model.ToggleCapabilityType:
		s := model.ToggleCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		c.State.Instance = string(s.Instance)
		c.State.Value = s.Value

	case model.CustomButtonCapabilityType:
		s := model.CustomButtonCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		c.State.Instance = string(s.Instance)
		c.State.Value = s.Value

	case model.QuasarServerActionCapabilityType:
		s := model.QuasarServerActionCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		c.State.Instance = string(s.Instance)
		c.State.Value = s.Value

	case model.QuasarCapabilityType:
		s := model.QuasarCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		c.State.Instance = string(s.Instance)
		c.State.Value = s.Value

	case model.VideoStreamCapabilityType:
		s := model.VideoStreamCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		c.State.Instance = string(s.Instance)
		c.State.Value = s.Value

	default:
		err = fmt.Errorf("unknown capability type: %s", cRaw.Type)
	}

	if err != nil {
		return xerrors.Errorf("cannot parse capability: %w", err)
	}
	return nil
}

func (c *CapabilityActionView) ToCapability(d model.Device) model.ICapability {
	capability := model.MakeCapabilityByType(c.Type)
	switch c.Type {
	case model.OnOffCapabilityType:
		capability.SetState(model.OnOffCapabilityState{
			Instance: model.OnOffCapabilityInstance(c.State.Instance),
			Value:    c.State.Value.(bool),
			Relative: c.State.Relative,
		})
	case model.RangeCapabilityType:
		capability.SetState(model.RangeCapabilityState{
			Instance: model.RangeCapabilityInstance(c.State.Instance),
			Value:    c.State.Value.(float64),
			Relative: c.State.Relative,
		})
	case model.ModeCapabilityType:
		capability.SetState(model.ModeCapabilityState{
			Instance: model.ModeCapabilityInstance(c.State.Instance),
			Value:    c.State.Value.(model.ModeValue),
		})
	case model.ColorSettingCapabilityType:
		colorCapabilities := d.GetCapabilitiesByType(model.ColorSettingCapabilityType)
		if len(colorCapabilities) == 0 {
			return nil
		}
		params, ok := colorCapabilities[0].Parameters().(model.ColorSettingCapabilityParameters)
		if !ok {
			return nil
		}
		switch c.State.Instance {
		case string(model.SceneCapabilityInstance):
			sceneColorID, ok := c.State.Value.(model.ColorSceneID)
			if !ok {
				return nil
			}
			scenesMap := params.GetAvailableScenes().AsMap()
			if scene, exist := scenesMap[sceneColorID]; exist {
				capability.SetState(scene.ToColorSettingCapabilityState())
			} else {
				return nil
			}
		case ColorCapabilityInstance:
			color, exists := model.ColorPalette[c.State.Value.(model.ColorID)]
			if !exists {
				return nil
			}
			capability.SetState(color.ToColorSettingCapabilityState(
				params.GetColorSettingCapabilityInstance()))
		}
	case model.ToggleCapabilityType:
		capability.SetState(model.ToggleCapabilityState{
			Instance: model.ToggleCapabilityInstance(c.State.Instance),
			Value:    c.State.Value.(bool),
		})
	case model.CustomButtonCapabilityType:
		capability.SetState(model.CustomButtonCapabilityState{
			Instance: model.CustomButtonCapabilityInstance(c.State.Instance),
			Value:    c.State.Value.(bool),
		})
	case model.QuasarServerActionCapabilityType:
		capability.SetState(model.QuasarServerActionCapabilityState{
			Instance: model.QuasarServerActionCapabilityInstance(c.State.Instance),
			Value:    c.State.Value.(string),
		})
	case model.QuasarCapabilityType:
		capability.SetState(model.QuasarCapabilityState{
			Instance: model.QuasarCapabilityInstance(c.State.Instance),
			Value:    model.MakeQuasarCapabilityValueByInstance(model.QuasarCapabilityInstance(c.State.Instance), c.State.Value),
		})
	case model.VideoStreamCapabilityType:
		capability.SetState(model.VideoStreamCapabilityState{
			Instance: model.VideoStreamCapabilityInstance(c.State.Instance),
			Value:    c.State.Value.(model.VideoStreamCapabilityValue),
		})
	}
	if actualCapability, exist := d.GetCapabilityByTypeAndInstance(capability.Type(), capability.Instance()); exist {
		clonedCapability := actualCapability.Clone()
		clonedCapability.SetState(capability.State())
		return clonedCapability
	}
	return capability
}

func (c *CapabilityActionView) ToQuasarCapability() model.ICapability {
	switch c.Type {
	case model.QuasarServerActionCapabilityType:
		capability := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		instance := model.QuasarServerActionCapabilityInstance(c.State.Instance)
		capability.SetState(model.QuasarServerActionCapabilityState{
			Instance: instance,
			Value:    c.State.Value.(string),
		})
		capability.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: instance})
		return capability
	case model.QuasarCapabilityType:
		capability := model.MakeCapabilityByType(model.QuasarCapabilityType)
		instance := model.QuasarCapabilityInstance(c.State.Instance)
		capability.SetState(model.QuasarCapabilityState{
			Instance: instance,
			Value:    model.MakeQuasarCapabilityValueByInstance(instance, c.State.Value),
		})
		capability.SetParameters(model.QuasarCapabilityParameters{Instance: instance})
		return capability
	default:
		return nil
	}
}

func (c CapabilityActionView) ToScenarioCreateRequestStep() ScenarioCreateRequestStep {
	return ScenarioCreateRequestStep{
		Type: model.ScenarioStepActionsType,
		Parameters: ScenarioCreateRequestStepActionsParameters{
			LaunchDevices:                make(ScenarioCreateDevices, 0),
			RequestedSpeakerCapabilities: ScenarioCreateCapabilities{c},
		},
	}
}

func (c *CapabilityActionView) IsOfTypeAndInstance(cType model.CapabilityType, cInstance string) bool {
	isOfType := c.Type == cType
	isOfInstance := c.State.Instance == cInstance
	return isOfType && isOfInstance
}

type DeviceActionView struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	Message   string `json:"message,omitempty"` //TODO: remove this
}

type DeviceConfigureView struct {
	Status              string           `json:"status"`
	RequestID           string           `json:"request_id"`
	ID                  string           `json:"id"`
	Name                string           `json:"name"`
	NameValidationError *string          `json:"name_validation_error,omitempty"`
	Names               []string         `json:"names"`
	Groups              []string         `json:"groups"`
	ChildDeviceIDs      []string         `json:"child_device_ids"`
	ParentDeviceID      string           `json:"parent_device_id,omitempty"`
	RoomValidationError *string          `json:"room_validation_error,omitempty"`
	SkillID             string           `json:"skill_id"`
	DeviceInfo          DeviceInfo       `json:"device_info,omitempty"`
	Favorite            bool             `json:"favorite"`
	InfraredInfo        *InfraredInfo    `json:"infrared_info,omitempty"`
	ExternalName        string           `json:"external_name"`
	ExternalID          string           `json:"external_id"`
	Type                *string          `json:"type,omitempty"`
	OriginalType        model.DeviceType `json:"original_type,omitempty"`
	FwUpgradable        bool             `json:"fw_upgradable"`
	RenderInfo          *RenderInfoView  `json:"render_info,omitempty"`
	SharingInfo         *SharingInfoView `json:"sharing_info,omitempty"`
	*DeviceQuasarConfigureView
}

type DeviceConfigureV1View struct {
	DeviceConfigureView
	RoomName      string `json:"room"`
	HouseholdName string `json:"household"`
}

func (d *DeviceConfigureV1View) FromDevice(
	ctx context.Context,
	device model.Device,
	userDevices model.Devices,
	household model.Household,
	deviceInfos quasarconfig.DeviceInfos,
	stereopairs model.Stereopairs,
	voiceprintDeviceConfigs settings.VoiceprintDeviceConfigs,
) {
	d.DeviceConfigureView.FromDevice(ctx, device, userDevices, deviceInfos, stereopairs, voiceprintDeviceConfigs)
	// if the device.Room is nil, the error was handled in DeviceConfigureView.FromDevice
	if device.Room != nil {
		d.RoomName = device.Room.Name
	}
	d.HouseholdName = household.Name
}

type DeviceConfigureV2View struct {
	DeviceConfigureView
	Room      *DeviceConfigureViewRoom     `json:"room,omitempty"`
	Household DeviceConfigureViewHousehold `json:"household"`
}

func (d *DeviceConfigureV2View) FromDevice(
	ctx context.Context,
	device model.Device,
	userDevices model.Devices,
	household model.Household,
	deviceInfos quasarconfig.DeviceInfos,
	stereopairs model.Stereopairs,
	voiceprintDeviceConfigs settings.VoiceprintDeviceConfigs) {
	d.DeviceConfigureView.FromDevice(ctx, device, userDevices, deviceInfos, stereopairs, voiceprintDeviceConfigs)
	// if the device.Room is nil, the error was handled in DeviceConfigureView.FromDevice
	if device.Room != nil {
		var view DeviceConfigureViewRoom
		view.FromRoom(*device.Room)
		d.Room = &view
	}
	var view DeviceConfigureViewHousehold
	view.FromHousehold(household)
	d.Household = view
}

type DeviceConfigureViewHousehold struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}

func (d *DeviceConfigureViewHousehold) FromHousehold(household model.Household) {
	d.ID = household.ID
	d.Name = household.Name
}

type DeviceConfigureViewRoom struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}

func (d *DeviceConfigureViewRoom) FromRoom(room model.Room) {
	d.ID = room.ID
	d.Name = room.Name
}

type DeviceSuggestionsView struct {
	Status           string            `json:"status"`
	RequestID        string            `json:"request_id"`
	SuggestionBlocks []SuggestionBlock `json:"commands"`
}

func (d *DeviceConfigureView) FromDevice(
	ctx context.Context,
	device model.Device,
	userDevices model.Devices,
	deviceInfos quasarconfig.DeviceInfos,
	stereopairs model.Stereopairs,
	voiceprintDeviceConfigs settings.VoiceprintDeviceConfigs,
) {
	d.ID = device.ID

	if stereopair, exist := stereopairs.GetByDeviceID(device.ID); exist {
		d.Name = stereopair.Name
	} else {
		d.Name = device.Name
	}
	d.Names = append([]string{d.Name}, device.Aliases...)

	if err := device.AssertName(); err != nil {
		var nameLengthError *model.NameLengthError
		switch {
		case xerrors.Is(err, &model.QuasarNameCharError{}):
			validationError := model.RenameToRussianAndLatinErrorMessage
			d.NameValidationError = &validationError
		case xerrors.Is(err, &model.NameCharError{}):
			validationError := model.RenameToRussianErrorMessage
			d.NameValidationError = &validationError
		case xerrors.As(err, &nameLengthError):
			validationError := fmt.Sprintf(model.NameLengthErrorMessage, nameLengthError.Limit)
			d.NameValidationError = &validationError
		case xerrors.Is(err, &model.NameMinLettersError{}):
			validationError := model.NameMinLettersErrorMessage
			d.NameValidationError = &validationError
		case xerrors.Is(err, &model.NameEmptyError{}):
			validationError := model.NameEmptyErrorMessage
			d.NameValidationError = &validationError
		}
	}

	d.DeviceInfo.FromDeviceInfo(ctx, device.DeviceInfo, device.CustomData)

	d.ExternalID = device.ExternalID

	d.ChildDeviceIDs = make([]string, 0)
	if device.IsQuasarDevice() {
		var quasarView DeviceQuasarConfigureView
		quasarView.From(ctx, device, userDevices, deviceInfos, stereopairs, voiceprintDeviceConfigs)
		d.DeviceQuasarConfigureView = &quasarView
		d.ChildDeviceIDs = (model.QuasarDevice)(device).ChildDevices(userDevices).GetIDs()
	}

	if device.SkillID == model.YANDEXIO {
		if parentDevice, ok := model.YandexIODevice(device).GetParent(userDevices); ok {
			d.ParentDeviceID = parentDevice.ID
		}
	}

	d.ExternalName = device.ExternalName

	groups := make([]string, 0, len(device.Groups))
	for _, group := range device.Groups {
		groups = append(groups, group.Name)
	}
	sort.Strings(groups)
	d.Groups = groups

	room := UserRoomView{}
	if device.Room != nil {
		room.FromRoom(*device.Room)
	} else {
		room.ValidationError = tools.AOS(model.DeviceHasNoRoomErrorMessage)
	}

	d.RoomValidationError = room.ValidationError

	d.SkillID = device.SkillID

	if _, switchable := model.DeviceSwitchTypeMap[device.OriginalType]; switchable {
		d.Type = tools.AOS(deviceTypeNameMap[device.Type])
	}

	d.OriginalType = device.OriginalType
	d.Favorite = device.Favorite

	if d.SkillID == model.TUYA && (d.OriginalType == model.HubDeviceType || d.OriginalType == model.LightDeviceType) {
		d.FwUpgradable = true
	}
	if d.SkillID == model.TUYA {
		var irCustomData btuya.CustomData
		if err := mapstructure.Decode(device.CustomData, &irCustomData); err == nil && irCustomData.InfraredData != nil {
			d.InfraredInfo = &InfraredInfo{
				Learned:       irCustomData.InfraredData.Learned,
				TransmitterID: irCustomData.InfraredData.TransmitterID,
			}
		}
	}
	d.SharingInfo = NewSharingInfoView(device.SharingInfo)
}

type QuasarInfo struct {
	DeviceID                    string                  `json:"device_id"`
	Platform                    string                  `json:"platform"`
	MultiroomAvailable          bool                    `json:"multiroom_available"`
	MultistepScenariosAvailable bool                    `json:"multistep_scenarios_available"`
	DeviceDiscoveryMethods      []model.DiscoveryMethod `json:"device_discovery_methods"`
}

func (q *QuasarInfo) FromCustomData(customData interface{}, deviceType model.DeviceType) {
	var cd quasar.CustomData
	if err := mapstructure.Decode(customData, &cd); err == nil {
		q.DeviceID = cd.DeviceID
		q.Platform = cd.Platform
		q.MultiroomAvailable = model.MultiroomSpeakers[deviceType]
		q.MultistepScenariosAvailable = model.MultistepScenarioSpeakers[deviceType]
		q.DeviceDiscoveryMethods = deviceType.GetDeviceDiscoveryMethods()
	}
}

type InfraredInfo struct {
	Learned       bool   `json:"learned"`
	TransmitterID string `json:"transmitter_id"`
}

type DeviceInfo struct {
	Manufacturer      *string                   `json:"manufacturer,omitempty"`
	Model             *string                   `json:"model,omitempty"`
	HwVersion         *string                   `json:"hw_version,omitempty"`
	SwVersion         *string                   `json:"sw_version,omitempty"`
	UpdateChangelogID *tuya.FirmwareChangelogID `json:"update_changelog_id,omitempty"`
}

func (di *DeviceInfo) FromDeviceInfo(ctx context.Context, deviceInfo *model.DeviceInfo, customData interface{}) {
	if deviceInfo != nil {
		di.Manufacturer = deviceInfo.Manufacturer
		di.Model = deviceInfo.Model
		di.HwVersion = deviceInfo.HwVersion
		di.SwVersion = deviceInfo.SwVersion
		di.UpdateChangelogID = tuya.KnownFirmwareChangelogs.GetAvailableChangelog(ctx, customData, deviceInfo.SwVersion)
	}
}

type DeviceNameEditView struct {
	Status          string   `json:"status"`
	RequestID       string   `json:"request_id"`
	Message         string   `json:"message,omitempty"`
	ID              string   `json:"id"`
	Name            string   `json:"name"`
	ValidationError *string  `json:"validation_error,omitempty"`
	Suggests        []string `json:"suggests"`
	Removable       bool     `json:"removable"`
}

func (dev *DeviceNameEditView) FromDevice(device model.Device, editableName string) {
	dev.ID = device.ID
	dev.Name = device.Name

	if device.AssertName() != nil {
		var validationFailureMessage string
		if device.IsQuasarDevice() {
			validationFailureMessage = model.RenameToRussianAndLatinErrorMessage
		} else {
			validationFailureMessage = model.RenameToRussianErrorMessage
		}
		dev.ValidationError = &validationFailureMessage
	}

	suggests := suggestions.DefaultDeviceNames
	if v, ok := suggestions.DeviceNames[device.Type]; ok {
		suggests = v
	}
	dev.Suggests = suggests

	dev.Removable = false
	if editableName != "" && len(device.Aliases) > 0 {
		dev.Removable = true
	}
}

type UserHubDevicesView struct {
	Status    string           `json:"status"`
	RequestID string           `json:"request_id"`
	Devices   []DeviceInfoView `json:"devices"`
}

func (uhdv *UserHubDevicesView) PopulateDevices(devices []model.Device) {
	hubDevices := make([]DeviceInfoView, 0, len(devices))
	for _, device := range devices {
		d := DeviceInfoView{}
		d.FromDevice(device)
		hubDevices = append(hubDevices, d)
	}
	uhdv.Devices = hubDevices
}

type DeviceDiscoveryResultView struct {
	Status    string              `json:"status"`
	RequestID string              `json:"request_id"`
	DebugInfo *recorder.DebugInfo `json:"debug,omitempty"`

	NewDeviceCount     int                   `json:"new_device_count"`
	UpdatedDeviceCount int                   `json:"updated_device_count"`
	LimitDeviceCount   int                   `json:"limit_device_count"`
	ErrorDeviceCount   int                   `json:"error_device_count"`
	NewDevices         []ShortDeviceInfoView `json:"new_devices"`
}

type ShortDeviceInfoView struct {
	ID   string           `json:"id"`
	Name string           `json:"name"`
	Type model.DeviceType `json:"type"`
}

func (div *ShortDeviceInfoView) FromDeviceStoreResult(dsr model.DeviceStoreResult) {
	div.ID = dsr.ID
	div.Name = dsr.Name
	div.Type = dsr.Type
}

func (ddrv *DeviceDiscoveryResultView) FromDeviceStoreResults(dsrs []model.DeviceStoreResult) {
	ddrv.NewDevices = make([]ShortDeviceInfoView, 0, len(dsrs))
	for _, dsr := range dsrs {
		switch dsr.Result {
		case model.StoreResultNew:
			ddrv.NewDeviceCount++
			var newDevice ShortDeviceInfoView
			newDevice.FromDeviceStoreResult(dsr)
			ddrv.NewDevices = append(ddrv.NewDevices, newDevice)
		case model.StoreResultUpdated:
			ddrv.UpdatedDeviceCount++
		case model.StoreResultLimitReached:
			ddrv.LimitDeviceCount++
		default:
			ddrv.ErrorDeviceCount++
		}
	}
}

type UpdateStatesEvent struct {
	UpdatedDevices []DevicePartialStateView `json:"updated_devices"`
	UpdateGroups   []GroupPartialStateView  `json:"update_groups"`
}

type GroupPartialStateView struct {
	ID           string                `json:"id"`
	State        model.DeviceStatus    `json:"state,omitempty"`
	Capabilities []CapabilityStateView `json:"capabilities,omitempty"`
}

type DevicePartialStateView struct {
	ID           string                `json:"id"`
	Capabilities []CapabilityStateView `json:"capabilities,omitempty"`
	Properties   []PropertyStateView   `json:"properties,omitempty"`
}

func (dpsv *DevicePartialStateView) FromDeviceState(deviceID string, capabilities model.Capabilities, properties model.Properties) {
	dpsv.ID = deviceID

	caps := make([]CapabilityStateView, 0, len(capabilities))
	for _, capability := range capabilities {
		var capabilityStateView CapabilityStateView
		capabilityStateView.FromCapability(capability)
		caps = append(caps, capabilityStateView)
	}
	dpsv.Capabilities = caps

	props := make([]PropertyStateView, 0, len(properties))
	for _, property := range properties {
		var propertyStateView PropertyStateView
		propertyStateView.FromProperty(property)
		props = append(props, propertyStateView)
	}
	dpsv.Properties = props
}

type DeviceAddNameRequest struct {
	Name string `json:"name"`
}

type DeviceChangeNameRequest struct {
	OldName string `json:"old_name"`
	NewName string `json:"new_name"`
}

type DeviceDeleteNameRequest struct {
	Name string `json:"name"`
}

type DevicesListPrefetchResponse struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
}

type DevicesMoveToHouseholdRequest struct {
	DevicesID   []string `json:"device_ids"`
	HouseholdID string   `json:"household_id"`
}
