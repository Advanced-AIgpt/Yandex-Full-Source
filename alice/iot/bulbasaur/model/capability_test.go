package model_test

import (
	"encoding/json"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/jsonmatcher"
	"github.com/stretchr/testify/assert"
)

func TestGetStateInstance(t *testing.T) {

	brightnessCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	brightnessCapability.SetState(model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    55.0,
	})

	volumeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	volumeCapability.SetState(model.RangeCapabilityState{
		Instance: model.VolumeRangeInstance,
		Value:    10.0,
	})

	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetState(model.OnOffCapabilityState{
		Instance: "on",
		Value:    true,
	})

	muteCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	muteCapability.SetState(model.ToggleCapabilityState{
		Instance: model.MuteToggleCapabilityInstance,
		Value:    true,
	})

	assert.Equal(t, string(model.BrightnessRangeInstance), brightnessCapability.Instance())
	assert.Equal(t, string(model.VolumeRangeInstance), volumeCapability.Instance())
	assert.Equal(t, string(model.OnOnOffCapabilityInstance), onOffCapability.Instance())
	assert.Equal(t, string(model.MuteToggleCapabilityInstance), muteCapability.Instance())
}

func TestGetInstance(t *testing.T) {
	brightnessRangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	brightnessRangeCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.BrightnessRangeInstance,
		Unit:     "percent",
	})

	volumeRangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	volumeRangeCapability.SetParameters(model.RangeCapabilityParameters{
		Instance: model.BrightnessRangeInstance,
		Unit:     "percent",
	})

	thermostatModeCapability := model.MakeCapabilityByType(model.ModeCapabilityType)
	thermostatModeCapability.SetParameters(model.ModeCapabilityParameters{
		Instance: model.ThermostatModeInstance,
	})

	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)

	colorSettingCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorSettingCapability.SetParameters(model.ColorSettingCapabilityParameters{
		nil,
		&model.TemperatureKParameters{
			Min: 1000,
			Max: 9000,
		},
		nil,
	})

	muteToggleCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	muteToggleCapability.SetParameters(model.ToggleCapabilityParameters{
		Instance: model.MuteToggleCapabilityInstance,
	})

	assert.Equal(t, string(model.BrightnessRangeInstance), brightnessRangeCapability.Instance())
	assert.Equal(t, string(model.BrightnessRangeInstance), volumeRangeCapability.Instance())
	assert.Equal(t, string(model.ThermostatModeInstance), thermostatModeCapability.Instance())
	assert.Equal(t, string(model.MuteToggleCapabilityInstance), muteToggleCapability.Instance())

	//assert.Panics(t, func() { onOffCapability.Instance() })
	//assert.Panics(t, func() { colorSettingCapability.Instance() })

	assert.Equal(t, string(model.OnOnOffCapabilityInstance), onOffCapability.Instance())
	assert.Equal(t, string(model.TemperatureKCapabilityInstance), colorSettingCapability.Instance())
}

func TestRangeParametersUnmarshal(t *testing.T) {
	input := []byte(`{
		"instance": "brightness",
		"range": {
			"max": 100,
			"min": 1,
			"precision": 1
		},
		"unit": "unit.percent"
	}`)

	var rcp1 model.RangeCapabilityParameters
	if err := json.Unmarshal(input, &rcp1); err != nil {
		t.Error(err)
		t.Fail()
	}

	expectedRcp1 := model.RangeCapabilityParameters{
		Instance:     model.BrightnessRangeInstance,
		Unit:         model.UnitPercent,
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Max:       100,
			Min:       1,
			Precision: 1,
		},
	}

	assert.Equal(t, expectedRcp1, rcp1)

	input = []byte(`{
		"random_access": false,
		"looped": true
	}`)

	var rcp2 model.RangeCapabilityParameters
	if err := json.Unmarshal(input, &rcp2); err != nil {
		t.Error(err)
		t.Fail()
	}

	expectedRcp2 := model.RangeCapabilityParameters{
		RandomAccess: false,
		Looped:       true,
	}

	assert.Equal(t, expectedRcp2, rcp2)
}

