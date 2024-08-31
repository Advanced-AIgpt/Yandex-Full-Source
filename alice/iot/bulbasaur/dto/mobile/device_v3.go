package mobile

import (
	"context"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	"a.yandex-team.ru/alice/library/go/recorder"
	"a.yandex-team.ru/alice/library/go/sorting"
	"a.yandex-team.ru/alice/library/go/tools"
)

type DeviceListViewV3 struct {
	Status     string                       `json:"status"`
	RequestID  string                       `json:"request_id"`
	Households []HouseholdWithDevicesViewV3 `json:"households"`
	Favorites  FavoriteListView             `json:"favorites"`
	UpdatesURL string                       `json:"updates_url,omitempty"`
}

func (d *DeviceListViewV3) From(ctx context.Context, userinfo model.UserInfo, updatesURL string) {
	d.UpdatesURL = updatesURL
	d.Households = NewHouseholdWithDevicesViews(ctx, userinfo)
	d.Favorites.From(ctx, userinfo)
}

func NewHouseholdWithDevicesViews(ctx context.Context, userinfo model.UserInfo) []HouseholdWithDevicesViewV3 {
	devicesPerHouseholds := userinfo.Devices.GroupByHousehold()
	results := make([]HouseholdWithDevicesViewV3, 0, len(userinfo.Households))
	for _, household := range userinfo.Households {
		var view HouseholdWithDevicesViewV3
		view.FromHousehold(household, userinfo.CurrentHouseholdID)
		if devicesForHousehold, exists := devicesPerHouseholds[view.ID]; exists {
			view.From(ctx, devicesForHousehold, userinfo.Stereopairs)
		} else {
			view.From(ctx, []model.Device{}, userinfo.Stereopairs)
		}
		results = append(results, view)
	}
	sort.Sort(HouseholdWithDevicesInfoViewV3Sorting(results))
	return results
}

type HouseholdWithDevicesViewV3 struct {
	HouseholdInfoView
	DeviceListInfoV3
}

type DeviceListInfoV3 struct {
	Rooms              RoomInfoViewsV3     `json:"rooms"`
	All                []ItemInfoView      `json:"all"`
	AllBackgroundImage BackgroundImageView `json:"all_background_image"`
}

func (d *DeviceListInfoV3) From(ctx context.Context, devices model.Devices, stereopairs model.Stereopairs) {
	d.All = make([]ItemInfoView, 0)
	rooms := make(map[string]*RoomInfoViewV3)
	unconfiguredDevicesCount := 0

	for _, device := range devices {
		// TODO: IOT-271: temporary hack to prevent showing cars in searchapp
		if device.SkillID == model.REMOTECAR {
			continue
		}
		if stereopairs.GetDeviceRole(device.ID) == model.FollowerRole {
			continue
		}

		var itemInfoView ItemInfoView
		if stereopair, ok := stereopairs.GetByDeviceID(device.ID); ok {
			itemInfoView.FromStereopair(ctx, stereopair)
		} else {
			itemInfoView.FromDevice(ctx, device)
		}

		if err := device.AssertSetup(); err != nil {
			d.All = append(d.All, itemInfoView)
			unconfiguredDevicesCount++
			continue // skip Room Grouping cause unconfigured devices should not be displayed at Rooms section
		}
		d.All = append(d.All, itemInfoView)
		// Devices grouped by Rooms
		if rooms[device.Room.Name] == nil {
			var newRoom RoomInfoViewV3
			newRoom.FromRoom(*device.Room)
			rooms[device.Room.Name] = &newRoom
		}
		rooms[device.Room.Name].Items = append(rooms[device.Room.Name].Items, itemInfoView)
	}

	devicesGroups := devices.GetGroups()
	devicesPerGroup := devices.GroupByGroupID()
	for _, group := range devicesGroups {
		groupDevices, exist := devicesPerGroup[group.ID]
		if !exist {
			continue
		}
		var itemInfoView ItemInfoView
		itemInfoView.FromGroup(group, groupDevices)
		d.All = append(d.All, itemInfoView)
		for _, groupRoom := range groupDevices.GetRooms() {
			if rooms[groupRoom.Name] == nil {
				var newRoom RoomInfoViewV3
				newRoom.FromRoom(groupRoom)
				rooms[groupRoom.Name] = &newRoom
			}
			rooms[groupRoom.Name].Items = append(rooms[groupRoom.Name].Items, itemInfoView)
		}
	}

	d.Rooms = make([]RoomInfoViewV3, 0, len(rooms))
	for _, room := range rooms {
		sort.Sort(ItemInfoViewSorting(room.Items))
		d.Rooms = append(d.Rooms, *room)
	}
	// FIXME: IOT-1164: rollback it until better times
	// hideAllSection := unconfiguredDevicesCount == 0 && len(d.Rooms) == 1 && len(d.Rooms[0].Groups()) == 0
	// if hideAllSection {
	//	 d.All = make([]ItemInfoView, 0)
	// }
	sort.Sort(RoomInfoViewV3Sorting(d.Rooms))
	sort.Sort(ItemInfoViewSorting(d.All))

	d.Rooms.FillBackgroundImages()
	d.AllBackgroundImage = NewBackgroundImageView(model.AllBackgroundImageID)
}

