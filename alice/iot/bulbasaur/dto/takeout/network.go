package takeout

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type NetworkView struct {
	SSID string
}

func (n *NetworkView) FromModel(network model.Network) {
	n.SSID = network.SSID
}
