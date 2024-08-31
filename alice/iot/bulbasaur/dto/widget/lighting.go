package widget

import (
	"fmt"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type LightingItemValueView struct {
	ID          string `json:"id"`
	Name        string `json:"name"`
	DeviceCount int    `json:"device_count,omitempty"`
	IconURL     string `json:"icon_url,omitempty"`
}

type RoomName string

func (rn RoomName) IconURL(format model.IconFormat) string {
	url, exist := KnownRoomNamesURL[rn]
	if !exist {
		url = "https://avatars.mds.yandex.net/get-iot/icons-rooms-default-room.svg"
	}
	switch format {
	case model.OriginalIconFormat:
		return fmt.Sprintf("%s/orig", url)
	default:
		panic(fmt.Sprintf("Unexpected icon format %s for room with name %s", format, rn))
	}
}

type LightingItemView struct {
	Type  string                `json:"type"`
	Value LightingItemValueView `json:"value"`
}

func (li *LightingItemView) FromRoom(room model.Room) {
	li.Type = LightingItemViewTypeRoom
	li.Value = LightingItemValueView{
		ID:          room.ID,
		Name:        room.Name,
		DeviceCount: len(room.Devices),
		IconURL:     RoomName(room.Name).IconURL(model.OriginalIconFormat),
	}
}

func (li *LightingItemView) FromGroup(group model.Group) {
	li.Type = LightingItemViewTypeGroup
	li.Value = LightingItemValueView{
		ID:          group.ID,
		Name:        group.Name,
		DeviceCount: len(group.Devices),
	}
}

func (li *LightingItemView) FromDevice(device model.Device) {
	li.Type = LightingItemViewTypeDevice
	li.Value = LightingItemValueView{
		ID:      device.ID,
		Name:    device.Name,
		IconURL: device.Type.IconURL(model.OriginalIconFormat),
	}
}

type LightingHouseholdView struct {
	ID    string             `json:"id"`
	Name  string             `json:"name"`
	Items []LightingItemView `json:"items"`
}

type LightingUserView struct {
	Households []LightingHouseholdView `json:"households"`
	RequestID  string                  `json:"request_id"`
	Status     string                  `json:"status"`
}

func (userView *LightingUserView) FromDevices(devices model.Devices, households model.Households) {
	idToHousehold := households.ToMap()

	devices = devices.FilterByDeviceTypes([]model.DeviceType{model.LightDeviceType})
	for householdID, curDevices := range devices.GroupByHousehold() {
		rooms := curDevices.GetRooms()
		groups := curDevices.GetGroups()
		items := make([]LightingItemView, 0)
		for _, room := range rooms {
			if len(room.Devices) == 0 {
				continue
			}
			var lightingItemRoomView LightingItemView
			lightingItemRoomView.FromRoom(room)
			items = append(items, lightingItemRoomView)
		}
		for _, group := range groups {
			if len(group.Devices) == 0 {
				continue
			}
			var lightingItemGroupView LightingItemView
			lightingItemGroupView.FromGroup(group)
			items = append(items, lightingItemGroupView)
		}
		for _, device := range curDevices {
			var lightingItemDeviceView LightingItemView
			lightingItemDeviceView.FromDevice(device)
			items = append(items, lightingItemDeviceView)
		}
		sort.Sort(LightingItemViewSorting(items))
		userView.Households = append(userView.Households, LightingHouseholdView{
			ID:    householdID,
			Name:  idToHousehold[householdID].Name,
			Items: items,
		})
	}
	sort.Sort(LightingHouseholdViewSorting(userView.Households))
}
