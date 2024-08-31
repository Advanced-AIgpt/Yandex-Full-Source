package model_test

import (
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
)

func TestActionToOnOffCapability(t *testing.T) {
	a := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.OnOffCapabilityType.String(),
		Instance: string(model.OnOnOffCapabilityInstance),
		Unit:     nil,
		Relative: nil,
		Value:    true,
	}
	d := model.Device{}
	c := a.ToCapability(d)

	assert.IsType(t, model.OnOffCapabilityState{}, c.State())
	assert.Equal(t, true, c.State().(model.OnOffCapabilityState).Value)
}

func TestActionToToggleCapability(t *testing.T) {
	a := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.ToggleCapabilityType.String(),
		Instance: string(model.MuteToggleCapabilityInstance),
		Value:    true,
	}
	d := model.Device{}
	c := a.ToCapability(d)

	assert.IsType(t, model.ToggleCapabilityState{}, c.State())
	assert.Equal(t, true, c.State().(model.ToggleCapabilityState).Value)
}

func TestActionToColorCapabilityTemperatureKRaw(t *testing.T) {
	a := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.ColorSettingCapabilityType.String(),
		Instance: string(model.TemperatureKCapabilityInstance),
		Unit:     nil,
		Relative: nil,
		Value:    2700,
	}
	d := model.Device{}
	c := a.ToCapability(d)

	assert.IsType(t, model.ColorSettingCapabilityState{}, c.State())
	assert.Equal(t, model.TemperatureK(2700), c.State().(model.ColorSettingCapabilityState).Value, c.State())
}

func TestActionToColorCapabilityTemperatureKRelative1(t *testing.T) {
	a := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.ColorSettingCapabilityType.String(),
		Instance: string(model.TemperatureKCapabilityInstance),
		Unit:     nil,
		Relative: model.RT(model.Increase),
		Value:    nil,
	}

	c := model.NewCapability(model.ColorSettingCapabilityType)
	d := model.NewDevice("d1").WithCapabilities(*c)

	ac := a.ToCapability(*d)
	assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

	defaultColor := model.ColorPalette.GetDefaultWhiteColor()
	assert.Equal(t, defaultColor.ToColorSettingCapabilityState(model.TemperatureKCapabilityInstance), ac.State())
}

func TestActionToColorCapabilityTemperatureKRelative2(t *testing.T) {
	a := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.ColorSettingCapabilityType.String(),
		Instance: string(model.TemperatureKCapabilityInstance),
		Unit:     nil,
		Relative: model.RT(model.Increase),
		Value:    nil,
	}

	initialColor := model.ColorPalette.GetColorByTemperatureK(2700)
	initialColorT := initialColor.Temperature

	s := model.ColorSettingCapabilityState{
		Instance: model.TemperatureKCapabilityInstance,
		Value:    initialColorT,
	}
	p := model.ColorSettingCapabilityParameters{
		TemperatureK: &model.TemperatureKParameters{
			Min: 1000,
			Max: 10000,
		},
	}
	c := model.NewCapability(model.ColorSettingCapabilityType).WithParameters(p).WithState(s)
	d := model.NewDevice("d1").WithCapabilities(*c)

	ac := a.ToCapability(*d)
	assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

	nextColor := model.ColorPalette.FilterType(model.WhiteColor).GetNext(initialColor)
	assert.Equal(t, nextColor.ToColorSettingCapabilityState(model.TemperatureKCapabilityInstance), ac.State())
}

