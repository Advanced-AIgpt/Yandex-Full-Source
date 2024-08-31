package adapter

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func TestActionResult_GetErrors(t *testing.T) {
	ar := ActionResult{Payload: ActionResultPayload{Devices: []DeviceActionResultView{
		{
			ID: "ext-dev-1",
			Capabilities: []CapabilityActionResultView{
				{
					Type: model.OnOffCapabilityType,
					State: CapabilityStateActionResultView{
						Instance: string(model.OnOnOffCapabilityInstance),
						ActionResult: StateActionResult{
							Status:    ERROR,
							ErrorCode: DeviceBusy,
						},
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: CapabilityStateActionResultView{
						Instance: string(model.BrightnessRangeInstance),
						ActionResult: StateActionResult{
							Status:    ERROR,
							ErrorCode: InvalidAction,
						},
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: CapabilityStateActionResultView{
						Instance: string(model.BrightnessRangeInstance),
						ActionResult: StateActionResult{
							Status:    ERROR,
							ErrorCode: ErrorCode(model.UnknownError),
						},
					},
				},
			},
		},
		{
			ID: "ext-dev-2",
			Capabilities: []CapabilityActionResultView{
				{
					Type: model.OnOffCapabilityType,
					State: CapabilityStateActionResultView{
						Instance: string(model.OnOnOffCapabilityInstance),
						ActionResult: StateActionResult{
							Status: DONE,
						},
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: CapabilityStateActionResultView{
						Instance: string(model.BrightnessRangeInstance),
						ActionResult: StateActionResult{
							Status:    ERROR,
							ErrorCode: InvalidAction,
						},
					},
				},
				{
					Type: model.ModeCapabilityType,
					State: CapabilityStateActionResultView{
						Instance: string(model.FanSpeedModeInstance),
						ActionResult: StateActionResult{
							Status:    ERROR,
							ErrorCode: InvalidValue,
						},
					},
				},
			},
		},
		{
			ID: "ext-dev-3",
			ActionResult: &StateActionResult{
				Status:    ERROR,
				ErrorCode: "SKIP_UNNORMALIZED",
			},
		},
	}}}

	expected := ErrorCodeCountMap{DeviceBusy: 1, InvalidAction: 2, InvalidValue: 1, ErrorCode(model.UnknownError): 1}
	actual := ar.GetErrors()

	assert.Equal(t, expected, actual)
}

func TestActionRequestFromDevices(t *testing.T) {
	brightnessState := model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    100.0,
	}
	brightnessCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	brightnessCapability.SetParameters(
		model.RangeCapabilityParameters{
			Instance: model.BrightnessRangeInstance,
			Unit:     "percent",
		})
	brightnessCapability.SetState(brightnessState)

	volumeState := model.RangeCapabilityState{
		Instance: model.VolumeRangeInstance,
		Value:    15,
	}
	volumeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	volumeCapability.SetState(volumeState)
	volumeCapability.SetParameters(
		model.RangeCapabilityParameters{
			Instance: model.VolumeRangeInstance,
		})

	customState := model.CustomButtonCapabilityState{
		Instance: "cb_2",
		Value:    true,
	}
	customButton := model.MakeCapabilityByType(model.CustomButtonCapabilityType)
	customButton.SetState(customState)
	customButton.SetParameters(
		model.CustomButtonCapabilityParameters{
			Instance:      "cb_2",
			InstanceNames: []string{"Мои маленькие пони"},
		})

	colorSettingState := model.ColorSettingCapabilityState{
		Instance: model.SceneCapabilityInstance,
		Value:    model.ColorSceneIDParty,
	}
	colorSetting := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorSetting.SetState(colorSettingState)
	colorSetting.SetParameters(model.ColorSettingCapabilityParameters{
		ColorSceneParameters: &model.ColorSceneParameters{
			Scenes: model.ColorScenes{
				{
					ID: model.ColorSceneIDFantasy,
				},
				{
					ID: model.ColorSceneIDParty,
				},
			},
		},
	})

	phraseState := model.QuasarServerActionCapabilityState{
		Instance: model.PhraseActionCapabilityInstance,
		Value:    "фразочка",
	}
	phrase := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
	phrase.SetState(phraseState)
	phrase.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

	textActionState := model.QuasarServerActionCapabilityState{
		Instance: model.TextActionCapabilityInstance,
		Value:    "командочка",
	}
	textAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
	textAction.SetState(textActionState)
	textAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance})

	stopEverythingState := model.QuasarCapabilityState{Instance: model.StopEverythingCapabilityInstance, Value: model.StopEverythingQuasarCapabilityValue{}}
	stopEverything := model.MakeCapabilityByType(model.QuasarCapabilityType)
	stopEverything.SetState(stopEverythingState)
	stopEverything.SetParameters(model.QuasarCapabilityParameters{Instance: model.StopEverythingCapabilityInstance})

	customData := struct {
		Type string
		Name string
	}{
		Type: "test-type",
		Name: "some-test-name",
	}

	device := model.Device{
		ID:           "test-id",
		OriginalType: "devices.types.light",
		ExternalID:   "ext-test-id",
		Capabilities: []model.ICapability{brightnessCapability, volumeCapability, customButton, phrase, textAction, colorSetting, stopEverything},
		CustomData:   customData,
	}

	var actionRequest ActionRequest
	actionRequest.FromDevices([]model.Device{device})
	var expectedActionRequest ActionRequest
	expectedActionRequest.Payload.Devices = []DeviceActionRequestView{
		{
			ID: "ext-test-id",
			Capabilities: []CapabilityActionView{
				{
					Type:  model.RangeCapabilityType,
					State: brightnessState,
				},
				{
					Type:  model.RangeCapabilityType,
					State: volumeState,
				},
				{
					Type:  model.CustomButtonCapabilityType,
					State: customState,
				},
				{
					Type:  model.QuasarServerActionCapabilityType,
					State: phraseState,
				},
				{
					Type:  model.QuasarServerActionCapabilityType,
					State: textActionState,
				},
				{
					Type:  model.ColorSettingCapabilityType,
					State: colorSettingState,
				},
				{
					Type:  model.QuasarCapabilityType,
					State: stopEverythingState,
				},
			},
			CustomData: customData,
		},
	}

	assert.Equal(t, expectedActionRequest, actionRequest)
	assert.Len(t, actionRequest.Payload.Devices, 1)
}
