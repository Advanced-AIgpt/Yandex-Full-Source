package mobile

import (
	"context"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type FavoritePropertiesAvailableResponse struct {
	Status     string                                     `json:"status"`
	RequestID  string                                     `json:"request_id"`
	Households []HouseholdFavoritePropertiesAvailableView `json:"households"`
}

func (resp *FavoritePropertiesAvailableResponse) From(currentHouseholdID string, userDevices model.Devices, userHouseholds []model.Household, favorites model.Favorites) {
	householdDevicesMap := userDevices.GroupByHousehold()
	resp.Households = make([]HouseholdFavoritePropertiesAvailableView, 0, len(userHouseholds))
	for _, household := range userHouseholds {
		var view HouseholdFavoritePropertiesAvailableView
		if householdDevices, exist := householdDevicesMap[household.ID]; exist {
			view.From(currentHouseholdID, household, householdDevices, favorites)
		} else {
			view.From(currentHouseholdID, household, model.Devices{}, favorites)
		}
		resp.Households = append(resp.Households, view)
	}
	sort.Sort(HouseholdFavoritePropertiesAvailableViewSorting(resp.Households))
}

type HouseholdFavoritePropertiesAvailableView struct {
	HouseholdFavoriteView
	Rooms       []RoomFavoritePropertiesAvailableView `json:"rooms"`
	WithoutRoom []FavoritePropertyAvailableView       `json:"without_room"`
}

func (view *HouseholdFavoritePropertiesAvailableView) From(currentHouseholdID string, household model.Household, householdDevices model.Devices, favorites model.Favorites) {
	view.FromHousehold(currentHouseholdID, household)
	view.WithoutRoom = make([]FavoritePropertyAvailableView, 0)
	roomsMap := make(map[string]RoomFavoritePropertiesAvailableView)
	for _, d := range householdDevices {
		for _, p := range d.Properties {
			var propertyView FavoritePropertyAvailableView
			propertyView.FromFavoriteProperty(p, d, favorites.Properties.Contains(p.Key(), d.ID))
			if d.Room == nil {
				view.WithoutRoom = append(view.WithoutRoom, propertyView)
				continue
			}
			roomView, exist := roomsMap[d.Room.ID]
			if !exist {
				roomView = RoomFavoritePropertiesAvailableView{}
				roomView.FromRoom(*d.Room)
			}
			roomView.Properties = append(roomView.Properties, propertyView)
			roomsMap[roomView.ID] = roomView
		}
	}
	view.Rooms = make([]RoomFavoritePropertiesAvailableView, 0, len(roomsMap))
	for _, room := range roomsMap {
		sort.Sort(FavoritePropertyAvailableViewSorting(room.Properties))
		view.Rooms = append(view.Rooms, room)
	}
	sort.Sort(RoomFavoritePropertiesAvailableViewSorting(view.Rooms))
	sort.Sort(FavoritePropertyAvailableViewSorting(view.WithoutRoom))
}

type RoomFavoritePropertiesAvailableView struct {
	ID         string                          `json:"id"`
	Name       string                          `json:"name"`
	Properties []FavoritePropertyAvailableView `json:"properties"`
}

func (v *RoomFavoritePropertiesAvailableView) FromRoom(room model.Room) {
	v.ID = room.ID
	v.Name = room.Name
}

type FavoritePropertyAvailableView struct {
	Property   PropertyStateView `json:"property"`
	DeviceID   string            `json:"device_id"`
	DeviceName string            `json:"device_name"`
	IsSelected bool              `json:"is_selected,omitempty"`
}

func (v *FavoritePropertyAvailableView) FromFavoriteProperty(property model.IProperty, device model.Device, favorite bool) {
	v.Property.FromProperty(property)
	v.DeviceID = device.ID
	v.DeviceName = device.Name
	v.IsSelected = favorite
}

type FavoriteDevicesAvailableResponse struct {
	Status     string                                  `json:"status"`
	RequestID  string                                  `json:"request_id"`
	Households []HouseholdFavoriteDevicesAvailableView `json:"households"`
}

func (resp *FavoriteDevicesAvailableResponse) From(currentHouseholdID string, userDevices model.Devices, userHouseholds []model.Household) {
	householdDevicesMap := userDevices.GroupByHousehold()
	resp.Households = make([]HouseholdFavoriteDevicesAvailableView, 0, len(userHouseholds))
	for _, household := range userHouseholds {
		var view HouseholdFavoriteDevicesAvailableView
		if householdDevices, exist := householdDevicesMap[household.ID]; exist {
			view.From(currentHouseholdID, household, householdDevices)
		} else {
			view.From(currentHouseholdID, household, model.Devices{})
		}
		resp.Households = append(resp.Households, view)
	}
	sort.Sort(HouseholdFavoriteDevicesAvailableViewSorting(resp.Households))
}

type HouseholdFavoriteDevicesAvailableView struct {
	HouseholdFavoriteView
	Rooms               []RoomFavoriteDevicesAvailableView `json:"rooms"`
	UnconfiguredDevices []FavoriteDeviceAvailableView      `json:"unconfigured_devices"`
}

func (view *HouseholdFavoriteDevicesAvailableView) From(currentHouseholdID string, household model.Household, householdDevices model.Devices) {
	view.FromHousehold(currentHouseholdID, household)
	view.UnconfiguredDevices = make([]FavoriteDeviceAvailableView, 0, len(householdDevices))
	roomsMap := make(map[string]RoomFavoriteDevicesAvailableView)
	for _, d := range householdDevices {
		var deviceView FavoriteDeviceAvailableView
		deviceView.FromDevice(d)
		if d.Room == nil {
			view.UnconfiguredDevices = append(view.UnconfiguredDevices, deviceView)
			continue
		}
		roomView, exist := roomsMap[d.Room.ID]
		if !exist {
			roomView = RoomFavoriteDevicesAvailableView{}
			roomView.FromRoom(*d.Room)
		}
		roomView.Devices = append(roomView.Devices, deviceView)
		roomsMap[roomView.ID] = roomView
	}
	view.Rooms = make([]RoomFavoriteDevicesAvailableView, 0, len(roomsMap))
	for _, room := range roomsMap {
		sort.Sort(FavoriteDeviceAvailableViewSorting(room.Devices))
		view.Rooms = append(view.Rooms, room)
	}
	sort.Sort(RoomFavoriteDevicesAvailableViewSorting(view.Rooms))
	sort.Sort(FavoriteDeviceAvailableViewSorting(view.UnconfiguredDevices))
}

type RoomFavoriteDevicesAvailableView struct {
	ID      string                        `json:"id"`
	Name    string                        `json:"name"`
	Devices []FavoriteDeviceAvailableView `json:"devices"`
}

func (v *RoomFavoriteDevicesAvailableView) FromRoom(room model.Room) {
	v.ID = room.ID
	v.Name = room.Name
}

type FavoriteDeviceAvailableView struct {
	ID         string           `json:"id"`
	Name       string           `json:"name"`
	Type       model.DeviceType `json:"type"`
	IsSelected bool             `json:"is_selected,omitempty"`
	SkillID    string           `json:"skill_id"`
	QuasarInfo *QuasarInfo      `json:"quasar_info,omitempty"`
	RenderInfo *RenderInfoView  `json:"render_info,omitempty"`
}

func (view *FavoriteDeviceAvailableView) FromDevice(device model.Device) {
	view.ID = device.ID
	view.Name = device.Name
	view.Type = device.Type
	view.SkillID = device.SkillID
	view.IsSelected = device.Favorite
	view.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		view.QuasarInfo = &quasarInfo
	}
}

