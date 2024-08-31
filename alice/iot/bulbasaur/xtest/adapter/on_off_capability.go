package xtestadapter

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func OnOffCapability() adapter.CapabilityInfoView {
	return adapter.CapabilityInfoView{
		Reportable:  false,
		Retrievable: true,
		Type:        model.OnOffCapabilityType,
		Parameters:  model.OnOffCapabilityParameters{Split: false},
		State:       nil,
	}
}

func OnOffState(value bool, ts timestamp.PastTimestamp) adapter.CapabilityStateView {
	return adapter.CapabilityStateView{
		Type: model.OnOffCapabilityType,
		State: model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    value,
		},
		Timestamp: ts,
	}
}

func OnOffAction(value bool) adapter.CapabilityActionView {
	return adapter.CapabilityActionView{
		Type: model.OnOffCapabilityType,
		State: model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    value,
		},
	}
}

func OnOffActionSuccessResult(ts timestamp.PastTimestamp) adapter.CapabilityActionResultView {
	return adapter.CapabilityActionResultView{
		Type: model.OnOffCapabilityType,
		State: adapter.CapabilityStateActionResultView{
			Instance:     model.OnOnOffCapabilityInstance.String(),
			ActionResult: adapter.StateActionResult{Status: adapter.DONE},
		},
		Timestamp: ts,
	}
}

func OnOffActionErrorResult(errorCode adapter.ErrorCode, ts timestamp.PastTimestamp) adapter.CapabilityActionResultView {
	return adapter.CapabilityActionResultView{
		Type: model.OnOffCapabilityType,
		State: adapter.CapabilityStateActionResultView{
			Instance:     model.OnOnOffCapabilityInstance.String(),
			ActionResult: adapter.StateActionResult{Status: adapter.ERROR, ErrorCode: errorCode},
		},
		Timestamp: ts,
	}
}
