package sup

import (
	"testing"

	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"github.com/stretchr/testify/assert"
)

func TestDiscoveryLinksAreNotForgotten(t *testing.T) {
	for _, dt := range model.VoiceDiscoveryDeviceTypes {
		_, exist := discoveryLinks[bmodel.DeviceType(dt)]
		assert.True(t, exist)
	}
}

func TestWifiIs5GhzPushTextsAreNotForgotten(t *testing.T) {
	for _, dt := range model.VoiceDiscoveryDeviceTypes {
		_, exist := wifiIs5GhzPushTexts[bmodel.DeviceType(dt)]
		assert.True(t, exist)
	}
}

func TestDiscoveryErrorPushTextsAreNotForgotten(t *testing.T) {
	for _, dt := range model.VoiceDiscoveryDeviceTypes {
		_, exist := discoveryErrorPushTexts[bmodel.DeviceType(dt)]
		assert.True(t, exist)
	}
}

func TestDiscoveryNotAllowedPushTextsAreNotForgotten(t *testing.T) {
	for _, dt := range model.VoiceDiscoveryDeviceTypes {
		_, exist := discoveryNotAllowedPushTexts[bmodel.DeviceType(dt)]
		assert.True(t, exist)
	}
}