type FavoriteScenariosAvailableResponse struct {
	Status    string                          `json:"status"`
	RequestID string                          `json:"request_id"`
	Scenarios []FavoriteScenarioAvailableView `json:"scenarios"`
}

func (r *FavoriteScenariosAvailableResponse) From(scenarios model.Scenarios) {
	r.Scenarios = make([]FavoriteScenarioAvailableView, 0, len(scenarios))
	for _, s := range scenarios {
		var view FavoriteScenarioAvailableView
		view.FromScenario(s)
		r.Scenarios = append(r.Scenarios, view)
	}
	sort.Sort(FavoriteScenarioAvailableViewSorting(r.Scenarios))
}

type FavoriteScenarioAvailableView struct {
	ID         string             `json:"id"`
	Name       string             `json:"name"`
	Icon       model.ScenarioIcon `json:"icon"`
	IconURL    string             `json:"icon_url"`
	IsSelected bool               `json:"is_selected,omitempty"`
}

func (v *FavoriteScenarioAvailableView) FromScenario(scenario model.Scenario) {
	v.ID = scenario.ID
	v.Name = string(scenario.Name)
	v.Icon = scenario.Icon
	v.IconURL = v.Icon.URL()
	v.IsSelected = scenario.Favorite
}

type ScenarioMakeFavoriteRequest struct {
	Favorite bool `json:"favorite"`
}