func TestActionToColorCapabilityTemperatureKRelative3(t *testing.T) {
	a := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.ColorSettingCapabilityType.String(),
		Instance: string(model.TemperatureKCapabilityInstance),
		Unit:     nil,
		Relative: model.RT(model.Increase),
		Value:    nil,
	}

	initialColor := model.ColorPalette.GetColorByTemperatureK(2700)
	initialColorT := initialColor.Temperature

	s := model.ColorSettingCapabilityState{
		Instance: model.TemperatureKCapabilityInstance,
		Value:    initialColorT,
	}
	p := model.ColorSettingCapabilityParameters{
		TemperatureK: &model.TemperatureKParameters{
			Min: 1000,
			Max: 3000,
		},
	}
	c := model.NewCapability(model.ColorSettingCapabilityType).WithParameters(p).WithState(s)
	d := model.NewDevice("d1").WithCapabilities(*c)

	ac := a.ToCapability(*d)
	assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

	assert.Equal(t, initialColor.ToColorSettingCapabilityState(model.TemperatureKCapabilityInstance), ac.State())
}

func TestActionToColorCapabilityTemperatureKRelative4(t *testing.T) {
	c := model.NewCapability(model.ColorSettingCapabilityType).
		WithParameters(model.ColorSettingCapabilityParameters{TemperatureK: &model.TemperatureKParameters{Max: 6546, Min: 2000}}).
		WithState(model.ColorSettingCapabilityState{Value: model.TemperatureK(5600), Instance: model.TemperatureKCapabilityInstance})

	d := model.NewDevice("d").WithID("d").WithCapabilities(*c)

	h := model.NewHypothesis().WithType(model.ActionHypothesisType)
	h.WithDevice("d")
	h.WithHypothesisValue(*model.NewActionHypothesis().
		WithCapabilityType(model.ColorSettingCapabilityType).
		WithInstance(string(model.TemperatureKCapabilityInstance)).
		WithRelative(model.Decrease))

	userInfo := model.UserInfo{
		Devices:   []model.Device{*d},
		Scenarios: []model.Scenario{},
	}
	_, filtered, _ := model.ExtractActions(nil, []model.Hypothesis{*h}, userInfo, false, false)

	assert.Equal(t, model.TemperatureK(4500), filtered[0].Devices[0].Capabilities[0].State().(model.ColorSettingCapabilityState).Value)
}

func TestActionToColorCapabilityColorHSV(t *testing.T) {
	color := model.ColorPalette["cyan"]
	a := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.ColorSettingCapabilityType.String(),
		Instance: string(model.HsvModelType),
		Unit:     nil,
		Relative: nil,
		Value:    string(color.ID),
	}
	c := model.NewCapability(model.ColorSettingCapabilityType).
		WithParameters(model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.HsvModelType)})
	d := model.NewDevice("d1").WithID("d1").WithCapabilities(c.ICapability)

	ac := a.ToCapability(*d)
	assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

	assert.Equal(t, color.ToColorSettingCapabilityState(model.HsvColorCapabilityInstance), ac.State())
}

func TestHypothesisActionToCapability(t *testing.T) {
	// Test range action
	percentUnit := model.UnitPercent
	rangeAction := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.BrightnessRangeInstance),
		Unit:     &percentUnit,
		Value:    float64(10),
	}

	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance: model.BrightnessRangeInstance,
		}).
		WithState(model.RangeCapabilityState{
			Instance: model.BrightnessRangeInstance,
			Value:    10,
		})

	device1 := model.NewDevice("d1").WithCapabilities(*rangeCapability)

	assert.Equal(t, rangeCapability.Retrievable(), rangeAction.ToCapability(*device1).Retrievable())
	assert.Equal(t, rangeCapability.Type(), rangeAction.ToCapability(*device1).Type())
	assert.Equal(t, rangeCapability.State(), rangeAction.ToCapability(*device1).State())

	// Test change hsv color action
	colorAction := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.ColorSettingCapabilityType.String(),
		Instance: string(model.HsvColorCapabilityInstance),
		Value:    string(model.ColorIDRed),
	}

	colorCapability := model.NewCapability(model.ColorSettingCapabilityType).
		WithParameters(model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.HsvModelType),
		}).
		WithState(model.ColorSettingCapabilityState{
			Instance: model.HsvColorCapabilityInstance,
			Value: model.HSV{
				H: 0, S: 96, V: 100,
			},
		})
	device2 := model.NewDevice("d2").WithCapabilities(colorCapability.ICapability)

	assert.Equal(t, colorCapability.State(), colorAction.ToCapability(*device2).State())
}