type ItemInfoView struct {
	ID           string                 `json:"id"`
	Name         string                 `json:"name"`
	Type         model.DeviceType       `json:"type"`
	IconURL      string                 `json:"icon_url"`
	Capabilities []model.ICapability    `json:"capabilities"`
	Properties   []PropertyStateView    `json:"properties"`
	ItemType     ItemInfoViewType       `json:"item_type"`
	SkillID      string                 `json:"skill_id,omitempty"`
	QuasarInfo   *QuasarInfo            `json:"quasar_info,omitempty"`
	Stereopair   *StereopairView        `json:"stereopair,omitempty"`
	RoomName     string                 `json:"room_name,omitempty"`
	Unconfigured bool                   `json:"unconfigured,omitempty"`
	State        model.DeviceStatus     `json:"state,omitempty"`
	RenderInfo   *RenderInfoView        `json:"render_info,omitempty"`
	SharingInfo  *SharingInfoView       `json:"sharing_info,omitempty"`
	Created      string                 `json:"created,omitempty"`
	GroupsIDs    []string               `json:"groups_ids,omitempty"`
	DevicesIDs   []string               `json:"devices_ids,omitempty"`
	Parameters   ItemInfoViewParameters `json:"parameters,omitempty"`

	// back-compatibility fields: purge it after not-supporting device v2 handlers
	// for groups
	RoomNames    []string `json:"room_names,omitempty"`
	DevicesCount int      `json:"devices_count,omitempty"`
}

func (v ItemInfoView) GetID() string { return v.ID }

func (v ItemInfoView) GetName() string { return v.Name }

func (v ItemInfoView) isFavoriteItemParameters() {}

func (v *ItemInfoView) FromDevice(ctx context.Context, device model.Device) {
	v.ID = device.ID
	v.Name = device.Name
	v.Type = device.Type
	v.IconURL = device.Type.IconURL(model.OriginalIconFormat)
	v.SkillID = device.SkillID
	v.fromCapabilities(device.Capabilities)
	v.fromProperties(device.Properties)
	if device.Room != nil {
		v.RoomName = device.Room.Name
	}
	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		v.QuasarInfo = &quasarInfo
	}
	v.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	v.SharingInfo = NewSharingInfoView(device.SharingInfo)
	if err := device.AssertSetup(); err != nil {
		v.Unconfigured = true
	}
	for _, group := range device.Groups {
		v.GroupsIDs = append(v.GroupsIDs, group.ID)
	}
	v.Created = formatTimestamp(device.Created)
	v.ItemType = DeviceItemInfoViewType
	parameters := DeviceItemInfoViewParameters{}
	parameters.FromDevice(ctx, device)
	v.Parameters = parameters
}

func (v *ItemInfoView) FromStereopair(ctx context.Context, stereopair model.Stereopair) {
	viewDevice := stereopair.GetLeaderDevice().Clone()
	// must set name before FromDevice - for use stereopair name for AssertSetup
	viewDevice.Name = stereopair.Name

	v.FromDevice(ctx, viewDevice)
	v.ItemType = StereopairItemInfoViewType
	v.Stereopair = &StereopairView{}
	v.Stereopair.From(ctx, stereopair)
}

