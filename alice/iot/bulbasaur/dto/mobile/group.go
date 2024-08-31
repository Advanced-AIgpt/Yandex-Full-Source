package mobile

import (
	"context"
	"sort"
	"strings"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid"
)

type GroupInfoView struct {
	ID           string              `json:"id"`
	Name         string              `json:"name"`
	Type         model.DeviceType    `json:"type"`
	IconURL      string              `json:"icon_url"`
	State        model.DeviceStatus  `json:"state"`
	Capabilities []model.ICapability `json:"capabilities"`
	DevicesCount int                 `json:"devices_count"`
}

func GroupInfoViewFromDevices(devices []model.Device) []GroupInfoView {
	groupsInfoView := make([]GroupInfoView, 0)
	groups := make(map[string]GroupInfoView)
	devicesInGroups := make(map[string][]model.Device)

	for _, device := range devices {
		for _, deviceGroup := range device.Groups {
			if group, exists := groups[deviceGroup.ID]; exists {
				group.DevicesCount += 1
				groups[deviceGroup.ID] = group
			} else {
				groups[deviceGroup.ID] = GroupInfoView{
					ID:           deviceGroup.ID,
					Name:         deviceGroup.Name,
					Type:         deviceGroup.Type,
					IconURL:      deviceGroup.Type.IconURL(model.OriginalIconFormat),
					State:        model.OnlineDeviceStatus,
					Capabilities: make([]model.ICapability, 0),
					DevicesCount: 1,
				}
			}

			devicesInGroups[deviceGroup.ID] = append(devicesInGroups[deviceGroup.ID], device)
		}
	}

	for groupID, group := range groups {
		if devicesInGroup, exists := devicesInGroups[groupID]; exists {
			group.State = model.GroupOnOffStateFromDevices(devicesInGroup)

			if groupOnOffCapability, ok := model.GetGroupCapabilityFromDevicesByTypeAndInstance(devicesInGroup, model.OnOffCapabilityType, string(model.OnOnOffCapabilityInstance)); ok {
				group.Capabilities = append(group.Capabilities, groupOnOffCapability)
			}
		}

		groupsInfoView = append(groupsInfoView, group)
	}

	// Sorting groups by names
	sort.Slice(groupsInfoView, func(i, j int) bool {
		return strings.ToLower(groupsInfoView[i].Name) < strings.ToLower(groupsInfoView[j].Name)
	})

	return groupsInfoView
}

type UserGroupView struct {
	ID      string           `json:"id"`
	Name    string           `json:"name"`
	Type    model.DeviceType `json:"type"`
	IconURL string           `json:"icon_url"`
	Active  *bool            `json:"active,omitempty"`
	devices []string
}

func (ugv *UserGroupView) FromGroup(group model.Group) {
	ugv.ID = group.ID
	ugv.Name = group.Name
	ugv.Type = group.Type
	ugv.IconURL = group.Type.IconURL(model.OriginalIconFormat)
	ugv.devices = group.Devices
}

type UserGroupsView struct {
	Status    string          `json:"status"`
	RequestID string          `json:"request_id"`
	Groups    []UserGroupView `json:"groups"`
}

func (ugv *UserGroupsView) FromGroups(groups []model.Group) {
	userGroups := make([]UserGroupView, 0, len(groups))
	for _, group := range groups {
		userGroup := UserGroupView{}
		userGroup.FromGroup(group)
		userGroups = append(userGroups, userGroup)
	}
	sort.Sort(userGroupViewByName(userGroups))
	ugv.Groups = userGroups
}

func (ugv *UserGroupsView) FilterForDevice(device model.Device) {
	deviceGroupsIDs := make([]string, 0, len(device.Groups))
	for _, group := range device.Groups {
		deviceGroupsIDs = append(deviceGroupsIDs, group.ID)
	}

	groups := make([]UserGroupView, 0, len(ugv.Groups))
	for _, group := range ugv.Groups {
		if device.Type.IsSmartSpeaker() && !model.MultiroomSpeakers[device.Type] {
			continue
		}
		isMultiroomGroup := group.Type == model.SmartSpeakerDeviceType && model.MultiroomSpeakers[device.Type]
		isDeviceTypeGroup := group.Type == device.Type
		isEmptyGroup := group.Type == ""

		// group is shown for device if
		// 1. it has same device type
		// 2. it is empty
		// 3. it is speaker multiroom group
		if isDeviceTypeGroup || isEmptyGroup || isMultiroomGroup {
			active := tools.Contains(group.ID, deviceGroupsIDs)
			group.Active = &active
			groups = append(groups, group)
		}
	}
	ugv.Groups = groups
}

