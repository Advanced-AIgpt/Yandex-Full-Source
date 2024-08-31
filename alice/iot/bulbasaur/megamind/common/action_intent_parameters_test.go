package common

import (
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/tools"
	megamindcommonpb "a.yandex-team.ru/alice/megamind/protos/common"
)

// This test's purpose is to ensure all capability types are considered in CapabilityFromIntentParameters()
func TestCapabilityFromIntentParameters(t *testing.T) {
	device := model.Device{}

	check := func(capabilityType model.CapabilityType) {
		if !SupportedCapabilityTypes.Contains(capabilityType) {
			panic("unknown_type")
		}

		var intentParameters ActionIntentParameters

		switch capabilityType {
		case model.ColorSettingCapabilityType:
			intentParameters = ActionIntentParameters{
				CapabilityType:     model.ColorSettingCapabilityType,
				CapabilityInstance: string(model.TemperatureKCapabilityInstance),
				CapabilityValue:    5000,
			}
		case model.CustomButtonCapabilityType:
			intentParameters = ActionIntentParameters{
				CapabilityType:  model.CustomButtonCapabilityType,
				CapabilityValue: true,
			}
		case model.ModeCapabilityType:
			intentParameters = ActionIntentParameters{
				CapabilityType:     model.ModeCapabilityType,
				CapabilityInstance: string(model.ThermostatModeInstance),
				CapabilityValue:    string(model.HeatMode),
			}
		case model.OnOffCapabilityType:
			intentParameters = ActionIntentParameters{
				CapabilityType:     model.OnOffCapabilityType,
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
				CapabilityValue:    true,
			}
		case model.QuasarCapabilityType:
			intentParameters = ActionIntentParameters{
				CapabilityType:     model.QuasarCapabilityType,
				CapabilityInstance: string(model.WeatherCapabilityInstance),
				CapabilityValue:    model.WeatherQuasarCapabilityValue{},
			}
		case model.QuasarServerActionCapabilityType:
			intentParameters = ActionIntentParameters{
				CapabilityType:     model.QuasarServerActionCapabilityType,
				CapabilityInstance: string(model.PhraseActionCapabilityInstance),
				CapabilityValue:    model.TTSQuasarCapabilityValue{}.Text,
			}
		case model.RangeCapabilityType:
			intentParameters = ActionIntentParameters{
				CapabilityType:     model.RangeCapabilityType,
				CapabilityInstance: string(model.TemperatureRangeInstance),
				CapabilityValue:    42.0,
			}
		case model.ToggleCapabilityType:
			intentParameters = ActionIntentParameters{
				CapabilityType:     model.ToggleCapabilityType,
				CapabilityInstance: string(model.MuteToggleCapabilityInstance),
				CapabilityValue:    false,
			}
		case model.VideoStreamCapabilityType:
			intentParameters = ActionIntentParameters{
				CapabilityType:     model.VideoStreamCapabilityType,
				CapabilityInstance: string(model.GetStreamCapabilityInstance),
			}
		default:
			intentParameters = ActionIntentParameters{
				CapabilityType: capabilityType,
			}
		}

		_, err := CapabilityFromIntentParameters(device, intentParameters)
		assert.NoError(t, err, "make sure you've added the capability to CapabilityFromIntentParameters()")
	}

	for _, capabilityType := range SupportedCapabilityTypes {
		assert.NotPanics(t, func() {
			check(capabilityType)
		})
	}

	assert.Panics(t, func() {
		check("unknown_type")
	})
}

