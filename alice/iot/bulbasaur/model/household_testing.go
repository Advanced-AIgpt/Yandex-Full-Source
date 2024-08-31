package model

func NewHousehold(name string) *Household {
	return &Household{
		Name: name,
	}
}

func (h *Household) WithID(id string) *Household {
	h.ID = id
	return h
}

func (h *Household) WithLocation(location *HouseholdLocation) *Household {
	h.Location = location
	return h
}

func NewHouseholdLocation(address string) *HouseholdLocation {
	return &HouseholdLocation{
		Address:      address,
		ShortAddress: address,
	}
}

func (l *HouseholdLocation) WithCoordinates(latitude float64, longitude float64) *HouseholdLocation {
	l.Latitude = latitude
	l.Longitude = longitude
	return l
}

func (l *HouseholdLocation) WithAddress(address string) *HouseholdLocation {
	l.Address = address
	return l
}

func (l *HouseholdLocation) WithShortAddress(shortAddress string) *HouseholdLocation {
	l.ShortAddress = shortAddress
	return l
}
