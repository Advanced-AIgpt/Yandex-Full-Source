package model

import (
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

const (
	BusyDeviceState    DeviceStateType = "BUSY"
	ReadyDeviceState   DeviceStateType = "READY"
	SuccessDeviceState DeviceStateType = "SUCCESS"
)

const (
	StateVerificationThreshold = time.Minute * 5
)

const (
	IoTVoiceDiscoveryScenarioIntent string = "voice_discovery"
)

const (
	VoiceDiscoveryConnectFrame          string = "alice.iot.voice_discovery"
	VoiceDiscoveryConnect2Frame         string = "alice.iot.voice_discovery.step2"
	VoiceDiscoveryBroadcastStartFrame   string = "alice.iot.voice_discovery.start"
	VoiceDiscoveryBroadcastSuccessFrame string = "alice.iot.voice_discovery.success"
	VoiceDiscoveryBroadcastFailureFrame string = "alice.iot.voice_discovery.failure"

	VoiceDiscoveryDiscoveryStartFrame   string = "alice.iot.voice_discovery.start.v2"
	VoiceDiscoveryDiscoverySuccessFrame string = "alice.iot.voice_discovery.success.v2"
	VoiceDiscoveryDiscoveryFailureFrame string = "alice.iot.voice_discovery.failure.v2"

	VoiceDiscoveryHowToFrame  string = "alice.iot.voice_discovery.how_to"
	VoiceDiscoveryCancelFrame string = "alice.iot.voice_discovery.cancel"
)

const (
	VoiceDiscoveryConnectingDeviceSlot     string = "connecting_device"
	VoiceDiscoveryConnectingDeviceSlotType string = "iot.device_type"
)

var VoiceDiscoveryDeviceTypes = []string{
	string(model.LightDeviceType),
	string(model.HubDeviceType),
	string(model.SocketDeviceType),
}
