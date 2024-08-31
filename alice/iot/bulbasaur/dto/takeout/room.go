package takeout

import "a.yandex-team.ru/alice/iot/bulbasaur/model"

type RoomView struct {
	ID          string `json:"id"`
	Name        string `json:"name"`
	HouseholdID string `json:"household_id"`
}

func (rv *RoomView) FromModel(group model.Room) {
	rv.ID = group.ID
	rv.Name = group.Name
	rv.HouseholdID = group.HouseholdID
}
