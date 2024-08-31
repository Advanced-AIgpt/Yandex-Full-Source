package xtestadapter

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func DeviceActionSuccessResult(id string) adapter.DeviceActionResultView {
	return adapter.DeviceActionResultView{
		ID:           id,
		ActionResult: &adapter.StateActionResult{Status: adapter.DONE},
	}
}

func SwitchDeviceDiscoveryResult(id, name string) adapter.DeviceInfoView {
	return adapter.DeviceInfoView{
		ID:           id,
		Name:         name,
		Type:         model.SwitchDeviceType,
		Capabilities: []adapter.CapabilityInfoView{OnOffCapability()},
	}
}
