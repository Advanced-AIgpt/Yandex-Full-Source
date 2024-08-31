package datasync

import (
	"a.yandex-team.ru/alice/library/go/tools"
)

type AddressItem struct {
	Address      string  `json:"address_line"`
	ShortAddress string  `json:"address_line_short"`
	Latitude     float64 `json:"latitude"`
	Longitude    float64 `json:"longitude"`
}

func (item AddressItem) NormalizedAddress() string {
	return tools.Capitalize(tools.StandardizeSpaces(item.Address))
}

func (item AddressItem) NormalizedShortAddress() string {
	return tools.Capitalize(tools.StandardizeSpaces(item.ShortAddress))
}

type PersonalityAddressesResponse struct {
	Items []AddressItem `json:"items"`
}