type DeviceMakeFavoriteRequest struct {
	Favorite bool `json:"favorite"`
}

type GroupMakeFavoriteRequest struct {
	Favorite bool `json:"favorite"`
}

type PropertyMakeFavoriteRequest struct {
	Type     model.PropertyType `json:"type"`
	Instance string             `json:"instance"`
	Favorite bool               `json:"favorite"`
}

type ScenarioUpdateFavoritesRequest struct {
	ScenarioIDs []string `json:"scenario_ids"`
}

func (r ScenarioUpdateFavoritesRequest) ValidateByScenarios(userScenarios model.Scenarios) error {
	userScenariosMap := userScenarios.ToMap()
	for _, scenarioID := range r.ScenarioIDs {
		_, exist := userScenariosMap[scenarioID]
		if !exist {
			return &model.ScenarioNotFoundError{}
		}
	}
	return nil
}

func (r ScenarioUpdateFavoritesRequest) ToScenarios(userScenarios model.Scenarios) model.Scenarios {
	userScenariosMap := userScenarios.ToMap()
	result := make(model.Scenarios, 0, len(r.ScenarioIDs))
	for _, scenarioID := range r.ScenarioIDs {
		if scenario, exist := userScenariosMap[scenarioID]; exist {
			result = append(result, scenario)
		}
	}
	return result
}

type DeviceUpdateFavoritesRequest struct {
	DeviceIDs []string `json:"device_ids"`
}

func (r DeviceUpdateFavoritesRequest) ValidateByDevices(userDevices model.Devices) error {
	userDevicesMap := userDevices.ToMap()
	for _, deviceID := range r.DeviceIDs {
		_, exist := userDevicesMap[deviceID]
		if !exist {
			return &model.DeviceNotFoundError{}
		}
	}
	return nil
}

func (r DeviceUpdateFavoritesRequest) ToDevices(userDevices model.Devices) model.Devices {
	userDevicesMap := userDevices.ToMap()
	result := make(model.Devices, 0, len(r.DeviceIDs))
	for _, deviceID := range r.DeviceIDs {
		if device, exist := userDevicesMap[deviceID]; exist {
			result = append(result, device)
		}
	}
	return result
}

type GroupUpdateFavoritesRequest struct {
	GroupIDs []string `json:"group_ids"`
}

func (r GroupUpdateFavoritesRequest) ValidateByGroups(userGroups model.Groups) error {
	userGroupsMap := userGroups.ToMap()
	for _, groupID := range r.GroupIDs {
		_, exist := userGroupsMap[groupID]
		if !exist {
			return &model.GroupNotFoundError{}
		}
	}
	return nil
}

func (r GroupUpdateFavoritesRequest) ToGroups(userGroups model.Groups) model.Groups {
	userGroupsMap := userGroups.ToMap()
	result := make(model.Groups, 0, len(r.GroupIDs))
	for _, groupID := range r.GroupIDs {
		if group, exist := userGroupsMap[groupID]; exist {
			result = append(result, group)
		}
	}
	return result
}

type DevicePropertiesUpdateFavoritesRequest struct {
	Properties []DevicePropertyUpdateFavoriteView `json:"properties"`
}

func (r DevicePropertiesUpdateFavoritesRequest) ValidateByDevices(userDevices model.Devices) error {
	userDevicesMap := userDevices.ToMap()
	for _, property := range r.Properties {
		device, exist := userDevicesMap[property.DeviceID]
		if !exist {
			return &model.DeviceNotFoundError{}
		}
		devicePropertiesMap := device.Properties.AsMap()
		_, exist = devicePropertiesMap[model.PropertyKey(property.Type, property.Instance)]
		if !exist {
			return &model.UnknownPropertyInstanceError{}
		}
	}
	return nil
}

