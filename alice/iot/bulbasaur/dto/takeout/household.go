package takeout

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type HouseholdView struct {
	ID       string                 `json:"id"`
	Name     string                 `json:"name"`
	Location *HouseholdLocationView `json:"location,omitempty"`
}

func (v *HouseholdView) FromModel(household model.Household) {
	v.ID = household.ID
	v.Name = household.Name
	if household.Location != nil {
		v.Location = &HouseholdLocationView{}
		v.Location.FromHouseholdLocation(*household.Location)
	}
}

type HouseholdLocationView struct {
	Address      string `json:"address"`
	ShortAddress string `json:"short_address"`
}

func (v *HouseholdLocationView) FromHouseholdLocation(location model.HouseholdLocation) {
	v.Address = location.Address
	v.ShortAddress = location.ShortAddress
}
