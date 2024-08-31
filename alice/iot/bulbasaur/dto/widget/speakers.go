package widget

import (
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type CallableSpeakersAvailableResponse struct {
	Status    string                `json:"status"`
	RequestID string                `json:"request_id"`
	Speakers  []CallableSpeakerView `json:"speakers"`
}

func (r *CallableSpeakersAvailableResponse) From(devices model.Devices, households model.Households) {
	householdsMap := households.ToMap()
	r.Speakers = make([]CallableSpeakerView, 0, len(devices))
	for _, device := range devices {
		if !model.CallableSpeakers[device.Type] {
			continue
		}
		household, exist := householdsMap[device.HouseholdID]
		if !exist {
			continue
		}
		var view CallableSpeakerView
		view.FromDevice(device, household)
		r.Speakers = append(r.Speakers, view)
	}
	sort.Sort(CallableSpeakersViewSorting(r.Speakers))
}

type CallableSpeakerView struct {
	ID         string                        `json:"id"`
	Name       string                        `json:"name"`
	Type       model.DeviceType              `json:"type"`
	IconURL    string                        `json:"icon_url"`
	QuasarInfo *mobile.QuasarInfo            `json:"quasar_info,omitempty"`
	Room       *CallableSpeakersRoomView     `json:"room,omitempty"`
	Household  CallableSpeakersHouseholdView `json:"household"`
}

func (v *CallableSpeakerView) FromDevice(device model.Device, household model.Household) {
	v.ID = device.ID
	v.Name = device.Name
	v.Type = device.Type
	v.IconURL = device.Type.IconURL(model.OriginalIconFormat)
	if device.IsQuasarDevice() {
		var quasarInfo mobile.QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		v.QuasarInfo = &quasarInfo
	}
	if device.Room != nil {
		var roomView CallableSpeakersRoomView
		roomView.FromRoom(*device.Room)
		v.Room = &roomView
	}
	v.Household.FromHousehold(household)
}

type CallableSpeakersRoomView struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}

func (v *CallableSpeakersRoomView) FromRoom(room model.Room) {
	v.ID = room.ID
	v.Name = room.Name
}

type CallableSpeakersHouseholdView struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}

func (v *CallableSpeakersHouseholdView) FromHousehold(household model.Household) {
	v.ID = household.ID
	v.Name = household.Name
}
