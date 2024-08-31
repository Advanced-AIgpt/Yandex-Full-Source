package mobile

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestNetworkSaveRequest_ToNetwork(t *testing.T) {
	expected := model.Network{
		SSID:     "vifi",
		Password: "parol",
	}

	saveRequest := NetworkSaveRequest{
		SSID:     "vifi",
		Password: "parol",
	}

	assert.Equal(t, expected, saveRequest.ToNetwork())
}
