package model

import (
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func NewNetwork(ssid string) *Network {
	return &Network{
		SSID: ssid,
	}
}

func (n *Network) WithPassword(password string) *Network {
	n.Password = password
	return n
}

func (n *Network) WithUpdated(ts timestamp.PastTimestamp) *Network {
	n.Updated = ts
	return n
}
