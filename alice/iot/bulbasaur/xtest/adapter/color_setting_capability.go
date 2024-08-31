package xtestadapter

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func ColorSceneState(value model.ColorSceneID, ts timestamp.PastTimestamp) adapter.CapabilityStateView {
	return adapter.CapabilityStateView{
		Type: model.ColorSettingCapabilityType,
		State: model.ColorSettingCapabilityState{
			Instance: model.SceneCapabilityInstance,
			Value:    value,
		},
		Timestamp: ts,
	}
}

func ColorSceneAction(sceneID model.ColorSceneID) adapter.CapabilityActionView {
	return adapter.CapabilityActionView{
		Type: model.ColorSettingCapabilityType,
		State: model.ColorSettingCapabilityState{
			Instance: model.SceneCapabilityInstance,
			Value:    sceneID,
		},
	}
}

func ColorSettingActionSuccessResult(instance model.ColorSettingCapabilityInstance, ts timestamp.PastTimestamp) adapter.CapabilityActionResultView {
	return adapter.CapabilityActionResultView{
		Type: model.ColorSettingCapabilityType,
		State: adapter.CapabilityStateActionResultView{
			Instance:     string(instance),
			ActionResult: adapter.StateActionResult{Status: adapter.DONE},
		},
		Timestamp: ts,
	}
}

func ColorSettingActionErrorResult(instance model.ColorSettingCapabilityInstance, errorCode adapter.ErrorCode, ts timestamp.PastTimestamp) adapter.CapabilityActionResultView {
	return adapter.CapabilityActionResultView{
		Type: model.ColorSettingCapabilityType,
		State: adapter.CapabilityStateActionResultView{
			Instance:     string(instance),
			ActionResult: adapter.StateActionResult{Status: adapter.ERROR, ErrorCode: errorCode},
		},
		Timestamp: ts,
	}
}