func (ugv *UserGroupsView) MarkDeviceGroups(device model.Device) {
	for i, group := range ugv.Groups {
		active := false
		if tools.Contains(device.ID, group.devices) {
			active = true
		}
		ugv.Groups[i].Active = &active
	}
}

type GroupEditView struct {
	Status          string                `json:"status"`
	RequestID       string                `json:"request_id"`
	Message         string                `json:"message,omitempty"`
	ID              string                `json:"id"`
	Name            string                `json:"name"`
	ValidationError *string               `json:"validation_error,omitempty"`
	Suggests        []string              `json:"suggests"`
	Devices         []DeviceGroupEditView `json:"devices"`
}

func (gev *GroupEditView) From(ctx context.Context, group model.Group, groupDevices model.Devices, stereopairs model.Stereopairs) {
	gev.ID = group.ID
	gev.Name = group.Name
	gev.Suggests = suggestions.GroupNameSuggests(group.Type)

	groupDevicesMap := make(map[string]bool)
	for _, deviceID := range group.Devices {
		groupDevicesMap[deviceID] = true
	}
	gev.Devices = make([]DeviceGroupEditView, 0, len(groupDevices))
	for _, d := range groupDevices {
		if !groupDevicesMap[d.ID] {
			continue
		}
		var view DeviceGroupEditView
		switch stereopairs.GetDeviceRole(d.ID) {
		case model.LeaderRole:
			stereopair, _ := stereopairs.GetByDeviceID(d.ID)
			view.FromStereopair(ctx, stereopair)
		case model.FollowerRole:
			// skip
			continue
		default:
			view.FromDevice(d)
		}
		gev.Devices = append(gev.Devices, view)
	}
}

type GroupCreateView struct {
	Status    string   `json:"status"`
	RequestID string   `json:"request_id"`
	Suggests  []string `json:"suggests"`
}

type GroupStateView struct {
	Status       string                 `json:"status"`
	RequestID    string                 `json:"request_id"`
	ID           string                 `json:"id"`
	Name         string                 `json:"name"`
	Type         model.DeviceType       `json:"type"`
	IconURL      string                 `json:"icon_url"`
	State        model.DeviceStatus     `json:"state"`
	Capabilities []CapabilityStateView  `json:"capabilities"`
	Devices      []GroupStateDeviceView `json:"devices"`
	Favorite     bool                   `json:"favorite"`

	// https://st.yandex-team.ru/IOT-993#60f69abf61e37423d15bfd87 new fields
	UnconfiguredDevices []GroupStateDeviceView `json:"unconfigured_devices"`
	Rooms               []GroupStateRoomView   `json:"rooms"`
}

func (gsv *GroupStateView) FromDevices(ctx context.Context, devices model.Devices, stereopairs model.Stereopairs) {
	gsv.State = model.GroupOnOffStateFromDevices(devices)

	groupedCapabilities := model.GroupCapabilitiesFromDevices(devices)
	gsv.Capabilities = make([]CapabilityStateView, 0, len(groupedCapabilities))
	for _, groupedCapability := range groupedCapabilities {
		var csv CapabilityStateView
		csv.Split = (groupedCapability.State() == nil) && groupedCapability.Retrievable()
		csv.FromCapability(groupedCapability)
		gsv.Capabilities = append(gsv.Capabilities, csv)
	}

	gsv.Devices = make([]GroupStateDeviceView, 0, len(devices))
	for _, device := range devices {
		var gsdv GroupStateDeviceView
		switch stereopairs.GetDeviceRole(device.ID) {
		case model.LeaderRole:
			stereopair, _ := stereopairs.GetByDeviceID(device.ID)
			gsdv.FromStereopair(ctx, stereopair)
		case model.FollowerRole:
			continue
		default:
			gsdv.FromDevice(device)
		}
		gsv.Devices = append(gsv.Devices, gsdv)
	}

	sort.Sort(GroupStateDeviceViewSorting(gsv.Devices))

	gsv.UnconfiguredDevices = make([]GroupStateDeviceView, 0, len(devices))
	gsv.Rooms = make([]GroupStateRoomView, 0)
	roomsMap := make(map[string]GroupStateRoomView)
	for _, device := range devices {
		var gsdv GroupStateDeviceView
		switch stereopairs.GetDeviceRole(device.ID) {
		case model.LeaderRole:
			stereopair, _ := stereopairs.GetByDeviceID(device.ID)
			gsdv.FromStereopair(ctx, stereopair)
		case model.FollowerRole:
			continue
		default:
			gsdv.FromDevice(device)
		}
		if device.Room == nil {
			gsv.UnconfiguredDevices = append(gsv.UnconfiguredDevices, gsdv)
			continue
		}
		roomView, exist := roomsMap[device.RoomID()]
		if !exist {
			roomView = GroupStateRoomView{}
			roomView.FromRoom(*device.Room)
		}
		roomView.Devices = append(roomView.Devices, gsdv)
		roomsMap[device.RoomID()] = roomView
	}
	for _, roomView := range roomsMap {
		sort.Sort(GroupStateDeviceViewSorting(roomView.Devices))
		gsv.Rooms = append(gsv.Rooms, roomView)
	}

	sort.Sort(GroupStateDeviceViewSorting(gsv.UnconfiguredDevices))
	sort.Sort(GroupStateRoomViewSorting(gsv.Rooms))
}