func (v *ItemInfoView) FromGroup(group model.Group, groupDevices model.Devices) {
	v.ID = group.ID
	v.Name = group.Name
	v.Type = group.Type
	v.IconURL = group.Type.IconURL(model.OriginalIconFormat)
	v.fromCapabilities(model.GroupCapabilitiesFromDevices(groupDevices).Flatten())
	v.Properties = make([]PropertyStateView, 0) // @lawyard: we do not show properties for groups for now
	v.ItemType = GroupItemInfoViewType
	v.State = model.GroupOnOffStateFromDevices(groupDevices)
	for _, device := range groupDevices {
		v.DevicesIDs = append(v.DevicesIDs, device.ID)
	}
	v.SharingInfo = NewSharingInfoView(group.SharingInfo)

	// back-compatibility fields
	v.DevicesCount = len(groupDevices)
	devicesRooms := groupDevices.GetRooms()
	v.RoomNames = make([]string, 0, len(devicesRooms))
	for _, deviceRoom := range devicesRooms {
		v.RoomNames = append(v.RoomNames, deviceRoom.Name)
	}
	sort.Sort(sorting.CaseInsensitiveStringsSorting(v.RoomNames))
}

func (v *ItemInfoView) fromCapabilities(capabilities model.Capabilities) {
	result := make([]model.ICapability, 0)
	// show only on_off capabilities on item info view
	if onOffCapability, exists := capabilities.GetCapabilityByTypeAndInstance(model.OnOffCapabilityType, string(model.OnOnOffCapabilityInstance)); exists {
		result = append(result, onOffCapability)
	}
	v.Capabilities = result
}

func (v *ItemInfoView) fromProperties(properties model.Properties) {
	result := make([]PropertyStateView, 0, len(properties))
	for _, property := range properties {
		var propertyStateView PropertyStateView
		propertyStateView.FromProperty(property)
		result = append(result, propertyStateView)
	}
	v.Properties = result
}

type ItemInfoViewParameters interface {
	Type() ItemInfoViewType
}

type DeviceItemInfoViewParameters struct {
	DeviceInfo *DeviceInfo     `json:"device_info,omitempty"`
	Voiceprint *VoiceprintView `json:"voiceprint,omitempty"`
}

func (DeviceItemInfoViewParameters) Type() ItemInfoViewType {
	return DeviceItemInfoViewType
}

func (p *DeviceItemInfoViewParameters) FromDevice(ctx context.Context, d model.Device) {
	if d.DeviceInfo != nil {
		p.DeviceInfo = &DeviceInfo{}
		p.DeviceInfo.FromDeviceInfo(ctx, d.DeviceInfo, d.CustomData)
	}
	// FIXME: we want to delete voiceprint view in v3 devices to not have dependency on memento
	// keep it here for backward compatibility for now
	p.Voiceprint = NewVoiceprintView(d, nil)
}

type ItemInfoViewType string

type RoomInfoViewV3 struct {
	ID              string              `json:"id"`
	Name            string              `json:"name"`
	SharingInfo     *SharingInfoView    `json:"sharing_info,omitempty"`
	Items           []ItemInfoView      `json:"items"`
	BackgroundImage BackgroundImageView `json:"background_image"`
}

func (v *RoomInfoViewV3) FromRoom(room model.Room) {
	v.ID = room.ID
	v.Name = room.Name
	v.SharingInfo = NewSharingInfoView(room.SharingInfo)
}