func TestUnmarshallingFromSlots(t *testing.T) {
	t.Run("missed_fields", func(t *testing.T) {
		rawSlotValue := `{
            "capability_type": "devices.capabilities.on_off",
            "capability_instance": "on",
            "capability_value": true
        }`

		slot := libmegamind.Slot{
			Name:  "name",
			Type:  "type",
			Value: rawSlotValue,
		}

		expectedParameters := ActionIntentParameters{
			CapabilityType:     "devices.capabilities.on_off",
			CapabilityInstance: "on",
			CapabilityValue:    true,
			CapabilityUnit:     "",
			RelativityType:     "",
		}

		var actualParameters ActionIntentParameters
		err := libmegamind.UnmarshalSlotValue(slot, &actualParameters)

		assert.NoError(t, err)
		assert.Equal(t, expectedParameters, actualParameters)
	})

	t.Run("all_fields", func(t *testing.T) {
		rawSlotValue := `{
			"capability_unit": "unit.temperature.celsius",
			"relativity_type": "increase",
            "capability_type": "devices.capabilities.range",
            "capability_instance": "temperature",
            "capability_value": 10
        }`

		slot := libmegamind.Slot{
			Name:  "name",
			Type:  "type",
			Value: rawSlotValue,
		}

		expectedParameters := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.TemperatureRangeInstance),
			CapabilityValue:    10.0,
			CapabilityUnit:     model.UnitTemperatureCelsius,
			RelativityType:     Increase,
		}

		var actualParameters ActionIntentParameters
		err := libmegamind.UnmarshalSlotValue(slot, &actualParameters)

		assert.NoError(t, err)
		assert.Equal(t, expectedParameters, actualParameters)
	})
}

func TestFillColorSettingCapability(t *testing.T) {
	t.Run("temperature_k_raw", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.ColorSettingCapabilityType,
			CapabilityInstance: string(model.TemperatureKCapabilityInstance),
			CapabilityValue:    2700,
		}

		d := model.Device{}
		c := &model.ColorSettingCapability{}
		c = fillColorSettingCapability(c, d, params)

		assert.IsType(t, model.ColorSettingCapabilityState{}, c.State())
		assert.Equal(t, model.TemperatureK(2700), c.State().(model.ColorSettingCapabilityState).Value, c.State())
	})

	t.Run("temperature_k_relative_1", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.ColorSettingCapabilityType,
			CapabilityInstance: string(model.TemperatureKCapabilityInstance),
			RelativityType:     Increase,
		}

		c := &model.ColorSettingCapability{}
		d := *model.NewDevice("d1").WithCapabilities(c)

		ac := fillColorSettingCapability(c, d, params)
		assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

		defaultColor := model.ColorPalette.GetDefaultWhiteColor()
		assert.Equal(t, defaultColor.ToColorSettingCapabilityState(model.TemperatureKCapabilityInstance), ac.State())
	})

	t.Run("temperature_k_relative_2", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.ColorSettingCapabilityType,
			CapabilityInstance: string(model.TemperatureKCapabilityInstance),
			RelativityType:     Increase,
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

		ac, ok := c.ICapability.(*model.ColorSettingCapability)
		assert.True(t, ok)

		ac = fillColorSettingCapability(ac, *d, params)
		assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

		nextColor := model.ColorPalette.FilterType(model.WhiteColor).GetNext(initialColor)
		assert.Equal(t, nextColor.ToColorSettingCapabilityState(model.TemperatureKCapabilityInstance), ac.State())
	})

	t.Run("temperature_k_relative_3", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.ColorSettingCapabilityType,
			CapabilityInstance: string(model.TemperatureKCapabilityInstance),
			RelativityType:     Increase,
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

		ac, ok := c.ICapability.(*model.ColorSettingCapability)
		assert.True(t, ok)

		ac = fillColorSettingCapability(ac, *d, params)
		assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

		assert.Equal(t, initialColor.ToColorSettingCapabilityState(model.TemperatureKCapabilityInstance), ac.State())
	})

	t.Run("temperature_k_relative_4", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.ColorSettingCapabilityType,
			CapabilityInstance: string(model.TemperatureKCapabilityInstance),
			RelativityType:     Decrease,
		}

		c := model.NewCapability(model.ColorSettingCapabilityType).
			WithParameters(model.ColorSettingCapabilityParameters{TemperatureK: &model.TemperatureKParameters{Max: 6546, Min: 2000}}).
			WithState(model.ColorSettingCapabilityState{Value: model.TemperatureK(5600), Instance: model.TemperatureKCapabilityInstance})

		d := model.NewDevice("d").WithID("d").WithCapabilities(*c)

		ac, ok := c.ICapability.(*model.ColorSettingCapability)
		assert.True(t, ok)

		ac = fillColorSettingCapability(ac, *d, params)
		assert.Equal(t, model.TemperatureK(4500), ac.State().(model.ColorSettingCapabilityState).Value)
	})

	t.Run("color_hsv_1", func(t *testing.T) {
		color := model.ColorPalette["cyan"]
		params := ActionIntentParameters{
			CapabilityType:     model.ColorSettingCapabilityType,
			CapabilityInstance: string(model.HsvColorCapabilityInstance),
			CapabilityValue:    string(color.ID),
		}

		c := model.NewCapability(model.ColorSettingCapabilityType).
			WithParameters(model.ColorSettingCapabilityParameters{
				ColorModel: model.CM(model.HsvModelType)})
		d := model.NewDevice("d1").WithID("d1").WithCapabilities(c.ICapability)

		ac := &model.ColorSettingCapability{}

		ac = fillColorSettingCapability(ac, *d, params)
		assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

		assert.Equal(t, color.ToColorSettingCapabilityState(model.HsvColorCapabilityInstance), ac.State())
	})

	t.Run("color_hsv_2", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.ColorSettingCapabilityType,
			CapabilityInstance: string(model.HsvColorCapabilityInstance),
			CapabilityValue:    string(model.ColorIDRed),
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
		device := model.NewDevice("d2").WithCapabilities(colorCapability.ICapability)
		ac := &model.ColorSettingCapability{}

		ac = fillColorSettingCapability(ac, *device, params)

		assert.Equal(t, colorCapability.State(), ac.State())
	})
}

