package takeout

import "a.yandex-team.ru/alice/iot/bulbasaur/model"

type DeviceView struct {
	ID          string   `json:"id"`
	Name        string   `json:"name"`
	Type        string   `json:"type"`
	HouseholdID string   `json:"household_id"`
	Room        string   `json:"room_id,omitempty"`
	Groups      []string `json:"group_ids"`
}

func (dv *DeviceView) FromModel(device model.Device) {
	dv.ID = device.ID

	dv.Name = device.Name

	dv.Type = string(device.Type)

	if device.Room != nil {
		dv.Room = device.Room.ID
	}

	dv.HouseholdID = device.HouseholdID

	dv.Groups = make([]string, len(device.Groups))
	for i, group := range device.Groups {
		dv.Groups[i] = group.ID
	}
}