func TestCapabilitiesUnmarshal(t *testing.T) {
	inputJSON := `[
		{
			"type": "devices.capabilities.on_off",
			"reportable": true,
			"retrievable": true,
			"state": null,
			"parameters": {}
		},
		{
			"type": "devices.capabilities.mode",
			"reportable": true,
			"retrievable": true,
			"state": {
				"instance": "fan_speed",
				"value": "auto"
			},
			"parameters": {
				"instance": "fan_speed",
				"modes": [{"value":"auto"}, {"value":"heat"}]
			}
		},
		{
			"type": "devices.capabilities.range",
			"reportable": true,
			"retrievable": true,
			"state": null,
			"parameters": {
				"instance": "brightness",
				"looped": false,
				"random_access": true,
				"range": {
					"max": 100,
					"min": 1,
					"precision": 1
				},
				"unit": "unit.percent"
			}
		},
		{
			"type": "devices.capabilities.color_setting",
			"reportable": false,
			"retrievable": true,
			"parameters": {
				"color_model": "hsv",
				"temperature_k": {
					"max": 6500,
					"min": 2700
				},
				"color_scene": {
					"scenes": [
						{"id": "movie"}, {"id": "candle"}
					]
				}
			},
			"state": null
    	},
		{
			"type": "devices.capabilities.toggle",
			"reportable": false,
			"retrievable": true,
			"parameters": {
				"instance": "backlight"
			},
			"state": null
		}
	]`
	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)
	onOffCapability.SetReportable(true)

	modeCapability := model.MakeCapabilityByType(model.ModeCapabilityType)
	modeCapability.SetReportable(true)
	modeCapability.SetRetrievable(true)
	modeCapability.SetState(model.ModeCapabilityState{
		Instance: model.FanSpeedModeInstance,
		Value:    model.AutoMode,
	})
	modeCapability.SetParameters(model.ModeCapabilityParameters{
		Instance: model.FanSpeedModeInstance,
		Modes: []model.Mode{
			{
				Value: model.AutoMode,
			},
			{
				Value: model.HeatMode,
			},
		},
	})

	rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	rangeCapability.SetRetrievable(true)
	rangeCapability.SetReportable(true)
	rangeCapability.SetParameters(
		model.RangeCapabilityParameters{
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

	colorCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorCapability.SetRetrievable(true)
	colorCapability.SetParameters(
		model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.HsvModelType),
			TemperatureK: &model.TemperatureKParameters{
				Min: 2700,
				Max: 6500,
			},
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: model.ColorScenes{
					{
						ID: model.ColorSceneIDMovie,
					},
					{
						ID: model.ColorSceneIDCandle,
					},
				},
			},
		})

	toggleCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	toggleCapability.SetRetrievable(true)
	toggleCapability.SetParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance})

	expected := []model.ICapability{
		onOffCapability,
		modeCapability,
		rangeCapability,
		colorCapability,
		toggleCapability,
	}
	capabilities, err := model.JSONUnmarshalCapabilities([]byte(inputJSON))
	assert.NoError(t, err)
	assert.Equal(t, expected, capabilities)
}