func TestFillModeCapability(t *testing.T) {
	t.Run("mode_decrease", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.ModeCapabilityType,
			CapabilityInstance: string(model.ThermostatModeInstance),
			RelativityType:     Decrease,
		}

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

		d := model.NewDevice("thermostat-test").WithCapabilities(*c)
		ac := &model.ModeCapability{}
		ac = fillModeCapability(ac, *d, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("mode_increase", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.ModeCapabilityType,
			CapabilityInstance: string(model.ThermostatModeInstance),
			RelativityType:     Increase,
		}

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
				Value:    model.HeatMode,
			})

		d := model.NewDevice("thermostat-test").WithCapabilities(*c)
		ac := &model.ModeCapability{}
		ac = fillModeCapability(ac, *d, params)

		assert.Equal(t, expectedCapability, ac)
	})
}

func TestFillOnOffCapability(t *testing.T) {
	t.Run("on_off", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.OnOffCapabilityType,
			CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			CapabilityValue:    true,
		}

		d := model.Device{}
		c := &model.OnOffCapability{}
		c = fillOnOffCapability(c, d, params)

		assert.IsType(t, model.OnOffCapabilityState{}, c.State())
		assert.Equal(t, true, c.State().(model.OnOffCapabilityState).Value)
	})

	t.Run("on_off_invert", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.OnOffCapabilityType,
			CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			RelativityType:     Invert,
		}

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

		ac := &model.OnOffCapability{}

		ac = fillOnOffCapability(ac, *device, params)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("on_off_invert_state_nil", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.OnOffCapabilityType,
			CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			RelativityType:     Invert,
		}

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

		ac := &model.OnOffCapability{}

		ac = fillOnOffCapability(ac, *device, params)
		assert.Equal(t, expectedCapability, ac)
	})
}

