package takeout

import "a.yandex-team.ru/alice/iot/bulbasaur/model"

type GroupView struct {
	ID          string `json:"id"`
	Name        string `json:"name"`
	HouseholdID string `json:"household_id"`
}

func (gv *GroupView) FromModel(group model.Group) {
	gv.ID = group.ID
	gv.Name = group.Name
	gv.HouseholdID = group.HouseholdID
}
