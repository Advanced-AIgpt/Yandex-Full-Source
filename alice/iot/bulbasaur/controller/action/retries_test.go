package action

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
)

func Test_FilterRequestAndMergeActionResults(t *testing.T) {
	// prepare devices data

	// 1. lamp
	lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	lampOnOff.SetRetrievable(true)
	lampRange := model.MakeCapabilityByType(model.RangeCapabilityType)
	lampRange.SetRetrievable(true)
	lampRange.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.BrightnessRangeInstance,
		Unit:         model.UnitPercent,
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})
	lampColor := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	lampColor.SetRetrievable(true)
	lampColor.SetParameters(model.ColorSettingCapabilityParameters{
		TemperatureK: &model.TemperatureKParameters{
			Min: 100,
			Max: 10000,
		},
	})
	lampToggle := model.MakeCapabilityByType(model.ToggleCapabilityType)
	lampToggle.SetRetrievable(true)
	lampToggle.SetParameters(model.ToggleCapabilityParameters{Instance: model.PauseToggleCapabilityInstance})

	// 2. socket
	socketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	socketOnOff.SetRetrievable(true)
	socketToggle := model.MakeCapabilityByType(model.ToggleCapabilityType)
	socketToggle.SetRetrievable(true)
	socketToggle.SetParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance})

	// 3. failed device with internal error but retrievable onOff - retriable
	failedOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	failedOnOff.SetRetrievable(true)

	// 4. unexisting device in answer
	unexistingOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	unexistingOnOff.SetRetrievable(true)

	devices := model.Devices{
		{
			ID:           "1-lamp",
			ExternalID:   "external-1-lamp",
			Capabilities: model.Capabilities{lampOnOff, lampRange, lampColor, lampToggle},
		},
		{
			ID:           "2-socket",
			ExternalID:   "external-2-socket",
			Capabilities: model.Capabilities{socketOnOff, socketToggle},
		},
		{
			ID:           "3-failed-device",
			ExternalID:   "external-3-failed-device",
			Capabilities: model.Capabilities{failedOnOff},
		},
		{
			ID:           "4-not-existing-device",
			ExternalID:   "external-4-not-existing-device",
			Capabilities: model.Capabilities{unexistingOnOff},
		},
	}

	// original request
	actionRequest := adapter.ActionRequest{
		Payload: adapter.ActionRequestPayload{
			Devices: []adapter.DeviceActionRequestView{
				{
					ID: "external-1-lamp",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Relative: tools.AOB(true),
								Value:    30,
							},
						},
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.TemperatureKCapabilityInstance,
								Value:    model.TemperatureK(6000),
							},
						},
						{
							Type: model.ToggleCapabilityType,
							State: model.ToggleCapabilityState{
								Instance: model.PauseToggleCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
				{
					ID: "external-2-socket",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type: model.ToggleCapabilityType,
							State: model.ToggleCapabilityState{
								Instance: model.BacklightToggleCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
				{
					ID: "external-3-failed-device",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
				{
					ID: "external-4-not-existing-device",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
			},
		},
	}

	// firstResult
	expectedFirstResult := adapter.ActionResult{
		RequestID: "first",
		Payload: adapter.ActionResultPayload{
			Devices: []adapter.DeviceActionResultView{
				{
					ID: "external-1-lamp",
					Capabilities: []adapter.CapabilityActionResultView{
						{
							Type: model.ColorSettingCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.TemperatureKCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status: adapter.DONE,
								},
							},
						},
						{
							Type: model.OnOffCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.OnOnOffCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status: adapter.DONE,
								},
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.BrightnessRangeInstance),
								ActionResult: adapter.StateActionResult{
									Status:       adapter.ERROR,
									ErrorCode:    adapter.DeviceUnreachable,
									ErrorMessage: "brightness is unreachable, but relative, so should not be retried",
								},
							},
						},
						{
							Type: model.ToggleCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.PauseToggleCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status:       adapter.ERROR,
									ErrorCode:    adapter.InvalidAction,
									ErrorMessage: "invalid action on toggle, should not be retried",
								},
							},
						},
					},
					ActionResult: &adapter.StateActionResult{
						Status: adapter.DONE,
					},
				},
				{
					ID:           "external-2-socket",
					Capabilities: []adapter.CapabilityActionResultView{},
					ActionResult: &adapter.StateActionResult{
						Status:       adapter.ERROR,
						ErrorCode:    adapter.DeviceUnreachable,
						ErrorMessage: "socket provided only full device state, should be fully retried",
					},
				},
				{
					ID:           "external-3-failed-device",
					Capabilities: []adapter.CapabilityActionResultView{},
					ActionResult: &adapter.StateActionResult{
						Status:       adapter.ERROR,
						ErrorCode:    adapter.InternalError,
						ErrorMessage: "should be retried",
					},
				},
			},
		},
	}
	expectedSecondRequest := adapter.ActionRequest{
		Payload: adapter.ActionRequestPayload{
			Devices: []adapter.DeviceActionRequestView{
				{
					ID: "external-2-socket",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type: model.ToggleCapabilityType,
							State: model.ToggleCapabilityState{
								Instance: model.BacklightToggleCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
				{
					ID: "external-3-failed-device",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
			},
		},
	}
	actualFirstResult := mergeActionResults(adapter.ActionResult{}, expectedFirstResult)
	actualSecondRequest := filterRequestOnCapabilitiesDone(actionRequest, devices, actualFirstResult)

	assert.Equal(t, expectedFirstResult, actualFirstResult, "first result")
	assert.Equal(t, expectedSecondRequest, actualSecondRequest, "second request")

	secondResult := adapter.ActionResult{
		RequestID: "second",
		Payload: adapter.ActionResultPayload{
			Devices: []adapter.DeviceActionResultView{
				{
					ID:           "external-2-socket",
					Capabilities: []adapter.CapabilityActionResultView{},
					ActionResult: &adapter.StateActionResult{
						Status: adapter.DONE,
					},
				},
				{
					ID:           "external-3-failed-device",
					Capabilities: []adapter.CapabilityActionResultView{},
					ActionResult: &adapter.StateActionResult{
						Status:       adapter.ERROR,
						ErrorCode:    adapter.InternalError,
						ErrorMessage: "should be retried",
					},
				},
			},
		},
	}

	// it should be merged with first result
	expectedSecondResult := adapter.ActionResult{
		RequestID: "second",
		Payload: adapter.ActionResultPayload{
			Devices: []adapter.DeviceActionResultView{
				{
					ID: "external-1-lamp",
					Capabilities: []adapter.CapabilityActionResultView{
						{
							Type: model.ColorSettingCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.TemperatureKCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status: adapter.DONE,
								},
							},
						},
						{
							Type: model.OnOffCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.OnOnOffCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status: adapter.DONE,
								},
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.BrightnessRangeInstance),
								ActionResult: adapter.StateActionResult{
									Status:       adapter.ERROR,
									ErrorCode:    adapter.DeviceUnreachable,
									ErrorMessage: "brightness is unreachable, but relative, so should not be retried",
								},
							},
						},
						{
							Type: model.ToggleCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.PauseToggleCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status:       adapter.ERROR,
									ErrorCode:    adapter.InvalidAction,
									ErrorMessage: "invalid action on toggle, should not be retried",
								},
							},
						},
					},
					ActionResult: &adapter.StateActionResult{
						Status: adapter.DONE,
					},
				},
				{
					ID:           "external-2-socket",
					Capabilities: []adapter.CapabilityActionResultView{},
					ActionResult: &adapter.StateActionResult{
						Status: adapter.DONE,
					},
				},
				{
					ID:           "external-3-failed-device",
					Capabilities: []adapter.CapabilityActionResultView{},
					ActionResult: &adapter.StateActionResult{
						Status:       adapter.ERROR,
						ErrorCode:    adapter.InternalError,
						ErrorMessage: "should be retried",
					},
				},
			},
		},
	}
	expectedThirdRequest := adapter.ActionRequest{
		Payload: adapter.ActionRequestPayload{
			Devices: []adapter.DeviceActionRequestView{
				{
					ID: "external-3-failed-device",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
			},
		},
	}

	actualSecondResult := mergeActionResults(actualFirstResult, secondResult)
	actualThirdRequest := filterRequestOnCapabilitiesDone(actualSecondRequest, devices, actualSecondResult)

	assert.Equal(t, expectedSecondResult, actualSecondResult, "second result")
	assert.Equal(t, expectedThirdRequest, actualThirdRequest, "third request")
}
