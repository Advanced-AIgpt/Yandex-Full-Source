package xtestadapter

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func ToggleCapability(instance model.ToggleCapabilityInstance) adapter.CapabilityInfoView {
	return adapter.CapabilityInfoView{
		Reportable:  false,
		Retrievable: true,
		Type:        model.ToggleCapabilityType,
		Parameters:  model.ToggleCapabilityParameters{Instance: instance},
		State:       nil,
	}
}

func ToggleState(instance model.ToggleCapabilityInstance, value bool, ts timestamp.PastTimestamp) adapter.CapabilityStateView {
	return adapter.CapabilityStateView{
		Type: model.ToggleCapabilityType,
		State: model.ToggleCapabilityState{
			Instance: instance,
			Value:    value,
		},
		Timestamp: ts,
	}
}

func ToggleAction(instance model.ToggleCapabilityInstance, value bool) adapter.CapabilityActionView {
	return adapter.CapabilityActionView{
		Type: model.ToggleCapabilityType,
		State: model.ToggleCapabilityState{
			Instance: instance,
			Value:    value,
		},
	}
}

func ToggleActionSuccessResult(instance model.ToggleCapabilityInstance, ts timestamp.PastTimestamp) adapter.CapabilityActionResultView {
	return adapter.CapabilityActionResultView{
		Type: model.ToggleCapabilityType,
		State: adapter.CapabilityStateActionResultView{
			Instance:     instance.String(),
			ActionResult: adapter.StateActionResult{Status: adapter.DONE},
		},
		Timestamp: ts,
	}
}

func ToggleActionErrorResult(instance model.ToggleCapabilityInstance, errorCode adapter.ErrorCode, ts timestamp.PastTimestamp) adapter.CapabilityActionResultView {
	return adapter.CapabilityActionResultView{
		Type: model.ToggleCapabilityType,
		State: adapter.CapabilityStateActionResultView{
			Instance:     instance.String(),
			ActionResult: adapter.StateActionResult{Status: adapter.ERROR, ErrorCode: errorCode},
		},
		Timestamp: ts,
	}
}