func (r DevicePropertiesUpdateFavoritesRequest) ToDeviceProperties(userDevices model.Devices) model.FavoritesDeviceProperties {
	userDevicesMap := userDevices.ToMap()
	result := make(model.FavoritesDeviceProperties, 0, len(r.Properties))
	for _, property := range r.Properties {
		device, exist := userDevicesMap[property.DeviceID]
		if !exist {
			continue
		}
		devicePropertiesMap := device.Properties.AsMap()
		modelProperty, exist := devicePropertiesMap[model.PropertyKey(property.Type, property.Instance)]
		if !exist {
			continue
		}
		result = append(result, property.ToFavoritesDeviceProperty(device.ID, modelProperty))
	}
	return result
}

type DevicePropertyUpdateFavoriteView struct {
	DeviceID string             `json:"device_id"`
	Type     model.PropertyType `json:"type"`
	Instance string             `json:"instance"`
}

func (v DevicePropertyUpdateFavoriteView) ToFavoritesDeviceProperty(deviceID string, property model.IProperty) model.FavoritesDeviceProperty {
	return model.FavoritesDeviceProperty{
		DeviceID: deviceID,
		Property: property,
	}
}

type FavoriteListView struct {
	Properties      []FavoriteDevicePropertyListView `json:"properties"`
	Items           []FavoriteListItemView           `json:"items"`
	BackgroundImage BackgroundImageView              `json:"background_image"`
}

func (v *FavoriteListView) From(ctx context.Context, userInfo model.UserInfo) {
	householdsMap := userInfo.Households.ToMap()
	devicesMap := userInfo.Devices.ToMap()
	favorites := userInfo.Favorites()

	// fill favorite properties
	v.Properties = make([]FavoriteDevicePropertyListView, 0, len(favorites.Properties))
	for _, favoriteDeviceProperty := range favorites.Properties {
		device, exist := devicesMap[favoriteDeviceProperty.DeviceID]
		if !exist {
			continue
		}
		household, exist := householdsMap[device.HouseholdID]
		if !exist {
			continue
		}
		var propertyView FavoriteDevicePropertyListView
		propertyView.From(favoriteDeviceProperty, device, household)
		v.Properties = append(v.Properties, propertyView)
	}

	sort.Sort(FavoriteDevicePropertyListViewSorting(v.Properties))

	// fill other favorite items
	v.Items = make([]FavoriteListItemView, 0, len(favorites.Devices)+len(favorites.Groups)+len(favorites.Scenarios))

	// fill devices
	for _, device := range favorites.Devices {
		var favoriteItemView FavoriteListItemView
		switch userInfo.Stereopairs.GetDeviceRole(device.ID) {
		case model.LeaderRole:
			stereopair, _ := userInfo.Stereopairs.GetByDeviceID(device.ID)
			favoriteItemView.FromStereopair(ctx, stereopair)
		case model.FollowerRole:
			// skip device
			continue
		default:
			favoriteItemView.FromDevice(ctx, device)
		}

		v.Items = append(v.Items, favoriteItemView)
	}

	// fill groups
	devicesPerGroupMap := userInfo.Devices.GroupByGroupID()
	for _, group := range favorites.Groups {
		var favoriteItemView FavoriteListItemView
		groupDevices, exist := devicesPerGroupMap[group.ID]
		if exist {
			favoriteItemView.FromGroup(group, groupDevices)
		} else {
			favoriteItemView.FromGroup(group, model.Devices{})
		}
		v.Items = append(v.Items, favoriteItemView)
	}

	// fill scenarios
	for _, scenario := range favorites.Scenarios {
		var favoriteItemView FavoriteListItemView
		favoriteItemView.FromScenario(scenario, userInfo.Devices)
		v.Items = append(v.Items, favoriteItemView)
	}

	v.BackgroundImage = NewBackgroundImageView(model.FavoriteBackgroundImageID)

	sort.Sort(FavoriteListItemViewSorting(v.Items))
}