func TestFillRangeCapability(t *testing.T) {
	t.Run("range_1", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.BrightnessRangeInstance),
			CapabilityUnit:     model.UnitPercent,
			CapabilityValue:    10.0,
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithParameters(model.RangeCapabilityParameters{
				Instance: model.BrightnessRangeInstance,
			}).
			WithState(model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    10,
			})

		device := model.NewDevice("d1").WithCapabilities(*rangeCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, rangeCapability.Retrievable(), ac.Retrievable())
		assert.Equal(t, rangeCapability.Type(), ac.Type())
		assert.Equal(t, rangeCapability.State(), ac.State())
	})

	t.Run("range_ir_tv_volume_up_1_no_range", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     Increase,
		}

		volumeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				Looped:       false,
				RandomAccess: false,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Relative: tools.AOB(true),
				Value:    3,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*volumeCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_volume_up_2_no_range", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     Increase,
			CapabilityValue:    5.0,
		}

		volumeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				Looped:       false,
				RandomAccess: false,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Relative: tools.AOB(true),
				Value:    5,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*volumeCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_absolute_no_range", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.ChannelRangeInstance),
			CapabilityValue:    127.0,
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    127,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_relative_no_value_no_range", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     Increase,
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Relative: tools.AOB(true),
				Value:    1,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_relative_with_value_no_range", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     Increase,
			CapabilityValue:    15.0,
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Relative: tools.AOB(true),
				Value:    15,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_absolute_with_range", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.ChannelRangeInstance),
			CapabilityValue:    127.0,
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    127,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_relative_no_value_with_range", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     Increase,
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Relative: tools.AOB(true),
				Value:    1,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_relative_with_value_with_range", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     Increase,
			CapabilityValue:    15.0,
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Relative: tools.AOB(true),
				Value:    15,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_with_range_retrievable_absolute", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.ChannelRangeInstance),
			CapabilityValue:    127.0,
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    127,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_with_range_retrievable_relative_no_value", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     Increase,
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    16,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_with_range_retrievable_relative_with_value", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.ChannelRangeInstance),
			CapabilityValue:    15.0,
			RelativityType:     Increase,
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    30,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_no_state", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.TemperatureRangeInstance),
			CapabilityValue:    1.0,
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.TemperatureRangeInstance,
				Value:    19,
			})

		device := model.NewDevice("ac-test").WithCapabilities(*tempCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_with_state", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.TemperatureRangeInstance),
			CapabilityValue:    1.0,
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.TemperatureRangeInstance,
				Value:    21,
			})

		device := model.NewDevice("ac-test").WithCapabilities(*tempCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_fractional_precision", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.TemperatureRangeInstance),
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.TemperatureRangeInstance,
				Value:    18.5,
			})

		device := model.NewDevice("ac-test").WithCapabilities(*rangeCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_big_multidelta_precision", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.TemperatureRangeInstance),
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.TemperatureRangeInstance,
				Value:    23,
			})

		device := model.NewDevice("ac-test").WithCapabilities(*rangeCapability)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_channel", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    3,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_unretrievable_channel", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     Increase,
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.ChannelRangeInstance,
					RandomAccess: false,
					Looped:       false,
				})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Relative: tools.AOB(true),
				Value:    1,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_volume", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    6,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_big_multidelta", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    31,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_multidelta_adjusting_border", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.VolumeCapabilityInstance),
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    5,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_volume_adjusting_border", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.VolumeCapabilityInstance),
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    4,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_unretrievable_volume", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.VolumeCapabilityInstance),
			RelativityType:     Increase,
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.VolumeRangeInstance,
					RandomAccess: false,
					Looped:       false,
				})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Relative: tools.AOB(true),
				Value:    3,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_relative_increase_with_true_provider", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.VolumeCapabilityInstance),
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Relative: tools.AOB(true),
				Value:    5,
			})

		device := model.NewDevice("tv-1").
			WithCapabilities(*rangeCapability).
			WithSkillID("sowow-awesome-skill")

		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_relative_decrease_with_true_provider", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.VolumeCapabilityInstance),
			RelativityType:     Decrease,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Relative: tools.AOB(true),
				Value:    -5,
			})

		device := model.NewDevice("tv-1").
			WithCapabilities(*rangeCapability).
			WithSkillID("sowow-awesome-skill")

		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_open_relative", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.OpenRangeInstance),
			CapabilityValue:    10.0,
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.OpenRangeInstance,
				Relative: tools.AOB(true),
				Value:    10,
			})

		device := model.NewDevice("щель").
			WithCapabilities(*rangeCapability).
			WithSkillID("so-wow-much-skill")

		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_open_relative_without_value", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.RangeCapabilityType,
			CapabilityInstance: string(model.OpenRangeInstance),
			RelativityType:     Increase,
		}

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

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.OpenRangeInstance,
				Relative: tools.AOB(true),
				Value:    20,
			})

		device := model.NewDevice("Окно").
			WithCapabilities(*rangeCapability).
			WithSkillID("so-wow-much-skill")

		ac := &model.RangeCapability{}

		ac = fillRangeCapability(ac, *device, params)

		assert.Equal(t, expectedCapability, ac)
	})
}

