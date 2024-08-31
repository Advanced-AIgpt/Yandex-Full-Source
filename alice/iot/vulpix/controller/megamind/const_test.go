package megamind

import (
	"testing"

	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"github.com/stretchr/testify/assert"
)

func TestScenarioConnectFrameNLG(t *testing.T) {
	for _, dt := range model.VoiceDiscoveryDeviceTypes {
		_, exist := nlgScenarioConnectFrameRunForDeviceType[bmodel.DeviceType(dt)]
		assert.True(t, exist)
	}
}

func TestScenarioBroadcastSuccessYandex(t *testing.T) {
	for _, dt := range model.VoiceDiscoveryDeviceTypes {
		_, exist := nlgScenarioBroadcastSuccessYandexForDeviceType[bmodel.DeviceType(dt)]
		assert.True(t, exist)
	}
}

func TestScenarioBroadcastSuccessOtherDevices(t *testing.T) {
	for _, dt := range model.VoiceDiscoveryDeviceTypes {
		_, exist := nlgScenarioBroadcastSuccessOtherDevices[bmodel.DeviceType(dt)]
		assert.True(t, exist)
	}
}

func TestHowToAllowedSpeakerNLG(t *testing.T) {
	for _, dt := range model.VoiceDiscoveryDeviceTypes {
		_, exist := nlgHowToAllowedSpeakerForDeviceType[bmodel.DeviceType(dt)]
		assert.True(t, exist)
	}
}

func TestHowToSearchAppNLG(t *testing.T) {
	for _, dt := range model.VoiceDiscoveryDeviceTypes {
		_, exist := nlgHowToSearchAppForDeviceType[bmodel.DeviceType(dt)]
		assert.True(t, exist)
	}
}

func TestHowToNotAllowedSpeakerAppNLG(t *testing.T) {
	for _, dt := range model.VoiceDiscoveryDeviceTypes {
		_, exist := nlgHowToNotAllowedSpeakerForDeviceType[bmodel.DeviceType(dt)]
		assert.True(t, exist)
	}
}