func TestAction_IRTvVolumeUp_NoRange_ToCapability(t *testing.T) {
	volumeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(false).
		WithParameters(model.RangeCapabilityParameters{
			Instance:     model.VolumeRangeInstance,
			Looped:       false,
			RandomAccess: false,
		})

	infraredTvDevice := model.NewDevice("tv-id-1").WithCapabilities(*volumeCapability)

	// Case 1: increase by 1
	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.VolumeRangeInstance),
		Relative: model.RT(model.Increase),
		Value:    nil,
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Relative: tools.AOB(true),
			Value:    3,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*infraredTvDevice))

	// Case 2: increase by 2
	action2 := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.VolumeRangeInstance),
		Relative: model.RT(model.Increase),
		Value:    float64(5),
	}

	expectedCapability2 := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability2.SetState(
		model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Relative: tools.AOB(true),
			Value:    5,
		})

	assert.Equal(t, expectedCapability2, action2.ToCapability(*infraredTvDevice))
}

func TestAction_IRTvChannel_WithoutRange_ToCapability(t *testing.T) {
	volumeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(false).
		WithParameters(model.RangeCapabilityParameters{
			Instance:     model.ChannelRangeInstance,
			Looped:       true,
			RandomAccess: true,
		})

	infraredTvDevice := model.NewDevice("tv-id-2").WithCapabilities(*volumeCapability)

	// CASE 1: Absolutive change
	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.ChannelRangeInstance),
		Value:    float64(127),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Value:    127,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*infraredTvDevice))

	// CASE 2: Relative change without value
	action2 := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.ChannelRangeInstance),
		Relative: model.RT(model.Increase),
		Value:    nil,
	}

	expectedCapability2 := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability2.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Relative: tools.AOB(true),
			Value:    1,
		})

	assert.Equal(t, expectedCapability2, action2.ToCapability(*infraredTvDevice))

	// CASE 3: Relative change with specific value
	action3 := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.ChannelRangeInstance),
		Relative: model.RT(model.Increase),
		Value:    float64(15),
	}

	expectedCapability3 := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability3.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Relative: tools.AOB(true),
			Value:    15,
		})

	assert.Equal(t, expectedCapability3, action3.ToCapability(*infraredTvDevice))
}

func TestAction_IRTvChannel_WithRange_ToCapability(t *testing.T) {
	volumeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(false).
		WithParameters(model.RangeCapabilityParameters{
			Instance:     model.ChannelRangeInstance,
			Looped:       true,
			RandomAccess: true,
			Range: &model.Range{
				Min:       1,
				Max:       999,
				Precision: 1,
			},
		})

	infraredTvDevice := model.NewDevice("tv-id-2").WithCapabilities(*volumeCapability)

	// CASE 1: Absolutive change
	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.ChannelRangeInstance),
		Value:    float64(127),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Value:    127,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*infraredTvDevice))

	// CASE 2: Relative change without value
	action2 := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.ChannelRangeInstance),
		Relative: model.RT(model.Increase),
		Value:    nil,
	}

	expectedCapability2 := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability2.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Relative: tools.AOB(true),
			Value:    1,
		})

	assert.Equal(t, expectedCapability2, action2.ToCapability(*infraredTvDevice))

	// CASE 3: Relative change with specific value
	action3 := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.ChannelRangeInstance),
		Relative: model.RT(model.Increase),
		Value:    float64(15),
	}

	expectedCapability3 := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability3.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Relative: tools.AOB(true),
			Value:    15,
		})

	assert.Equal(t, expectedCapability3, action3.ToCapability(*infraredTvDevice))
}

