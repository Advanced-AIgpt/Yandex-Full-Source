package xtestadapter

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func QuasarActionSuccessResult(instance model.QuasarCapabilityInstance, ts timestamp.PastTimestamp) adapter.CapabilityActionResultView {
	return adapter.CapabilityActionResultView{
		Type: model.QuasarCapabilityType,
		State: adapter.CapabilityStateActionResultView{
			Instance:     string(instance),
			ActionResult: adapter.StateActionResult{Status: adapter.DONE},
		},
		Timestamp: ts,
	}
}

func QuasarActionInProgressResult(instance model.QuasarCapabilityInstance, ts timestamp.PastTimestamp) adapter.CapabilityActionResultView {
	return adapter.CapabilityActionResultView{
		Type: model.QuasarCapabilityType,
		State: adapter.CapabilityStateActionResultView{
			Instance:     string(instance),
			ActionResult: adapter.StateActionResult{Status: adapter.INPROGRESS},
		},
		Timestamp: ts,
	}
}

func QuasarActionErrorResult(instance model.QuasarCapabilityInstance, errorCode adapter.ErrorCode, ts timestamp.PastTimestamp) adapter.CapabilityActionResultView {
	return adapter.CapabilityActionResultView{
		Type: model.QuasarCapabilityType,
		State: adapter.CapabilityStateActionResultView{
			Instance: string(instance),
			ActionResult: adapter.StateActionResult{
				Status:    adapter.ERROR,
				ErrorCode: errorCode,
			},
		},
		Timestamp: ts,
	}
}

func QuasarServerActionSuccessResult(instance model.QuasarServerActionCapabilityInstance, ts timestamp.PastTimestamp) adapter.CapabilityActionResultView {
	return adapter.CapabilityActionResultView{
		Type: model.QuasarServerActionCapabilityType,
		State: adapter.CapabilityStateActionResultView{
			Instance:     string(instance),
			ActionResult: adapter.StateActionResult{Status: adapter.DONE},
		},
		Timestamp: ts,
	}
}