type FavoriteDevicePropertyListView struct {
	DeviceID      string            `json:"device_id"`
	Property      PropertyStateView `json:"property"`
	RoomName      string            `json:"room_name,omitempty"`
	HouseholdName string            `json:"household_name,omitempty"`
}

func (v *FavoriteDevicePropertyListView) From(favoriteProperty model.FavoritesDeviceProperty, device model.Device, household model.Household) {
	v.DeviceID = device.ID
	v.Property.FromProperty(favoriteProperty.Property)
	if device.Room != nil {
		v.RoomName = device.Room.Name
	}
	v.HouseholdName = household.Name
}

type FavoriteListItemView struct {
	Type       model.FavoriteType      `json:"type"`
	Parameters IFavoriteItemParameters `json:"parameters"`
}

func (v *FavoriteListItemView) FromDevice(ctx context.Context, device model.Device) {
	v.Type = model.DeviceFavoriteType
	var itemView ItemInfoView
	itemView.FromDevice(ctx, device)
	v.Parameters = itemView
}

func (v *FavoriteListItemView) FromStereopair(ctx context.Context, stereopair model.Stereopair) {
	v.Type = model.StereopairFavoriteType
	var itemView ItemInfoView
	itemView.FromStereopair(ctx, stereopair)
	v.Parameters = itemView
}

func (v *FavoriteListItemView) FromGroup(group model.Group, groupDevices model.Devices) {
	v.Type = model.GroupFavoriteType
	var itemView ItemInfoView
	itemView.FromGroup(group, groupDevices)
	v.Parameters = itemView
}

func (v *FavoriteListItemView) FromScenario(scenario model.Scenario, userDevices model.Devices) {
	v.Type = model.ScenarioFavoriteType
	var scenarioView ScenarioListView
	scenarioView.From(scenario, userDevices)
	v.Parameters = scenarioView
}

type FavoriteGroupsAvailableResponse struct {
	Status     string                                 `json:"status"`
	RequestID  string                                 `json:"request_id"`
	Households []HouseholdFavoriteGroupsAvailableView `json:"households"`
}

func (r *FavoriteGroupsAvailableResponse) From(currentHouseholdID string, userHouseholds model.Households, userGroups model.Groups) {
	householdGroupsMap := userGroups.GroupByHousehold()
	r.Households = make([]HouseholdFavoriteGroupsAvailableView, 0, len(userHouseholds))
	for _, household := range userHouseholds {
		var view HouseholdFavoriteGroupsAvailableView
		if householdGroups, exist := householdGroupsMap[household.ID]; exist {
			view.From(currentHouseholdID, household, householdGroups)
		} else {
			view.From(currentHouseholdID, household, householdGroups)
		}
		r.Households = append(r.Households, view)
	}
	sort.Sort(HouseholdFavoriteGroupsAvailableViewSorting(r.Households))
}

type HouseholdFavoriteGroupsAvailableView struct {
	HouseholdFavoriteView
	Groups []FavoriteGroupAvailableView `json:"groups"`
}

func (v *HouseholdFavoriteGroupsAvailableView) From(currentHouseholdID string, household model.Household, householdGroups model.Groups) {
	v.FromHousehold(currentHouseholdID, household)
	v.Groups = make([]FavoriteGroupAvailableView, 0, len(householdGroups))
	for _, group := range householdGroups {
		if len(group.Devices) == 0 {
			continue
		}
		var view FavoriteGroupAvailableView
		view.FromGroup(group)
		v.Groups = append(v.Groups, view)
	}
	sort.Sort(FavoriteGroupAvailableViewSorting(v.Groups))
}

type FavoriteGroupAvailableView struct {
	ID           string           `json:"id"`
	Name         string           `json:"name"`
	Type         model.DeviceType `json:"type"`
	IsSelected   bool             `json:"is_selected,omitempty"`
	DevicesCount int              `json:"devices_count"`
}

func (v *FavoriteGroupAvailableView) FromGroup(group model.Group) {
	v.ID = group.ID
	v.Name = group.Name
	v.Type = group.Type
	v.IsSelected = group.Favorite
	v.DevicesCount = len(group.Devices)
}

type IFavoriteItemParameters interface {
	GetID() string
	GetName() string
	isFavoriteItemParameters()
}