func (v *RoomInfoViewV3) FillBackgroundImage(defaultImage model.BackgroundImageID) {
	name := tools.Standardize(v.Name)
	var backgroundImage model.BackgroundImageID
	switch name {
	case "гостиная", "гостинная", "зал", "большая комната":
		backgroundImage = model.LivingRoomBackgroundImageID
	case "спальня":
		backgroundImage = model.BedroomBackgroundImageID
	case "кухня", "столовая":
		backgroundImage = model.KitchenBackgroundImageID
	case "детская", "детская комната":
		backgroundImage = model.ChildrenRoomBackgroundImageID
	case "прихожая", "гардеробная", "гардероб", "коридор":
		backgroundImage = model.HallwayBackgroundImageID
	case "ванная", "туалет", "ванная комната":
		backgroundImage = model.BathroomBackgroundImageID
	case "кабинет":
		backgroundImage = model.WorkspaceBackgroundImageID
	case "дача":
		backgroundImage = model.CountryHouseBackgroundImageID
	case "моя комната", "моя", "комната":
		backgroundImage = model.MyRoomBackgroundImageID
	case "балкон":
		backgroundImage = model.BalconyBackgroundImageID
	default:
		backgroundImage = defaultImage
	}
	v.BackgroundImage = NewBackgroundImageView(backgroundImage)
}

func (v *RoomInfoViewV3) Groups() []ItemInfoView {
	result := make([]ItemInfoView, 0, len(v.Items))
	for _, item := range v.Items {
		if item.ItemType == GroupItemInfoViewType {
			result = append(result, item)
		}
	}
	return result
}

type RoomInfoViewsV3 []RoomInfoViewV3

func (views RoomInfoViewsV3) FillBackgroundImages() {
	for i := range views {
		defaultImage := defaultBackgroundImageIDs[i%len(defaultBackgroundImageIDs)]
		views[i].FillBackgroundImage(defaultImage)
	}
}

type DeviceDiscoveryResultViewV3 struct {
	Status    string              `json:"status"`
	RequestID string              `json:"request_id"`
	DebugInfo *recorder.DebugInfo `json:"debug,omitempty"`

	NewDeviceCount     int                     `json:"new_device_count"`
	UpdatedDeviceCount int                     `json:"updated_device_count"`
	LimitDeviceCount   int                     `json:"limit_device_count"`
	ErrorDeviceCount   int                     `json:"error_device_count"`
	NewDevices         []DeviceDiscoveryViewV3 `json:"new_devices"`
}

func (v *DeviceDiscoveryResultViewV3) From(storeResults model.DeviceStoreResults, households model.Households, rooms model.Rooms) {
	householdsMap := households.ToMap()
	roomsMap := rooms.ToMap()
	for _, storeResult := range storeResults {
		switch storeResult.Result {
		case model.StoreResultNew:
			v.NewDeviceCount++
			deviceHousehold, exist := householdsMap[storeResult.HouseholdID]
			if !exist {
				continue
			}
			var deviceRoom *model.Room
			if room, exist := roomsMap[storeResult.RoomID()]; exist {
				deviceRoom = &room
			}
			var newDevice DeviceDiscoveryViewV3
			newDevice.From(storeResult.Device, deviceHousehold, deviceRoom)
			v.NewDevices = append(v.NewDevices, newDevice)
		case model.StoreResultUpdated:
			v.UpdatedDeviceCount++
		case model.StoreResultLimitReached:
			v.LimitDeviceCount++
		default:
			v.ErrorDeviceCount++
		}
	}
}

type DeviceDiscoveryViewV3 struct {
	ID                      string             `json:"id"`
	Name                    string             `json:"name"`
	NameValidationErrorCode model.ErrorCode    `json:"name_validation_error_code,omitempty"`
	Type                    model.DeviceType   `json:"type"`
	Household               HouseholdInfoView  `json:"household"`
	Room                    *RoomDiscoveryView `json:"room,omitempty"`
	RenderInfo              *RenderInfoView    `json:"render_info,omitempty"`
	NameSuggests            []string           `json:"name_suggests"`
}

func (v *DeviceDiscoveryViewV3) From(device model.Device, household model.Household, room *model.Room) {
	v.ID = device.ID
	v.Name = device.Name
	if err := device.AssertName(); err != nil {
		v.NameValidationErrorCode = nameValidationErrorToErrorCode(err)
	}
	v.Type = device.Type
	v.Household.FromHousehold(household, "")
	if room != nil {
		v.Room = &RoomDiscoveryView{}
		v.Room.FromRoom(*room)
	}
	v.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	suggests := suggestions.DefaultDeviceNames
	if v, ok := suggestions.DeviceNames[device.Type]; ok {
		suggests = v
	}
	v.NameSuggests = suggests
}