func TestCapabilitiesMarshal(t *testing.T) {
	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetReportable(true)
	onOffCapability.SetRetrievable(true)

	modeCapability := model.MakeCapabilityByType(model.ModeCapabilityType)
	modeCapability.SetReportable(true)
	modeCapability.SetRetrievable(true)
	modeCapability.SetState(model.ModeCapabilityState{
		Instance: model.FanSpeedModeInstance,
		Value:    model.AutoMode,
	})
	modeCapability.SetParameters(model.ModeCapabilityParameters{
		Instance: model.FanSpeedModeInstance,
		Modes: []model.Mode{
			{
				Value: model.AutoMode,
			},
			{
				Value: model.HeatMode,
			},
		},
	})

	rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	rangeCapability.SetReportable(true)
	rangeCapability.SetRetrievable(true)
	rangeCapability.SetParameters(
		model.RangeCapabilityParameters{
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

	colorCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorCapability.SetReportable(false)
	colorCapability.SetRetrievable(true)
	colorCapability.SetParameters(
		model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.HsvModelType),
			TemperatureK: &model.TemperatureKParameters{
				Min: 2700,
				Max: 6500,
			},
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: model.ColorScenes{
					{
						ID: model.ColorSceneIDMovie,
					},
					{
						ID: model.ColorSceneIDCandle,
					},
				},
			},
		})

	toggleCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	toggleCapability.SetReportable(false)
	toggleCapability.SetRetrievable(true)
	toggleCapability.SetParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance})

	capabilities := []model.ICapability{onOffCapability, modeCapability, rangeCapability, colorCapability, toggleCapability}

	expected := `[
		{
			"type": "devices.capabilities.on_off",
			"reportable": true,
			"retrievable": true,
			"last_updated": 0,
			"state": null,
			"parameters": {
				"split": false
			}
		},
		{
			"type": "devices.capabilities.mode",
			"reportable": true,
			"retrievable": true,
			"last_updated": 0,
			"state": {
				"instance": "fan_speed",
				"value": "auto"
			},
			"parameters": {
				"instance": "fan_speed",
				"modes": [{"value":"auto"}, {"value":"heat"}]
			}
		},
		{
			"type": "devices.capabilities.range",
			"reportable": true,
			"retrievable": true,
			"last_updated": 0,
			"state": null,
			"parameters": {
				"instance": "brightness",
				"looped": false,
				"random_access": true,
				"range": {
					"max": 100,
					"min": 1,
					"precision": 1
				},
				"unit": "unit.percent"
			}
		},
		{
			"type": "devices.capabilities.color_setting",
			"reportable": false,
			"retrievable": true,
			"last_updated": 0,
			"parameters": {
				"color_model": "hsv",
				"temperature_k": {
					"max": 6500,
					"min": 2700
				},
				"color_scene": {
					"scenes": [
						{"id":"movie"},{"id":"candle"}
					]
				}
			},
			"state": null
    	},
		{
			"type": "devices.capabilities.toggle",
			"reportable": false,
			"retrievable": true,
			"last_updated": 0,
			"parameters": {
				"instance": "backlight"
			},
			"state": null
		}
	]`
	actual, err := json.Marshal(capabilities)
	assert.NoError(t, err)
	assert.NoError(t, jsonmatcher.JSONContentsMatch(expected, string(actual)))
}

func TestCapabilitiesEqual(t *testing.T) {
	firstMode := model.MakeCapabilityByType(model.ModeCapabilityType)
	firstMode.SetReportable(true)
	firstMode.SetParameters(
		model.ModeCapabilityParameters{
			Instance: model.FanSpeedModeInstance,
			Modes: []model.Mode{
				{
					Value: model.AutoMode,
				},
				{
					Value: model.HeatMode,
				},
			},
		})
	secondMode := model.MakeCapabilityByType(model.ModeCapabilityType)
	secondMode.SetReportable(true)
	secondMode.SetParameters(firstMode.Parameters())

	assert.True(t, firstMode.Equal(secondMode))
	assert.True(t, secondMode.Equal(firstMode))

	secondMode.SetParameters(
		model.ModeCapabilityParameters{
			Instance: model.FanSpeedModeInstance,
			Modes: []model.Mode{
				{
					Value: model.AutoMode,
				},
				{
					Value: model.HeatMode,
				},
				{
					Value: model.EcoMode,
				},
			},
		})

	assert.False(t, firstMode.Equal(secondMode))
	assert.False(t, secondMode.Equal(firstMode))

	firstColor := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	firstColor.SetRetrievable(true)
	firstColor.SetParameters(
		model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.HsvModelType),
			TemperatureK: &model.TemperatureKParameters{
				Min: 2700,
				Max: 6500,
			},
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: model.ColorScenes{
					{
						ID: model.ColorSceneIDMovie,
					},
					{
						ID: model.ColorSceneIDCandle,
					},
				},
			},
		})
	secondColor := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	secondColor.SetRetrievable(true)
	secondColor.SetParameters(firstColor.Parameters())

	assert.True(t, firstColor.Equal(secondColor))
	assert.True(t, secondColor.Equal(firstColor))

	secondColor.SetState(
		model.ColorSettingCapabilityState{
			Instance: model.TemperatureKCapabilityInstance,
			Value:    model.TemperatureK(6500),
		})

	assert.False(t, firstColor.Equal(secondColor))
	assert.False(t, secondColor.Equal(firstColor))

	// different types of capabilities
	assert.False(t, firstMode.Equal(firstColor))
}

