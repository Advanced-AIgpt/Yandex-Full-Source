package model

import (
	"a.yandex-team.ru/alice/library/go/datasync"
	"a.yandex-team.ru/alice/library/go/geosuggest"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/common"
)

type Household struct {
	ID          string             `json:"id"`
	Name        string             `json:"name"`
	Location    *HouseholdLocation `json:"location,omitempty"`
	SharingInfo *SharingInfo       `json:"sharing_info,omitempty"`
}

func (h Household) AssertName() error {
	return validName(h.Name, HouseholdNameLength)
}

func (h Household) ValidateName(households []Household) error {
	if err := h.AssertName(); err != nil {
		return err
	}
	for _, household := range households {
		if household.ID == h.ID {
			continue
		}
		if tools.IsAlphanumericEqual(household.Name, h.Name) {
			return &NameIsAlreadyTakenError{}
		}
	}
	return nil
}

func (h Household) ToUserInfoProto() *common.TIoTUserInfo_THousehold {
	res := &common.TIoTUserInfo_THousehold{
		Id:          h.ID,
		Name:        h.Name,
		SharingInfo: h.SharingInfo.ToUserInfoProto(),
	}
	if h.Location != nil {
		res.Longitude = h.Location.Longitude
		res.Latitude = h.Location.Latitude
		res.Address = h.Location.Address
	}
	return res
}

type Households []Household

func (hs Households) ToMap() map[string]Household {
	res := make(map[string]Household)
	for _, household := range hs {
		res[household.ID] = household
	}
	return res
}

func (hs Households) DeleteByID(householdID string) Households {
	res := make(Households, 0, len(hs))
	for _, household := range hs {
		if householdID == household.ID {
			continue
		}
		res = append(res, household)
	}
	return res
}

func (hs Households) GetByID(householdID string) (Household, bool) {
	for _, household := range hs {
		if household.ID == householdID {
			return household, true
		}
	}
	return Household{}, false
}

func (hs Households) FilterByIDs(householdIDs []string) Households {
	householdMap := make(map[string]bool, len(householdIDs))
	for _, householdID := range householdIDs {
		householdMap[householdID] = true
	}
	result := make(Households, 0, len(hs))
	for _, household := range hs {
		if householdMap[household.ID] {
			result = append(result, household)
		}
	}
	return result
}

func (hs Households) SetSharingInfoAndName(sharingInfos SharingInfos) {
	sharingInfoMap := sharingInfos.ToHouseholdMap()
	for i := range hs {
		sharingInfo, ok := sharingInfoMap[hs[i].ID]
		if !ok {
			continue
		}
		hs[i].SharingInfo = &sharingInfo
		hs[i].Name = sharingInfo.HouseholdName
	}
}

func GetDefaultHousehold() Household {
	return Household{
		Name: "Мой дом",
	}
}

type HouseholdLocation struct {
	Longitude    float64 `json:"longitude"`
	Latitude     float64 `json:"latitude"`
	Address      string  `json:"address"`
	ShortAddress string  `json:"short_address"`
}

func (l HouseholdLocation) Clone() HouseholdLocation {
	return HouseholdLocation{
		Longitude:    l.Longitude,
		Latitude:     l.Latitude,
		Address:      l.Address,
		ShortAddress: l.ShortAddress,
	}
}

func (l *HouseholdLocation) FromGeosuggest(gs geosuggest.Geosuggest) error {
	coords, err := gs.Coordinates()
	if err != nil {
		return err
	}
	l.Address = gs.Address()
	l.ShortAddress = gs.ShortAddress()
	l.Latitude = coords.Latitude
	l.Longitude = coords.Longitude
	return nil
}

func (l *HouseholdLocation) FromDatasyncAddress(datasyncAddress datasync.AddressItem) {
	l.Longitude = datasyncAddress.Longitude
	l.Latitude = datasyncAddress.Latitude
	l.Address = datasyncAddress.NormalizedAddress()
	l.ShortAddress = datasyncAddress.NormalizedShortAddress()
}