func TestFillToggleCapability(t *testing.T) {
	t.Run("mute_toggle", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.ToggleCapabilityType,
			CapabilityInstance: string(model.MuteToggleCapabilityInstance),
			CapabilityValue:    true,
		}

		d := model.Device{}
		c := &model.ToggleCapability{}

		c = fillToggleCapability(c, d, params)
		assert.IsType(t, model.ToggleCapabilityState{}, c.State())
		assert.Equal(t, true, c.State().(model.ToggleCapabilityState).Value)
	})

	t.Run("toggle_invert", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.ToggleCapabilityType,
			CapabilityInstance: string(model.BacklightToggleCapabilityInstance),
			RelativityType:     Invert,
		}

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

		ac := &model.ToggleCapability{}

		ac = fillToggleCapability(ac, *device, params)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("toggle_invert_state_nil", func(t *testing.T) {
		params := ActionIntentParameters{
			CapabilityType:     model.ToggleCapabilityType,
			CapabilityInstance: string(model.BacklightToggleCapabilityInstance),
			RelativityType:     Invert,
		}

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

		ac := &model.ToggleCapability{}

		ac = fillToggleCapability(ac, *device, params)
		assert.Equal(t, expectedCapability, ac)
	})
}

func TestFromProto(t *testing.T) {
	inputs := []struct {
		Name           string
		ParamsProto    *megamindcommonpb.TIoTActionIntentParameters
		ExpectedParams ActionIntentParameters
		ExpectedOk     bool
	}{
		{
			Name: "bool_value",
			ParamsProto: &megamindcommonpb.TIoTActionIntentParameters{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: "on",
				CapabilityValue: &megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue{
					RelativityType: "",
					Unit:           "",
					Value:          &megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue_BoolValue{BoolValue: true},
				},
			},
			ExpectedOk: true,
			ExpectedParams: ActionIntentParameters{
				CapabilityType:     model.OnOffCapabilityType,
				CapabilityInstance: "on",
				CapabilityValue:    true,
			},
		},
		{
			Name: "no_value_relative",
			ParamsProto: &megamindcommonpb.TIoTActionIntentParameters{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: "on",
				CapabilityValue: &megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue{
					RelativityType: string(Invert),
				},
			},
			ExpectedOk: true,
			ExpectedParams: ActionIntentParameters{
				CapabilityType:     model.OnOffCapabilityType,
				CapabilityInstance: "on",
				RelativityType:     Invert,
			},
		},
		{
			Name: "no_value",
			ParamsProto: &megamindcommonpb.TIoTActionIntentParameters{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: "on",
				CapabilityValue:    &megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue{},
			},
			ExpectedOk: false,
		},
		{
			Name: "num_value",
			ParamsProto: &megamindcommonpb.TIoTActionIntentParameters{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: model.TemperatureKCapabilityInstance.String(),
				CapabilityValue: &megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue{
					Value: &megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue_NumValue{NumValue: 3},
				},
			},
			ExpectedOk: true,
			ExpectedParams: ActionIntentParameters{
				CapabilityType:     model.RangeCapabilityType,
				CapabilityInstance: model.TemperatureKCapabilityInstance.String(),
				CapabilityValue:    3.0,
			},
		},
		{
			Name: "mode_value",
			ParamsProto: &megamindcommonpb.TIoTActionIntentParameters{
				CapabilityType:     string(model.ModeCapabilityType),
				CapabilityInstance: model.ThermostatModeInstance.String(),
				CapabilityValue: &megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue{
					Value: &megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue_ModeValue{ModeValue: string(model.CoolMode)},
				},
			},
			ExpectedOk: true,
			ExpectedParams: ActionIntentParameters{
				CapabilityType:     model.ModeCapabilityType,
				CapabilityInstance: model.ThermostatModeInstance.String(),
				CapabilityValue:    string(model.CoolMode),
			},
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			params := ActionIntentParameters{}
			var err error
			assert.NotPanics(t, func() {
				err = params.FromProto(input.ParamsProto)
			})

			if !input.ExpectedOk {
				assert.Error(t, err)
				return
			}

			assert.NoError(t, err)
			assert.Equal(t, input.ExpectedParams, params)
		})
	}
}