func TestCapabilitiesClone(t *testing.T) {
	capability := model.MakeCapabilityByType(model.ModeCapabilityType)
	capability.SetReportable(true)
	capability.SetParameters(
		model.ModeCapabilityParameters{
			Instance: model.FanSpeedModeInstance,
			Modes: []model.Mode{
				{
					Value: model.AutoMode,
				},
				{
					Value: model.HeatMode,
				},
			},
		})
	cloneCap := capability.Clone()
	cloneCapAnother := cloneCap.Clone()
	assert.Equal(t, capability, cloneCap)
	assert.Equal(t, capability, cloneCapAnother)
}

func TestCapabilitiesPopulateInternals(t *testing.T) {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetReportable(true)
	onOff.SetRetrievable(true)
	onOff.SetParameters(model.OnOffCapabilityParameters{Split: true})
	onOff.SetState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true})

	modeCap := model.MakeCapabilityByType(model.ModeCapabilityType)
	modeCap.SetReportable(true)
	modeCap.SetRetrievable(true)
	modeCap.SetParameters(model.ModeCapabilityParameters{
		Instance: model.ProgramModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.AutoMode],
			model.KnownModes[model.ExpressMode],
		},
	})
	modeCap.SetState(model.ModeCapabilityState{Instance: model.ProgramModeInstance, Value: model.AutoMode})

	colorCap := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorCap.SetReportable(true)
	colorCap.SetRetrievable(true)
	colorCap.SetParameters(model.ColorSettingCapabilityParameters{
		TemperatureK: &model.TemperatureKParameters{
			Min: 100,
			Max: 10000,
		},
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
	colorCap.SetState(model.ColorSettingCapabilityState{Instance: model.TemperatureKCapabilityInstance, Value: model.TemperatureK(100)})

	toggle := model.MakeCapabilityByType(model.ToggleCapabilityType)
	toggle.SetReportable(true)
	toggle.SetRetrievable(true)
	toggle.SetParameters(model.ToggleCapabilityParameters{Instance: model.PauseToggleCapabilityInstance})
	toggle.SetState(model.ToggleCapabilityState{Instance: model.PauseToggleCapabilityInstance, Value: true})

	rangeCap := model.MakeCapabilityByType(model.RangeCapabilityType)
	rangeCap.SetReportable(true)
	rangeCap.SetRetrievable(true)
	rangeCap.SetParameters(model.RangeCapabilityParameters{
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
	rangeCap.SetState(model.RangeCapabilityState{Instance: model.BrightnessRangeInstance, Value: 100})

	customButton := model.MakeCapabilityByType(model.CustomButtonCapabilityType)
	customButton.SetRetrievable(true)
	customButton.SetParameters(model.CustomButtonCapabilityParameters{
		Instance:      "1",
		InstanceNames: []string{"кнопка"},
	})
	customButton.SetState(model.CustomButtonCapabilityState{Instance: "1", Value: true})

	quasarServerActionCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
	quasarServerActionCap.SetRetrievable(true)
	quasarServerActionCap.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})
	quasarServerActionCap.SetState(model.QuasarServerActionCapabilityState{Instance: model.PhraseActionCapabilityInstance, Value: "хахаха"})

	quasarCap := model.MakeCapabilityByType(model.QuasarCapabilityType)
	quasarCap.SetRetrievable(true)
	quasarCap.SetParameters(model.QuasarCapabilityParameters{Instance: model.StopEverythingCapabilityInstance})
	quasarCap.SetState(model.QuasarCapabilityState{Instance: model.StopEverythingCapabilityInstance, Value: model.StopEverythingQuasarCapabilityValue{}})

	startingCapabilities := model.Capabilities{
		onOff, modeCap, colorCap, toggle, rangeCap, customButton, quasarServerActionCap, quasarCap,
	}
	containers := make(model.Capabilities, 0)
	for _, c := range startingCapabilities {
		stateContainer := model.MakeCapabilityByType(c.Type())
		stateContainer.SetState(c.State())
		containers = append(containers, stateContainer)
	}
	containers.PopulateInternals(startingCapabilities)
	assert.Equal(t, startingCapabilities, containers)
}

func TestCapabilitiesFilterByActualCapabilities(t *testing.T) {
	on := model.MakeCapabilityByType(model.OnOffCapabilityType)
	on.SetState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true})

	off := model.MakeCapabilityByType(model.OnOffCapabilityType)
	off.SetState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: false})

	customButton := model.MakeCapabilityByType(model.CustomButtonCapabilityType)
	customButton.SetParameters(model.CustomButtonCapabilityParameters{Instance: "111111", InstanceNames: []string{"кнопочка"}})

	capabilities := model.Capabilities{on, customButton}
	actual := model.Capabilities{off}

	// state should not be affected by filtration
	expected := model.Capabilities{on}
	assert.Equal(t, expected, capabilities.FilterByActualCapabilities(actual))
}

