package takeout

import "a.yandex-team.ru/alice/iot/bulbasaur/model"

type DataView struct {
	Devices    map[string]DeviceView    `json:"devices"`
	Groups     map[string]GroupView     `json:"groups"`
	Rooms      map[string]RoomView      `json:"rooms"`
	Scenarios  map[string]ScenarioView  `json:"scenarios"`
	Households map[string]HouseholdView `json:"households"`
	Networks   []NetworkView            `json:"networks"`
}

func (data *DataView) IsEmpty() bool {
	return len(data.Devices) == 0 &&
		len(data.Groups) == 0 &&
		len(data.Rooms) == 0 &&
		len(data.Scenarios) == 0 &&
		len(data.Networks) == 0 &&
		len(data.Households) == 0
}

func (data *DataView) PopulateDevices(devices []model.Device) {
	data.Devices = make(map[string]DeviceView)
	for _, device := range devices {
		dv := DeviceView{}
		dv.FromModel(device)
		data.Devices[device.ID] = dv
	}
}

func (data *DataView) PopulateGroups(groups []model.Group) {
	data.Groups = make(map[string]GroupView)
	for _, group := range groups {
		gv := GroupView{}
		gv.FromModel(group)
		data.Groups[group.ID] = gv
	}
}

func (data *DataView) PopulateRooms(rooms []model.Room) {
	data.Rooms = make(map[string]RoomView)
	for _, room := range rooms {
		rv := RoomView{}
		rv.FromModel(room)
		data.Rooms[room.ID] = rv
	}
}

func (data *DataView) PopulateScenarios(scenarios []model.Scenario) {
	data.Scenarios = make(map[string]ScenarioView)
	for _, scenario := range scenarios {
		sv := ScenarioView{}
		sv.FromModel(scenario)
		data.Scenarios[scenario.ID] = sv
	}
}

func (data *DataView) PopulateHouseholds(households []model.Household) {
	data.Households = make(map[string]HouseholdView)
	for _, household := range households {
		hv := HouseholdView{}
		hv.FromModel(household)
		data.Households[household.ID] = hv
	}
}

func (data *DataView) PopulateNetworks(networks []model.Network) {
	data.Networks = make([]NetworkView, 0, len(networks))
	for _, network := range networks {
		var nv NetworkView
		nv.FromModel(network)
		data.Networks = append(data.Networks, nv)
	}
}