type GroupSuggestionsView struct {
	Status      string   `json:"status"`
	RequestID   string   `json:"request_id"`
	Suggestions []string `json:"suggestions"`
}

var _ valid.Validator = new(GroupName)

type GroupName string

func (gn GroupName) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors

	group := model.Group{Name: string(gn)}
	if err := group.AssertName(); err != nil {
		verrs = append(verrs, err)
	}
	if len(verrs) != 0 {
		return false, verrs
	}
	return false, nil
}

type GroupCreateRequest struct {
	Name        GroupName `json:"name"`
	Devices     []string  `json:"devices"`
	HouseholdID *string   `json:"household_id,omitempty"`
}

func (req GroupCreateRequest) ToGroup() model.Group {
	var householdID string
	if req.HouseholdID != nil {
		householdID = *req.HouseholdID
	}
	return model.Group{
		Name:        string(req.Name),
		Devices:     req.Devices,
		HouseholdID: householdID,
	}
}

type GroupUpdateRequest struct {
	Name    *GroupName `json:"name,omitempty"`
	Devices []string   `json:"devices"`
}

func (req GroupUpdateRequest) ApplyOnGroup(group model.Group) model.Group {
	res := group
	if req.Name != nil {
		res.Name = string(*req.Name)
	}
	if req.Devices != nil {
		res.Devices = req.Devices
	}
	return res
}

func (req GroupUpdateRequest) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors
	if req.Name != nil {
		if _, e := req.Name.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				verrs = append(verrs, ves...)
			} else {
				verrs = append(verrs, e)
			}
		}
	}
	if len(verrs) != 0 {
		return false, verrs
	}
	return false, nil
}

type GroupAvailableDevicesView struct {
	Status              string                             `json:"status"`
	RequestID           string                             `json:"request_id"`
	Rooms               []RoomDevicesAvailableForGroupView `json:"rooms"`
	UnconfiguredDevices []DeviceAvailableForGroupView      `json:"unconfigured_devices"`
}

func (view *GroupAvailableDevicesView) From(ctx context.Context, devices model.Devices, group model.Group, availableDeviceType model.DeviceType, stereopairs model.Stereopairs) {
	devicesAlreadyInGroupMap := make(map[string]struct{})
	view.Rooms = make([]RoomDevicesAvailableForGroupView, 0)
	view.UnconfiguredDevices = make([]DeviceAvailableForGroupView, 0)
	for _, d := range group.Devices {
		devicesAlreadyInGroupMap[d] = struct{}{}
	}
	// fictional group needed for all logic about group compatibility concentration in one place
	fictionalGroup := model.Group{Type: availableDeviceType}
	roomsMap := make(map[string]RoomDevicesAvailableForGroupView)
	for _, d := range devices {
		if !group.CompatibleWith(d) || !fictionalGroup.CompatibleWith(d) {
			continue
		}
		var deviceView DeviceAvailableForGroupView
		switch stereopairs.GetDeviceRole(d.ID) {
		case model.LeaderRole:
			stereopair, _ := stereopairs.GetByDeviceID(d.ID)
			deviceView.FromStereopair(ctx, stereopair, group)
		case model.FollowerRole:
			// skip
			continue
		default:
			deviceView.From(d, group)
		}
		if d.Room == nil {
			view.UnconfiguredDevices = append(view.UnconfiguredDevices, deviceView)
			continue
		}
		roomView, exist := roomsMap[d.Room.ID]
		if !exist {
			roomView = RoomDevicesAvailableForGroupView{}
			roomView.FromRoom(*d.Room)
		}
		roomView.Devices = append(roomView.Devices, deviceView)
		roomsMap[d.Room.ID] = roomView
	}
	for _, roomView := range roomsMap {
		sort.Sort(DeviceAvailableForGroupViewSorting(roomView.Devices))
		view.Rooms = append(view.Rooms, roomView)
	}
	sort.Sort(DeviceAvailableForGroupViewSorting(view.UnconfiguredDevices))
	sort.Sort(RoomDevicesAvailableForGroupViewSorting(view.Rooms))
}