func TestKnownInternalCapabilities(t *testing.T) {
	type testCase struct {
		capabilityType     model.CapabilityType
		expectedIsInternal bool
	}
	testCases := []testCase{
		{
			capabilityType:     model.OnOffCapabilityType,
			expectedIsInternal: false,
		},
		{
			capabilityType:     model.RangeCapabilityType,
			expectedIsInternal: false,
		},
		{
			capabilityType:     model.ToggleCapabilityType,
			expectedIsInternal: false,
		},
		{
			capabilityType:     model.ModeCapabilityType,
			expectedIsInternal: false,
		},
		{
			capabilityType:     model.CustomButtonCapabilityType,
			expectedIsInternal: true,
		},
		{
			capabilityType:     model.QuasarServerActionCapabilityType,
			expectedIsInternal: true,
		},
		{
			capabilityType:     model.QuasarCapabilityType,
			expectedIsInternal: true,
		},
	}
	for _, tc := range testCases {
		capability := model.MakeCapabilityByType(tc.capabilityType)
		assert.Equal(t, capability.IsInternal(), tc.expectedIsInternal)
	}
}

func TestCapabilitiesDropByTypeAndInstance(t *testing.T) {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})
	pause := model.MakeCapabilityByType(model.ToggleCapabilityType)
	pause.SetParameters(model.ToggleCapabilityParameters{Instance: model.PauseToggleCapabilityInstance})
	pause.SetState(model.ToggleCapabilityState{
		Instance: model.PauseToggleCapabilityInstance,
		Value:    true,
	})
	expected := model.Capabilities{onOff}
	actual := model.Capabilities{onOff, pause}.DropByTypeAndInstance(model.ToggleCapabilityType, string(model.PauseToggleCapabilityInstance))
	assert.Equal(t, expected, actual)
}
