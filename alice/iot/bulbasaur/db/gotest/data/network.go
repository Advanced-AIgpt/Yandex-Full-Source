package data

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
)

func GenerateNetwork() model.Network {
	network := model.Network{
		SSID:     testing.RandLatinString(20),
		Password: testing.RandLatinString(20),
	}
	return network
}