func TestAction_IRTvChannel_WithRange_WithRetrievable_ToCapability(t *testing.T) {
	volumeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(model.RangeCapabilityParameters{
			Instance:     model.ChannelRangeInstance,
			Looped:       true,
			RandomAccess: true,
			Range: &model.Range{
				Min:       1,
				Max:       999,
				Precision: 1,
			},
		}).
		WithState(model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Value:    15,
		})

	infraredTvDevice := model.NewDevice("tv-id-2").WithCapabilities(*volumeCapability).WithSkillID(model.TUYA)

	// CASE 1: Absolutive change
	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.ChannelRangeInstance),
		Value:    float64(127),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Value:    127,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*infraredTvDevice))

	// CASE 2: Relative change without value
	action2 := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.ChannelRangeInstance),
		Relative: model.RT(model.Increase),
		Value:    nil,
	}

	expectedCapability2 := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability2.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Value:    16,
		})

	assert.Equal(t, expectedCapability2, action2.ToCapability(*infraredTvDevice))

	// CASE 3: Relative change with specific value
	action3 := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.ChannelRangeInstance),
		Relative: model.RT(model.Increase),
		Value:    float64(15),
	}

	expectedCapability3 := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability3.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Value:    30,
		})

	assert.Equal(t, expectedCapability3, action3.ToCapability(*infraredTvDevice))
}

func TestActionToModeCapabilityRelative(t *testing.T) {
	modes := []model.Mode{
		model.KnownModes[model.HeatMode],
		model.KnownModes[model.CoolMode],
		model.KnownModes[model.AutoMode],
	}
	sort.Sort(model.ModesSorting(modes))
	c := model.NewCapability(model.ModeCapabilityType).
		WithParameters(model.ModeCapabilityParameters{Instance: model.ThermostatModeInstance, Modes: modes}).
		WithState(model.ModeCapabilityState{Instance: model.ThermostatModeInstance, Value: model.CoolMode})

	expectedCapability := model.MakeCapabilityByType(model.ModeCapabilityType)
	expectedCapability.SetState(
		model.ModeCapabilityState{
			Instance: model.ThermostatModeInstance,
			Value:    model.AutoMode,
		})

	modeTestDevice := model.NewDevice("thermostat-test").WithCapabilities(*c)

	actionDecrease := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.ModeCapabilityType.String(),
		Instance: string(model.ThermostatModeInstance),
		Relative: model.RT(model.Decrease),
	}

	assert.Equal(t, expectedCapability, actionDecrease.ToCapability(*modeTestDevice))

	actionIncrease := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.ModeCapabilityType.String(),
		Instance: string(model.ThermostatModeInstance),
		Relative: model.RT(model.Increase),
	}

	expectedCapability2 := model.MakeCapabilityByType(model.ModeCapabilityType)
	expectedCapability2.SetState(
		model.ModeCapabilityState{
			Instance: model.ThermostatModeInstance,
			Value:    model.HeatMode,
		})

	assert.Equal(t, expectedCapability2, actionIncrease.ToCapability(*modeTestDevice))
}