type RoomDevicesAvailableForGroupView struct {
	ID      string                        `json:"id"`
	Name    string                        `json:"name"`
	Devices []DeviceAvailableForGroupView `json:"devices"`
}

func (view *RoomDevicesAvailableForGroupView) FromRoom(room model.Room) {
	view.ID = room.ID
	view.Name = room.Name
}

type DeviceAvailableForGroupView struct {
	ID         string           `json:"id"`
	Name       string           `json:"name"`
	Type       model.DeviceType `json:"type"`
	ItemType   ItemInfoViewType `json:"item_type"`
	Stereopair *StereopairView  `json:"stereopair,omitempty"`
	IsSelected *bool            `json:"is_selected,omitempty"`
	SkillID    string           `json:"skill_id"`
	QuasarInfo *QuasarInfo      `json:"quasar_info,omitempty"`
	RenderInfo *RenderInfoView  `json:"render_info,omitempty"`
}

func (view *DeviceAvailableForGroupView) From(device model.Device, group model.Group) {
	view.ID = device.ID
	view.Name = device.Name
	view.Type = device.Type
	view.SkillID = device.SkillID
	view.ItemType = DeviceItemInfoViewType
	view.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	if slices.Contains(group.Devices, view.ID) {
		view.IsSelected = ptr.Bool(true)
	}
	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		view.QuasarInfo = &quasarInfo
	}
}
func (view *DeviceAvailableForGroupView) FromStereopair(ctx context.Context, stereopair model.Stereopair, group model.Group) {
	view.From(stereopair.GetLeaderDevice(), group)
	view.ItemType = StereopairItemInfoViewType
	view.Name = stereopair.Name
	view.Stereopair = &StereopairView{}
	view.Stereopair.From(ctx, stereopair)
}

type GroupStateRoomView struct {
	ID      string                 `json:"id"`
	Name    string                 `json:"name"`
	Devices []GroupStateDeviceView `json:"devices"`
}

func (v *GroupStateRoomView) FromRoom(room model.Room) {
	v.ID = room.ID
	v.Name = room.Name
}

type GroupStateDeviceView struct {
	ID           string              `json:"id"`
	Name         string              `json:"name"`
	Type         model.DeviceType    `json:"type"`
	ItemType     ItemInfoViewType    `json:"item_type"`
	Stereopair   *StereopairView     `json:"stereopair,omitempty"`
	IconURL      string              `json:"icon_url"`
	Capabilities []model.ICapability `json:"capabilities"`
	Properties   []PropertyStateView `json:"properties"`
	Groups       []string            `json:"groups"`
	SkillID      string              `json:"skill_id"`
	RenderInfo   *RenderInfoView     `json:"render_info,omitempty"`
	QuasarInfo   *QuasarInfo         `json:"quasar_info,omitempty"`
}

func (d *GroupStateDeviceView) FromDevice(device model.Device) {
	d.ID = device.ID
	d.Name = device.Name
	d.Type = device.Type
	d.ItemType = DeviceItemInfoViewType
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
	d.SkillID = device.SkillID
	d.Groups = make([]string, 0) // FIXME: ALICE-10531: front cannot show the component due to absence of this field

	d.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		d.QuasarInfo = &quasarInfo
	}
}

func (d *GroupStateDeviceView) FromStereopair(ctx context.Context, stereopair model.Stereopair) {
	d.FromDevice(stereopair.GetLeaderDevice())
	d.Name = stereopair.Name
	d.ItemType = StereopairItemInfoViewType
	d.Stereopair = &StereopairView{}
	d.Stereopair.From(ctx, stereopair)
}

type DeviceGroupEditView struct {
	ID         string           `json:"id"`
	Name       string           `json:"name"`
	Type       model.DeviceType `json:"type"`
	ItemType   ItemInfoViewType `json:"item_type"`
	Stereopair *StereopairView  `json:"stereopair,omitempty"`
	QuasarInfo *QuasarInfo      `json:"quasar_info,omitempty"`
	RenderInfo *RenderInfoView  `json:"render_info,omitempty"`
}

func (view *DeviceGroupEditView) FromDevice(device model.Device) {
	view.ID = device.ID
	view.Name = device.Name
	view.Type = device.Type
	view.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		view.QuasarInfo = &quasarInfo
	}
}

func (view *DeviceGroupEditView) FromStereopair(ctx context.Context, stereopair model.Stereopair) {
	view.FromDevice(stereopair.GetLeaderDevice())
	view.Name = stereopair.Name
	view.ItemType = StereopairItemInfoViewType
	view.Stereopair = &StereopairView{}
	view.Stereopair.From(ctx, stereopair)
}