func TestActionToRangeCapabilityWithoutState(t *testing.T) {
	tempCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(model.RangeCapabilityParameters{
			Instance:     model.TemperatureRangeInstance,
			RandomAccess: true,
			Range: &model.Range{
				Min:       18,
				Max:       24,
				Precision: 1,
			},
		})

	device := model.NewDevice("ac-test").WithCapabilities(*tempCapability).WithSkillID(model.TUYA)

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.TemperatureRangeInstance,
			Value:    19,
		})

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.TemperatureRangeInstance),
		Relative: model.RT(model.Increase),
		Value:    float64(1),
	}

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityWithState(t *testing.T) {
	tempCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(model.RangeCapabilityParameters{
			Instance:     model.TemperatureRangeInstance,
			RandomAccess: true,
			Range: &model.Range{
				Min:       18,
				Max:       24,
				Precision: 1,
			},
		}).
		WithState(model.RangeCapabilityState{
			Instance: model.TemperatureRangeInstance,
			Value:    20,
		})

	device := model.NewDevice("ac-test").WithCapabilities(*tempCapability).WithSkillID(model.TUYA)

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.TemperatureRangeInstance,
			Value:    21,
		})

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.TemperatureRangeInstance),
		Relative: model.RT(model.Increase),
		Value:    float64(1),
	}

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToOnOffCapabilityInvert(t *testing.T) {
	tempCapability := model.NewCapability(model.OnOffCapabilityType).
		WithRetrievable(true).
		WithParameters(model.OnOffCapabilityParameters{}).
		WithState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    false,
		})

	device := model.NewDevice("lamp-1").WithCapabilities(*tempCapability)

	expectedCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	expectedCapability.SetState(
		model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.OnOffCapabilityType.String(),
		Instance: string(model.OnOnOffCapabilityInstance),
		Relative: model.RT(model.Invert),
	}

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToOnOffCapabilityInvertStateNil(t *testing.T) {
	tempCapability := model.NewCapability(model.OnOffCapabilityType).
		WithRetrievable(true).
		WithParameters(model.OnOffCapabilityParameters{})

	device := model.NewDevice("lamp-1").WithCapabilities(*tempCapability)

	expectedCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	expectedCapability.SetState(
		model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    false,
		})

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.OnOffCapabilityType.String(),
		Instance: string(model.OnOnOffCapabilityInstance),
		Relative: model.RT(model.Invert),
	}

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToToggleCapabilityInvert(t *testing.T) {
	tempCapability := model.NewCapability(model.ToggleCapabilityType).
		WithRetrievable(true).
		WithParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance}).
		WithState(model.ToggleCapabilityState{
			Instance: model.BacklightToggleCapabilityInstance,
			Value:    false,
		})

	device := model.NewDevice("mirror-1").WithCapabilities(*tempCapability)

	expectedCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	expectedCapability.SetState(
		model.ToggleCapabilityState{
			Instance: model.BacklightToggleCapabilityInstance,
			Value:    true,
		})

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.ToggleCapabilityType.String(),
		Instance: string(model.BacklightToggleCapabilityInstance),
		Relative: model.RT(model.Invert),
	}

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToToggleCapabilityInvertStateNil(t *testing.T) {
	tempCapability := model.NewCapability(model.ToggleCapabilityType).
		WithRetrievable(true).
		WithParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance})

	device := model.NewDevice("mirror-1").WithCapabilities(*tempCapability)

	expectedCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	expectedCapability.SetState(
		model.ToggleCapabilityState{
			Instance: model.BacklightToggleCapabilityInstance,
			Value:    false,
		})

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.ToggleCapabilityType.String(),
		Instance: string(model.BacklightToggleCapabilityInstance),
		Relative: model.RT(model.Invert),
	}

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityFractionalPrecision(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.TemperatureRangeInstance,
				Unit:         model.UnitTemperatureCelsius,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       18,
					Max:       30,
					Precision: 0.5,
				},
			})

	device := model.NewDevice("lg-temperature-1").WithCapabilities(*rangeCapability).WithSkillID(model.LGSkill)

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.TemperatureRangeInstance),
		Relative: model.RT(model.Increase),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.TemperatureRangeInstance,
			Value:    18.5,
		})
	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityBigMultiDeltaPrecision(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.TemperatureRangeInstance,
				Unit:         model.UnitTemperatureCelsius,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       18,
					Max:       30,
					Precision: 5,
				},
			})

	device := model.NewDevice("lg-temperature-1").WithCapabilities(*rangeCapability).WithSkillID(model.LGSkill)

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.TemperatureRangeInstance),
		Relative: model.RT(model.Increase),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.TemperatureRangeInstance,
			Value:    23,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityChannel(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       1,
					Max:       100,
					Precision: 2,
				},
			})

	device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.ChannelRangeInstance),
		Relative: model.RT(model.Increase),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Value:    3,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityUnretrievableChannel(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				RandomAccess: false,
				Looped:       false,
			})

	device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.ChannelRangeInstance),
		Relative: model.RT(model.Increase),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Relative: tools.AOB(true),
			Value:    1,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityVolume(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       1,
					Max:       100,
					Precision: 5,
				},
			})

	device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.VolumeRangeInstance),
		Relative: model.RT(model.Increase),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Value:    6,
		})
	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityBigRangeMultiDeltaInstance(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       1,
					Max:       1000,
					Precision: 5,
				},
			}).
		WithState(model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Value:    1,
		})
	device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.VolumeRangeInstance),
		Relative: model.RT(model.Increase),
	}
	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Value:    31,
		})
	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityMultiDeltaAdjustingBorder(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       1,
					Max:       100,
					Precision: 2,
				},
			}).
		WithState(model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Value:    1,
		})
	// we try to increase, adjusting border (5) is not multiple to precision, so we got 6
	device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.VolumeRangeInstance),
		Relative: model.RT(model.Increase),
	}
	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Value:    5,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityVolumeAdjustingBorder(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       1,
					Max:       100,
					Precision: 1,
				},
			}).
		WithState(model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Value:    1,
		})
	// we try to increase, adjusting border (5) is not multiple to precision, so we got 6
	device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.VolumeRangeInstance),
		Relative: model.RT(model.Increase),
	}
	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Value:    4,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityUnretrievableVolume(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				RandomAccess: false,
				Looped:       false,
			})

	device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.VolumeRangeInstance),
		Relative: model.RT(model.Increase),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Relative: tools.AOB(true),
			Value:    3,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityRelativeIncreaseWithTrueProvider(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       1,
					Max:       100,
					Precision: 5,
				},
			})
	rangeCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Value:    5,
		})

	device := model.NewDevice("tv-1").
		WithCapabilities(*rangeCapability).
		WithSkillID("sowow-awesome-skill")

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.VolumeRangeInstance),
		Relative: model.RT(model.Increase),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Relative: tools.AOB(true),
			Value:    5,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeCapabilityRelativeDecreaseWithTrueProvider(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       1,
					Max:       100,
					Precision: 5,
				},
			})
	rangeCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Value:    5,
		})

	device := model.NewDevice("tv-1").
		WithCapabilities(*rangeCapability).
		WithSkillID("sowow-awesome-skill")

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.VolumeRangeInstance),
		Relative: model.RT(model.Decrease),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.VolumeRangeInstance,
			Relative: tools.AOB(true),
			Value:    -5,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeOpenCapabilityRelative(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.OpenRangeInstance,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       1,
					Max:       100,
					Precision: 5,
				},
			})
	rangeCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.OpenRangeInstance,
			Value:    5,
		})

	device := model.NewDevice("щель").
		WithCapabilities(*rangeCapability).
		WithSkillID("so-wow-much-skill")

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.OpenRangeInstance),
		Value:    float64(10),
		Relative: model.RT(model.Increase),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.OpenRangeInstance,
			Relative: tools.AOB(true),
			Value:    10,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}

func TestActionToRangeOpenCapabilityRelativeWithoutValue(t *testing.T) {
	rangeCapability := model.NewCapability(model.RangeCapabilityType).
		WithRetrievable(true).
		WithParameters(
			model.RangeCapabilityParameters{
				Instance:     model.OpenRangeInstance,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       1,
					Max:       100,
					Precision: 5,
				},
			})
	rangeCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.OpenRangeInstance,
			Value:    5,
		})

	device := model.NewDevice("Окно").
		WithCapabilities(*rangeCapability).
		WithSkillID("so-wow-much-skill")

	action := model.HypothesisValue{
		Target:   model.CapabilityTarget,
		Type:     model.RangeCapabilityType.String(),
		Instance: string(model.OpenRangeInstance),
		Relative: model.RT(model.Increase),
	}

	expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	expectedCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.OpenRangeInstance,
			Relative: tools.AOB(true),
			Value:    20,
		})

	assert.Equal(t, expectedCapability, action.ToCapability(*device))
}
