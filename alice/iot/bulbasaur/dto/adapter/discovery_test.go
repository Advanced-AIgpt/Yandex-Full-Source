package adapter

import (
	"context"
	"reflect"
	"testing"

	"github.com/stretchr/testify/assert"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	btest "a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

func TestCapabilityInfoView(t *testing.T) {
	testCases := []struct {
		name        string
		input       string
		expectValue CapabilityInfoView
		expectErr   error
	}{
		//invalid
		{
			name: "invalid_json",
			input: `{
				"msg": {
					"description
					"authority": "the Bruce Willis"
				}
			`,
			expectErr: xerrors.New("json: syntax error at 33: invalid character '\\n' in string literal"),
		},
		{
			name: "no_type",
			input: `{
				"msg": {
					"description": "dynamite",
					"authority": "the Bruce Willis"
				}
			}`,
			expectErr: xerrors.New("capability type is empty"),
		},
		{
			name: "invalid_type",
			input: `{
				"type": "sound",
				"msg": {
					"description": "dynamite",
					"authority": "the Bruce Willis"
				}
			}`,
			expectErr: xerrors.New("unknown capability type: sound"),
		},
		{
			name: "invalid_color_model",
			input: `{
				"type": "devices.capabilities.color_setting",
				"parameters": {
					"color_model": "merid"
				}
			}`,
			expectErr: xerrors.New("unknown color_model: merid"),
		},
		{
			name: "invalid_color_temperature",
			input: `{
				"type": "devices.capabilities.color_setting",
				"parameters": {
					"temperature_k": {
						"min": 10,
						"max": 10000
					}
				}
			}`,
			expectErr: xerrors.New("min temperature_k cannot be less than 100"),
		},
		{
			name: "invalid_empty_scenes",
			input: `{
				"type": "devices.capabilities.color_setting",
				"parameters": {
					"color_scene": {
						"scenes": []
					}
				}
			}`,
			expectErr: xerrors.New("color_setting validation failed: expected at least one scene in scenes list"),
		},
		{
			name: "invalid_empty_modes",
			input: `{
				"type": "devices.capabilities.mode",
				"parameters": {
					"instance": "fan_speed",
					"modes": []
				}
			}`,
			expectErr: xerrors.New("mode validation failed: expected at least one mode in modes list"),
		},
		{
			name: "invalid_unknown_mode_instance",
			input: `{
				"type": "devices.capabilities.mode",
				"parameters": {
					"instance": "buttonka",
					"modes": [
						{"value": "eco"},
						{"value": "dry"}
					]
				}
			}`,
			expectErr: xerrors.New("mode validation failed: unknown instance: buttonka"),
		},
		{
			name: "invalid_unknown_mode_value",
			input: `{
				"type": "devices.capabilities.mode",
				"parameters": {
					"instance": "thermostat",
					"modes": [
						{"value": "eco"},
						{"value": "tacos"}
					]
				}
			}`,
			expectErr: xerrors.New("mode validation failed: unknown mode value: tacos"),
		},
		{
			name: "invalid_mode_no_parameters",
			input: `{
		       "type": "devices.capabilities.mode",
		       "retrievable": true
		   }`,
			expectErr: xerrors.New("cannot parse capability: parameters for mode capability type is missing"),
		},
		{
			name: "invalid_toggle_no_parameters",
			input: `{
		       "type": "devices.capabilities.toggle",
		       "retrievable": true
		   }`,
			expectErr: xerrors.New("cannot parse capability: parameters for toggle capability type is missing"),
		},
		{
			name: "invalid_unknown_toggle_instance",
			input: `{
				"type": "devices.capabilities.toggle",
				"parameters": {
					"instance": "moggle"
				}
			}`,
			expectErr: xerrors.New("unknown toggle instance: moggle"),
		},
		{
			name: "invalid_unknown_range_instance",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "grunge",
					"unit": "unit.percent",
					"range": {
						"min": 0,
						"max": 100,
						"precision": 1
					}
				}
			}`,
			expectErr: xerrors.New("unknown range instance: grunge"),
		},
		{
			name: "invalid_range_brightness_unit",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "brightness",
					"unit": "unit.temperature.celsius",
					"range": {
						"min": 0,
						"max": 100,
						"precision": 1
					}
				}
			}`,
			expectErr: xerrors.New("unacceptable unit for brightness instance: unit.temperature.celsius"),
		},
		{
			name: "invalid_range_temperature_unit",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "temperature",
					"unit": "unit.percent",
					"range": {
						"min": 0,
						"max": 100,
						"precision": 1
					}
				}
			}`,
			expectErr: xerrors.New("unacceptable unit for temperature instance: unit.percent"),
		},
		{
			name: "invalid_range_brightness_values",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "brightness",
					"unit": "unit.percent",
					"range": {
						"min": 10,
						"max": 101,
						"precision": 150
					}
				}
			}`,
			expectErr: xerrors.New(
				"brightness range min param must be 0 or 1, not 10.000000; " +
					"brightness range max param must be 100, not 101.000000; " +
					"range precision param cannot be bigger or equal than available range",
			),
		},
		{
			name: "invalid_range_brightness_precision",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "brightness",
					"unit": "unit.percent",
					"range": {
						"min": 0,
						"max": 100,
						"precision": -1
					}
				}
			}`,
			expectErr: xerrors.New("range precision param cannot be less or equal to zero"),
		},
		{
			name: "invalid_range_brightness_no_range",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "brightness",
					"unit": "unit.percent"
				}
			}`,
			expectErr: xerrors.New("range parameter is required for brightness instance"),
		},
		{
			name: "invalid_range_temperature_no_range",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "temperature",
					"unit": "unit.temperature.celsius"
				}
			}`,
			expectErr: xerrors.New("range parameter is required for temperature instance"),
		},
		{
			name: "invalid_range_volume_unit",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "volume",
					"unit": "unit.temperature.celsius",
					"range": {
						"min": 0,
						"max": 100,
						"precision": 1
					}
				}
			}`,
			expectErr: xerrors.New("unacceptable unit for volume instance: unit.temperature.celsius"),
		},

		//valid
		{
			name: "valid_on_off",
			input: `{
				"type": "devices.capabilities.on_off"
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.OnOffCapabilityType,
				Parameters:  model.OnOffCapabilityParameters{},
			},
		},
		{
			name: "valid_retrievable_false_on_off",
			input: `{
				"type": "devices.capabilities.on_off",
				"retrievable": false
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: false,
				Type:        model.OnOffCapabilityType,
				Parameters:  model.OnOffCapabilityParameters{},
			},
		},
		{
			name: "valid_color_setting_temperature_and_hsv",
			input: `{
				"type": "devices.capabilities.color_setting",
				"parameters": {
					"color_model": "hsv",
					"temperature_k": {
						"min": 1000,
						"max": 10000
					}
				}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					ColorModel: model.CM(model.HsvModelType),
					TemperatureK: &model.TemperatureKParameters{
						Min: 1000,
						Max: 10000,
					},
				},
			},
		},
		{
			name: "valid_color_setting_only_scenes",
			input: `{
				"type": "devices.capabilities.color_setting",
				"parameters": {
					"color_scene": {
						"scenes": [
							{"id": "movie"}, {"id": "party"}
						]
					}
				}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					ColorSceneParameters: &model.ColorSceneParameters{
						Scenes: model.ColorScenes{
							{
								ID: model.ColorSceneIDMovie,
							},
							{
								ID: model.ColorSceneIDParty,
							},
						},
					},
				},
			},
		},
		{
			name: "valid_color_setting_only_rgb",
			input: `{
				"type": "devices.capabilities.color_setting",
				"parameters": {
					"color_model": "rgb"
				}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					ColorModel: model.CM(model.RgbModelType),
				},
			},
		},
		{
			name: "valid_color_setting_only_temperature",
			input: `{
				"type": "devices.capabilities.color_setting",
				"parameters": {
					"temperature_k": {
						"min": 1000,
						"max": 10000
					}
				}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1000,
						Max: 10000,
					},
				},
			},
		},
		{
			name: "valid_color_setting_only_temperature",
			input: `{
				"type": "devices.capabilities.color_setting",
				"reportable": true,
				"parameters": {
					"temperature_k": {
						"min": 1000,
						"max": 10000
					}
				}
			}`,
			expectValue: CapabilityInfoView{
				Reportable:  true,
				Retrievable: true,
				Type:        model.ColorSettingCapabilityType,
				Parameters: model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1000,
						Max: 10000,
					},
				},
			},
		},
		{
			name: "valid_thermostat_modes",
			input: `{
				"type": "devices.capabilities.mode",
				"parameters": {
					"instance": "thermostat",
					"modes": [
						{"value": "eco"},
						{"value": "dry"}
					]
				}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.ModeCapabilityType,
				Parameters: model.ModeCapabilityParameters{
					Instance: model.ThermostatModeInstance,
					Modes: []model.Mode{
						{
							Value: model.EcoMode,
						},
						{
							Value: model.DryMode,
						},
					},
				},
			},
		},
		{
			name: "valid_fan_modes",
			input: `{
				"type": "devices.capabilities.mode",
				"reportable": true,
				"parameters": {
					"instance": "fan_speed",
					"modes": [
						{"value": "auto"},
						{"value": "low"}
					]
				}
			}`,
			expectValue: CapabilityInfoView{
				Reportable:  true,
				Retrievable: true,
				Type:        model.ModeCapabilityType,
				Parameters: model.ModeCapabilityParameters{
					Instance: model.FanSpeedModeInstance,
					Modes: []model.Mode{
						{
							Value: model.AutoMode,
						},
						{
							Value: model.LowMode,
						},
					},
				},
			},
		},
		{
			name: "valid_brightness_range_1_percent",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "brightness",
					"unit": "unit.percent",
					"range": {
						"min": 1,
						"max": 100,
						"precision": 1
					}
				}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				},
			},
		},
		{
			name: "valid_brightness_range_0_percent",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "brightness",
					"unit": "unit.percent",
					"range": {
						"min": 0,
						"max": 100,
						"precision": 1
					}
				}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       0,
						Max:       100,
						Precision: 1,
					},
				},
			},
		},
		{
			name: "valid_range_channel",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "channel",
					"random_access": true,
					"looped": false
				}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.ChannelRangeInstance,
					RandomAccess: true,
					Looped:       false,
				},
			},
		},
		{
			name: "valid_range_volume",
			input: `{
				"type": "devices.capabilities.range",
				"parameters": {
					"instance": "volume",
					"unit": "unit.percent",
					"range": {
						"min": 0,
						"max": 100,
						"precision": 1
					}
				}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance:     model.VolumeRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       0,
						Max:       100,
						Precision: 1,
					},
				},
			},
		},
		{
			name: "split on_off parameters",
			input: `{
				"type": "devices.capabilities.on_off",
				"retrievable": false,
				"parameters": {
					"split": true
				}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: false,
				Type:        model.OnOffCapabilityType,
				Parameters: model.OnOffCapabilityParameters{
					Split: true,
				},
			},
		},
		{
			name: "split on_off parameters",
			input: `{
				"type": "devices.capabilities.on_off"
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.OnOffCapabilityType,
				Parameters: model.OnOffCapabilityParameters{
					Split: false,
				},
			},
		},
		{
			name: "valid phrase",
			input: `{
				"type": "devices.capabilities.quasar.server_action",
				"parameters": {"instance": "phrase_action"}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.QuasarServerActionCapabilityType,
				Parameters:  model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance},
			},
		},
		{
			name: "valid text action",
			input: `{
				"type": "devices.capabilities.quasar.server_action",
				"parameters": {"instance": "text_action"}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.QuasarServerActionCapabilityType,
				Parameters:  model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance},
			},
		},
		{
			name: "valid text action",
			input: `{
				"type": "devices.capabilities.quasar.server_action",
				"parameters": {"instance": "text_action"}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.QuasarServerActionCapabilityType,
				Parameters:  model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance},
			},
		},
		{
			name: "valid quasar weather",
			input: `{
				"type": "devices.capabilities.quasar",
				"parameters": {"instance": "weather"}
			}`,
			expectValue: CapabilityInfoView{
				Retrievable: true,
				Type:        model.QuasarCapabilityType,
				Parameters:  model.QuasarCapabilityParameters{Instance: model.WeatherCapabilityInstance},
			},
		},
		{
			name: "valid reportable on_off",
			input: `{
				"type": "devices.capabilities.on_off",
				"parameters": {"split": false},
				"reportable": true
			}`,
			expectValue: CapabilityInfoView{
				Reportable:  true,
				Retrievable: true,
				Type:        model.OnOffCapabilityType,
				Parameters:  model.OnOffCapabilityParameters{Split: false},
			},
		},
		{
			name: "valid reportable range",
			input: `{
				"type": "devices.capabilities.range",
				"retrievable": true,
				"reportable": true,
				"parameters": {
					"instance": "brightness",
					"unit": "unit.percent",
					"random_access": false,
					"range": {
						"min": 0,
						"max": 100,
						"precision": 10
					}
				}
			}`,
			expectValue: CapabilityInfoView{
				Reportable:  true,
				Retrievable: true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance: model.BrightnessRangeInstance,
					Unit:     model.UnitPercent,
					Range:    &model.Range{Max: 100.0, Min: 0.0, Precision: 10},
				},
			},
		},
		{
			name: "valid reportable toggle",
			input: `{
				"type": "devices.capabilities.toggle",
				"reportable": true,
				"retrievable": true,
				"parameters": {"instance": "pause" }
			}`,
			expectValue: CapabilityInfoView{
				Reportable:  true,
				Retrievable: true,
				Type:        model.ToggleCapabilityType,
				Parameters: model.ToggleCapabilityParameters{
					Instance: model.PauseToggleCapabilityInstance,
				},
			},
		},
		{
			name: "valid reportable range with state",
			input: `{
				"type": "devices.capabilities.range",
				"retrievable": true,
				"reportable": true,
				"parameters": {
					"instance": "brightness",
					"unit": "unit.percent",
					"random_access": false,
					"range": {
						"min": 0,
						"max": 100,
						"precision": 10
					}
				},
                "state": {
                    "instance": "brightness",
                    "value": 40.5
                }
			}`,
			expectValue: CapabilityInfoView{
				Reportable:  true,
				Retrievable: true,
				Type:        model.RangeCapabilityType,
				Parameters: model.RangeCapabilityParameters{
					Instance: model.BrightnessRangeInstance,
					Unit:     model.UnitPercent,
					Range:    &model.Range{Max: 100.0, Min: 0.0, Precision: 10},
				},
				State: model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    40.5,
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			var c CapabilityInfoView
			err := binder.Bind(valid.NewValidationCtx(), []byte(tc.input), &c)

			if tc.expectErr == nil {
				assert.NoError(t, err)
				assert.Equal(t, tc.expectValue, c)
			} else {
				assert.EqualError(t, err, tc.expectErr.Error())
			}
		})
	}
}

func TestPropertyInfoView(t *testing.T) {
	testCases := []struct {
		name        string
		input       string
		expectValue PropertyInfoView
		expectErr   error
	}{
		//invalid
		{
			name: "invalid_property_no_type",
			input: `{
				"retrievable": false,
				"parameters": {
					"instance": "amperage"
				}
			}`,
			expectErr: valid.Errors{
				xerrors.New("property type is empty"),
			},
		},
		{
			name: "valid_property_retrievable_false",
			input: `{
                "type": "devices.properties.float",
				"retrievable": false,
				"parameters": {
					"instance": "amperage",
					"unit": "unit.ampere"
				}
			}`,
			expectValue: PropertyInfoView{
				Type:        model.FloatPropertyType,
				Retrievable: false,
				Parameters: model.FloatPropertyParameters{
					Instance: model.AmperagePropertyInstance,
					Unit:     model.UnitAmpere,
				},
			},
		},
		{
			name: "invalid_property_no_instance",
			input: `{
                "type": "devices.properties.float",
				"retrievable": true,
				"parameters": {}
			}`,
			expectErr: valid.Errors{
				xerrors.New("unknown float property instance: \"\""),
			},
		},
		{
			name: "invalid_property_many_errors",
			input: `{
                "type": "devices.properties.float",
				"retrievable": false,
				"parameters": {
					"instance": "amperage"
				}
			}`,
			expectErr: valid.Errors{
				xerrors.New("\"amperage\" instance unit should be \"unit.ampere\", not \"\""),
			},
		},
		//valid
		{
			name: "valid_property_amperage",
			input: `{
                "type": "devices.properties.float",
				"retrievable": true,
				"parameters": {
					"instance": "amperage",
					"unit": "unit.ampere"
				}
			}`,
			expectValue: PropertyInfoView{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.AmperagePropertyInstance,
					Unit:     model.UnitAmpere,
				},
			},
		},
		{
			name: "valid_property_timer",
			input: `{
                "type": "devices.properties.float",
				"retrievable": true,
				"parameters": {
					"instance": "timer",
					"unit": "unit.time.seconds"
				}
			}`,
			expectValue: PropertyInfoView{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.TimerPropertyInstance,
					Unit:     model.UnitTimeSeconds,
				},
			},
		},
		{
			name: "valid_property_amperage_with_state",
			input: `{
                "type": "devices.properties.float",
				"retrievable": true,
				"parameters": {
					"instance": "amperage",
					"unit": "unit.ampere"
				},
                "state": {
                    "instance": "amperage",
                    "value": 0.1
                }
			}`,
			expectValue: PropertyInfoView{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.AmperagePropertyInstance,
					Unit:     model.UnitAmpere,
				},
				State: model.FloatPropertyState{
					Instance: model.AmperagePropertyInstance,
					Value:    0.1,
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			var p PropertyInfoView
			err := binder.Bind(valid.NewValidationCtx(), []byte(tc.input), &p)

			if tc.expectErr == nil {
				assert.NoError(t, err)
				assert.Equal(t, tc.expectValue, p)
			} else {
				assert.EqualError(t, err, tc.expectErr.Error())
			}
		})
	}
}

func TestDeviceInfoView(t *testing.T) {
	testCases := []struct {
		name            string
		input           string
		createCtx       func() *valid.ValidationCtx
		expectValue     DeviceInfoView
		skillID         string
		getExpectDevice func() model.Device
		expectErr       error
	}{
		// invalid
		{
			name: "invalid_device_duplicate_capability",
			input: `{
				"id": "abc-123",
				"name": "лампa",
				"description": "цветная лампа филипс",
				"room": "спальня",
				"type": "devices.types.light",
				"capabilities": [			{
						"type": "devices.capabilities.on_off",
						"retrievable": false
					}, {
						"type": "devices.capabilities.on_off",
						"retrievable": false
					}
				]
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectErr: bulbasaur.Errors{xerrors.New("duplicated capability found - devices.capabilities.on_off:on")},
		},
		{
			name: "invalid_device_duplicate_property",
			input: `{
				"id": "abc-123",
				"name": "розетка",
				"description": "умная розетка филипс",
				"room": "спальня",
				"type": "devices.types.socket",
				"capabilities": [{
						"type": "devices.capabilities.on_off",
						"retrievable": false
					}
				],
				"properties": [
					{
                        "type": "devices.properties.float",
						"parameters": {"instance":"amperage", "unit":"unit.ampere"}
					},
					{
                        "type": "devices.properties.float",
						"parameters": {"instance":"amperage", "unit":"unit.ampere"}
					}
				]
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectErr: bulbasaur.Errors{xerrors.New("duplicated property found - devices.properties.float:amperage")},
		},
		{
			name: "invalid_device_zero_capabilities_and_properties",
			input: `{
				"id": "abc-123",
				"name": "розетка",
				"description": "умная розетка филипс",
				"room": "спальня",
				"type": "devices.types.socket",
				"capabilities": [],
				"properties": []
			}`,
			createCtx: func() *valid.ValidationCtx {
				ctx := context.Background()
				return NewDiscoveryInfoViewValidationContext(ctx, btest.NopLogger(), model.PhilipsSkill, true)
			},
			expectErr: bulbasaur.Errors{xerrors.New("device must contain at least one property or capability")},
		},
		{
			name: "invalid_skill_id_for_custom_buttons",
			input: `{
				"id": "abc-123",
				"name": "розетка",
				"description": "умная розетка филипс",
				"room": "спальня",
				"type": "devices.types.socket",
				"capabilities": [
					{
						"type": "devices.capabilities.custom.button",
						"reportable": true,
						"retrievable": true,
						"parameters": {
							"instance": "1111111",
							"instance_names": ["kek", "cheburek"]
						}
					}
				],
				"properties": []
			}`,
			createCtx: func() *valid.ValidationCtx {
				ctx := context.Background()
				return NewDiscoveryInfoViewValidationContext(ctx, btest.NopLogger(), model.PhilipsSkill, true)
			},
			expectErr: bulbasaur.Errors{xerrors.New("unsupported capability type: devices.capabilities.custom.button")},
		},
		// valid
		{
			name: "valid_philips_lamp",
			input: `{
				"id": "abc-123",
				"name": "лампa",
				"description": "цветная лампа филипс",
				"room": "спальня",
				"type": "devices.types.light",
				"capabilities": [{
						"type": "devices.capabilities.range",
						"retrievable": true,
						"parameters": {
							"instance": "brightness",
							"unit": "unit.percent",
							"range": {
								"min": 0,
								"max": 100,
								"precision": 10
							},
							"incremental": true,
							"absolute": true
						}
					},
					{
						"type": "devices.capabilities.on_off",
						"retrievable": false
					},
					{
						"type": "devices.capabilities.color_setting",
						"parameters": {
							"color_model": "hsv",
							"temperature_k": {
								"min": 2700,
								"max": 9000
							}
						}
					}
				],
				"device_info": {
					"manufacturer": "Philips N.V.",
					"model": "hue g11",
					"hw_version": "1.2",
					"sw_version": "5.4"
				}
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectValue: DeviceInfoView{
				ID:          "abc-123",
				Name:        "лампa",
				Description: "цветная лампа филипс",
				Type:        model.LightDeviceType,
				Capabilities: []CapabilityInfoView{
					{
						Type:        model.RangeCapabilityType,
						Retrievable: true,
						Parameters: model.RangeCapabilityParameters{
							Instance:     model.BrightnessRangeInstance,
							RandomAccess: true,
							Looped:       false,
							Unit:         model.UnitPercent,
							Range: &model.Range{
								Min:       0,
								Max:       100,
								Precision: 10,
							},
						},
					},
					{
						Type:        model.OnOffCapabilityType,
						Retrievable: false,
						Parameters:  model.OnOffCapabilityParameters{},
					},
					{
						Type: model.ColorSettingCapabilityType,
						Parameters: model.ColorSettingCapabilityParameters{
							ColorModel: model.CM(model.HsvModelType),
							TemperatureK: &model.TemperatureKParameters{
								Min: 2700,
								Max: 9000,
							},
						},
						Retrievable: true,
					},
				},
				Room: "спальня",
				DeviceInfo: &model.DeviceInfo{
					Manufacturer: tools.AOS("Philips N.V."),
					Model:        tools.AOS("hue g11"),
					HwVersion:    tools.AOS("1.2"),
					SwVersion:    tools.AOS("5.4"),
				},
			},
			skillID: "philips",
			getExpectDevice: func() model.Device {
				onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
				onOff.SetParameters(model.OnOffCapabilityParameters{})

				brightnessRange := model.MakeCapabilityByType(model.RangeCapabilityType)
				brightnessRange.SetRetrievable(true)
				brightnessRange.SetParameters(model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					RandomAccess: true,
					Looped:       false,
					Unit:         model.UnitPercent,
					Range: &model.Range{
						Min:       0,
						Max:       100,
						Precision: 10,
					},
				})

				colorSetting := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
				colorSetting.SetRetrievable(true)
				colorSetting.SetParameters(model.ColorSettingCapabilityParameters{
					ColorModel: model.CM(model.HsvModelType),
					TemperatureK: &model.TemperatureKParameters{
						Min: 2700,
						Max: 9000,
					},
				})
				return model.Device{
					ID:           "",
					Name:         "лампa",
					Description:  tools.AOS("цветная лампа филипс"),
					ExternalID:   "abc-123",
					ExternalName: "лампa",
					SkillID:      "philips",
					Type:         model.LightDeviceType,
					OriginalType: model.LightDeviceType,
					Room:         &model.Room{Name: "спальня"},
					Capabilities: model.Capabilities{
						brightnessRange, onOff, colorSetting,
					},
					Properties: model.Properties{},
					DeviceInfo: &model.DeviceInfo{
						Manufacturer: tools.AOS("Philips N.V."),
						Model:        tools.AOS("hue g11"),
						HwVersion:    tools.AOS("1.2"),
						SwVersion:    tools.AOS("5.4"),
					},
				}
			},
			expectErr: nil,
		},
		{
			name: "valid_tuya_hub_zero_capabilities_and_properties",
			input: `{
				"id": "abc-123",
				"name": "хаб",
				"description": "умный хаб",
				"room": "спальня",
				"type": "devices.types.hub",
				"capabilities": [],
				"properties": [],
				"device_info": {
					"manufacturer": "Philips N.V.",
					"model": "hue g11",
					"hw_version": "1.2",
					"sw_version": "5.4"
				}
			}`,
			createCtx: func() *valid.ValidationCtx {
				ctx := context.Background()
				return NewDiscoveryInfoViewValidationContext(ctx, btest.NopLogger(), model.TUYA, true)
			},
			expectValue: DeviceInfoView{
				ID:           "abc-123",
				Name:         "хаб",
				Description:  "умный хаб",
				Type:         model.HubDeviceType,
				Capabilities: []CapabilityInfoView{},
				Properties:   []PropertyInfoView{},
				Room:         "спальня",
				DeviceInfo: &model.DeviceInfo{
					Manufacturer: tools.AOS("Philips N.V."),
					Model:        tools.AOS("hue g11"),
					HwVersion:    tools.AOS("1.2"),
					SwVersion:    tools.AOS("5.4"),
				},
			},
			skillID: model.TUYA,
			getExpectDevice: func() model.Device {
				return model.Device{
					ID:           "",
					Name:         "хаб",
					Description:  tools.AOS("умный хаб"),
					ExternalID:   "abc-123",
					ExternalName: "хаб",
					SkillID:      model.TUYA,
					Type:         model.HubDeviceType,
					OriginalType: model.HubDeviceType,
					Room:         &model.Room{Name: "спальня"},
					Capabilities: model.Capabilities{},
					Properties:   model.Properties{},
					DeviceInfo: &model.DeviceInfo{
						Manufacturer: tools.AOS("Philips N.V."),
						Model:        tools.AOS("hue g11"),
						HwVersion:    tools.AOS("1.2"),
						SwVersion:    tools.AOS("5.4"),
					},
				}
			},
			expectErr: nil,
		},
		{
			name: "valid_polaris_kettle_with_bad_initial_state",
			input: `{
				"id": "abc-123",
				"name": "Чайник",
				"type": "devices.types.cooking.kettle",
				"capabilities": [
					{
						"type": "devices.capabilities.on_off",
						"retrievable": false
					},
					{
						"type": "devices.capabilities.mode",
						"retrievable": false,
						"parameters": {
							"instance": "tea_mode",
							"modes": [{"value": "green_tea"}, {"value": "black_tea"}]
						},
 						"state": {
							"instance": "tea_mode",
							"value": "gree_tea"
						}
					}
				]
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectValue: DeviceInfoView{
				ID:   "abc-123",
				Name: "Чайник",
				Type: model.KettleDeviceType,
				Capabilities: []CapabilityInfoView{
					{
						Type:        model.OnOffCapabilityType,
						Retrievable: false,
						Parameters:  model.OnOffCapabilityParameters{},
					},
					{
						Type:        model.ModeCapabilityType,
						Retrievable: false,
						Parameters: model.ModeCapabilityParameters{
							Instance: model.TeaModeInstance,
							Modes: []model.Mode{
								{Value: model.GreenTeaMode},
								{Value: model.BlackTeaMode},
							},
						},
						State: model.ModeCapabilityState{
							Instance: model.TeaModeInstance,
							Value:    "gree_tea", // this should turn into state:null
						},
					},
				},
			},
			skillID: "polaris",
			getExpectDevice: func() model.Device {
				onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
				onOff.SetParameters(model.OnOffCapabilityParameters{})

				teaMode := model.MakeCapabilityByType(model.ModeCapabilityType)
				teaMode.SetParameters(model.ModeCapabilityParameters{
					Instance: model.TeaModeInstance,
					Modes: []model.Mode{
						{Value: model.GreenTeaMode},
						{Value: model.BlackTeaMode},
					},
				})
				// important: tea mode has no state

				return model.Device{
					ID:           "",
					Name:         "Чайник",
					ExternalID:   "abc-123",
					ExternalName: "Чайник",
					SkillID:      "polaris",
					Type:         model.KettleDeviceType,
					OriginalType: model.KettleDeviceType,
					Capabilities: model.Capabilities{onOff, teaMode},
					Properties:   model.Properties{},
				}
			},
			expectErr: nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			var d DeviceInfoView

			err := binder.Bind(tc.createCtx(), []byte(tc.input), &d)

			if tc.expectErr == nil {
				assert.NoError(t, err)
				assert.Equal(t, tc.expectValue, d)
				assert.Equal(t, tc.getExpectDevice(), d.ToDevice(tc.skillID))
			} else {
				assert.EqualError(t, err, tc.expectErr.Error())
			}
		})
	}
}

func TestDiscoveryResult(t *testing.T) {
	testCases := []struct {
		name             string
		input            string
		createCtx        func() *valid.ValidationCtx
		expectValue      DiscoveryResult
		skillID          string
		getExpectDevices func() []model.Device
		expectErr        error
	}{
		// invalid
		{
			name: "invalid_discovery_result_with_duplicates",
			input: `{
				"request_id": "4F3BDFA0-239F-484F-B238-AD00495E4358",
				"payload": {
					"user_id": "r58j1SuAWkZ3cWRS422bUV5VS3bxv8Sbf3sN8dIb",
					"devices": [
						{
							"id": "00:17:88:01:03:03:cf:29-0b",
							"name": "misha",
							"description": "Hue color lamp",
							"room": "",
							"capabilities": [
								{
									"retrievable": true,
									"type": "devices.capabilities.on_off"
								}
							],
							"type": "devices.types.light",
							"device_info": {
								"manufacturer": "Philips",
								"model": "Hue color lamp",
								"hw_version": "LCT015",
								"sw_version": "1.46.13_r26312"
							}
						},
						{
							"id": "00:17:88:01:03:03:cf:29-0b",
							"name": "misha",
							"description": "Hue color lamp",
							"room": "",
							"capabilities": [
								{
									"retrievable": true,
									"type": "devices.capabilities.on_off"
								}
							],
							"type": "devices.types.light",
							"device_info": {
								"manufacturer": "Philips",
								"model": "Hue color lamp",
								"hw_version": "LCT015",
								"sw_version": "1.46.13_r26312"
							}
						}
					]
				}
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectErr: bulbasaur.Errors{xerrors.New("device ID is not unique in discovery 00:17:88:01:03:03:cf:29-0b")},
		},
		{
			name: "invalid_discovery_result_empty_capability_parameters",
			input: `{
				"request_id": "ff36a3cc-ec34-11e6-b1a0-64510650abcf",
				"payload": {
					"user_id": "user-001",
					"devices": [{
						"id": "lamp-001-xdl",
						"name": "лампочка",
						"description": "умная лампочка xdl",
						"room": "спальня",
						"type": "devices.types.light",
						"capabilities": [{
							"type": "devices.capabilities.range",
							"retrievable": true
						}]
					}]
				}
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectErr: bulbasaur.Errors{xerrors.New("cannot parse capability: parameters for range capability type is missing")},
		},
		// valid
		{
			name: "valid_discovery_result_with_split_capability_parameters",
			input: `{
				"request_id": "valid-req-id",
				"ts": 2020,
				"payload": {
					"user_id": "user-001",
					"devices": [{
						"id": "lamp-001",
						"name": "Лампочка",
						"description": "my lamp",
						"room": "спальня",
						"type": "devices.types.light",
						"capabilities": [{
							"type": "devices.capabilities.on_off",
							"retrievable": true,
							"parameters": {"split": true}
						}]
					}]
				}
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectValue: DiscoveryResult{
				RequestID: "valid-req-id",
				Timestamp: 2020,
				Payload: DiscoveryPayload{
					UserID: "user-001",
					Devices: []DeviceInfoView{
						{
							ID:          "lamp-001",
							Name:        "Лампочка",
							Description: "my lamp",
							Type:        model.LightDeviceType,
							Room:        "спальня",
							Capabilities: []CapabilityInfoView{
								{
									Retrievable: true,
									Type:        model.OnOffCapabilityType,
									Parameters: model.OnOffCapabilityParameters{
										Split: true,
									},
								},
							},
						},
					},
				},
			},
			skillID: model.TUYA,
			getExpectDevices: func() []model.Device {
				expectedCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				expectedCapability.SetRetrievable(true)
				expectedCapability.SetParameters(model.OnOffCapabilityParameters{Split: true})
				expectedDevices := []model.Device{
					{
						Name:         "Лампочка",
						Description:  tools.AOS("my lamp"),
						ExternalID:   "lamp-001",
						ExternalName: "Лампочка",
						SkillID:      model.TUYA,
						Type:         model.LightDeviceType,
						OriginalType: model.LightDeviceType,
						Room: &model.Room{
							Name: "спальня",
						},
						Capabilities: []model.ICapability{
							expectedCapability,
						},
						Properties: []model.IProperty{},
						Updated:    2020,
					},
				}
				return expectedDevices
			},
		},
		{
			name: "valid_tuya_discovery_result",
			input: `{
				"request_id": "valid-req-id",
				"ts": 2086,
				"payload": {
					"user_id": "man",
					"devices": [{
						"id": "test-device-01",
						"name": "Tuya lamp",
						"description": "my lamp",
						"room": "spalnya",
						"capabilities": [{
							"retrievable": true,
							"type": "devices.capabilities.range",
							"parameters": {
								"instance": "brightness",
								"unit": "unit.percent",
								"random_access": false,
								"looped": false,
								"range": {
									"min": 1,
									"max": 100,
									"precision": 10
								}
							}
						}],
						"properties": null,
						"type": "devices.types.light"
					}]
				}
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectValue: DiscoveryResult{
				RequestID: "valid-req-id",
				Timestamp: 2086,
				Payload: DiscoveryPayload{
					UserID: "man",
					Devices: []DeviceInfoView{
						{
							ID:          "test-device-01",
							Name:        "Tuya lamp",
							Description: "my lamp",
							Type:        model.LightDeviceType,
							Room:        "spalnya",
							Capabilities: []CapabilityInfoView{
								{
									Retrievable: true,
									Type:        model.RangeCapabilityType,
									Parameters: model.RangeCapabilityParameters{
										Instance: model.BrightnessRangeInstance,
										Unit:     model.UnitPercent,
										Range: &model.Range{
											Min:       1,
											Max:       100,
											Precision: 10,
										},
									},
								},
							},
						},
					},
				},
			},
			skillID: model.TUYA,
			getExpectDevices: func() []model.Device {
				expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
				expectedCapability.SetRetrievable(true)
				expectedCapability.SetParameters(model.RangeCapabilityParameters{
					Instance: model.BrightnessRangeInstance,
					Unit:     model.UnitPercent,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 10,
					},
				})
				expectedDevices := []model.Device{
					{
						Name:         "Tuya lamp",
						Description:  tools.AOS("my lamp"),
						ExternalID:   "test-device-01",
						ExternalName: "Tuya lamp",
						SkillID:      model.TUYA,
						Type:         model.LightDeviceType,
						OriginalType: model.LightDeviceType,
						Room: &model.Room{
							Name: "spalnya",
						},
						Capabilities: []model.ICapability{
							expectedCapability,
						},
						Properties: []model.IProperty{},
						Updated:    2086,
					},
				}
				return expectedDevices
			},
		},
		{
			name: "valid_philips_discovery_result",
			input: `{
				"request_id": "4F3BDFA0-239F-484F-B238-AD00495E4358",
				"payload": {
					"user_id": "r58j1SuAWkZ3cWRS422bUV5VS3bxv8Sbf3sN8dIb",
					"devices": [
						{
							"id": "00:17:88:01:03:03:cf:29-0b",
							"name": "misha",
							"description": "Hue color lamp",
							"room": "",
							"capabilities": [
								{
									"retrievable": true,
									"type": "devices.capabilities.on_off"
								},
								{
									"retrievable": true,
									"type": "devices.capabilities.color_setting",
									"parameters": {
										"color_model": "hsv",
										"temperature_k": {
											"min": 153,
											"max": 500
										}
									}
								}
							],
							"type": "devices.types.light",
							"device_info": {
								"manufacturer": "Philips",
								"model": "Hue color lamp",
								"hw_version": "LCT015",
								"sw_version": "1.46.13_r26312"
							}
						},
						{
							"id": "00:17:88:01:03:fa:1e:b7-0b",
							"name": "lamp 1",
							"description": "Hue color lamp",
							"room": "",
							"capabilities": [
								{
									"retrievable": true,
									"type": "devices.capabilities.on_off"
								},
								{
									"retrievable": true,
									"type": "devices.capabilities.color_setting",
									"parameters": {
										"color_model": "hsv",
										"temperature_k": {
											"min": 153,
											"max": 500
										}
									}
								}
							],
							"type": "devices.types.light",
							"device_info": {
								"manufacturer": "Philips",
								"model": "Hue color lamp",
								"hw_version": "LCT015",
								"sw_version": "1.46.13_r26312"
							}
						},
						{
							"id": "00:17:88:01:03:7a:d2:2d-0b",
							"name": "lamp 2",
							"description": "Hue color lamp",
							"room": "",
							"capabilities": [
								{
									"retrievable": true,
									"type": "devices.capabilities.on_off"
								},
								{
									"retrievable": true,
									"type": "devices.capabilities.color_setting",
									"parameters": {
										"color_model": "hsv",
										"temperature_k": {
											"min": 153,
											"max": 500
										}
									}
								}
							],
							"type": "devices.types.light",
							"device_info": {
								"manufacturer": "Philips",
								"model": "Hue color lamp",
								"hw_version": "LCT015",
								"sw_version": "1.46.13_r26312"
							}
						}
					]
				}
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectValue: DiscoveryResult{
				RequestID: "4F3BDFA0-239F-484F-B238-AD00495E4358",
				Timestamp: 0,
				Payload: DiscoveryPayload{
					UserID: "r58j1SuAWkZ3cWRS422bUV5VS3bxv8Sbf3sN8dIb",
					Devices: []DeviceInfoView{
						{
							ID:          "00:17:88:01:03:03:cf:29-0b",
							Name:        "misha",
							Description: "Hue color lamp",
							Room:        "",
							Capabilities: []CapabilityInfoView{
								{
									Retrievable: true,
									Type:        model.OnOffCapabilityType,
									Parameters:  model.OnOffCapabilityParameters{},
								},
								{
									Retrievable: true,
									Type:        model.ColorSettingCapabilityType,
									Parameters: model.ColorSettingCapabilityParameters{
										ColorModel: model.CM("hsv"),
										TemperatureK: &model.TemperatureKParameters{
											Min: 153,
											Max: 500,
										},
									},
								},
							},
							Type: model.LightDeviceType,
							DeviceInfo: &model.DeviceInfo{
								Manufacturer: tools.AOS("Philips"),
								Model:        tools.AOS("Hue color lamp"),
								HwVersion:    tools.AOS("LCT015"),
								SwVersion:    tools.AOS("1.46.13_r26312"),
							},
						},
						{
							ID:          "00:17:88:01:03:fa:1e:b7-0b",
							Name:        "lamp 1",
							Description: "Hue color lamp",
							Type:        model.LightDeviceType,
							Capabilities: []CapabilityInfoView{
								{
									Retrievable: true,
									Type:        model.OnOffCapabilityType,
									Parameters:  model.OnOffCapabilityParameters{},
								},
								{
									Retrievable: true,
									Type:        model.ColorSettingCapabilityType,
									Parameters: model.ColorSettingCapabilityParameters{
										ColorModel: model.CM("hsv"),
										TemperatureK: &model.TemperatureKParameters{
											Min: 153,
											Max: 500,
										},
									},
								},
							},
							DeviceInfo: &model.DeviceInfo{
								Manufacturer: tools.AOS("Philips"),
								Model:        tools.AOS("Hue color lamp"),
								HwVersion:    tools.AOS("LCT015"),
								SwVersion:    tools.AOS("1.46.13_r26312"),
							},
						},
						{
							ID:          "00:17:88:01:03:7a:d2:2d-0b",
							Name:        "lamp 2",
							Description: "Hue color lamp",
							Type:        model.LightDeviceType,
							Capabilities: []CapabilityInfoView{
								{
									Retrievable: true,
									Type:        model.OnOffCapabilityType,
									Parameters:  model.OnOffCapabilityParameters{},
								},
								{
									Retrievable: true,
									Type:        model.ColorSettingCapabilityType,
									Parameters: model.ColorSettingCapabilityParameters{
										ColorModel: model.CM("hsv"),
										TemperatureK: &model.TemperatureKParameters{
											Min: 153,
											Max: 500,
										},
									},
								},
							},
							DeviceInfo: &model.DeviceInfo{
								Manufacturer: tools.AOS("Philips"),
								Model:        tools.AOS("Hue color lamp"),
								HwVersion:    tools.AOS("LCT015"),
								SwVersion:    tools.AOS("1.46.13_r26312"),
							},
						},
					},
				},
			},
			skillID: model.PhilipsSkill,
			getExpectDevices: func() []model.Device {
				// Create expected devices
				// -- ColorSettingCapability
				tempParameters := model.TemperatureKParameters{
					Min: 153,
					Max: 500,
				}
				hsv := model.HsvModelType
				colorSettingCapabilityParams := model.ColorSettingCapabilityParameters{
					ColorModel:   &hsv,
					TemperatureK: &tempParameters,
				}
				colorCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
				colorCapability.SetRetrievable(true)
				colorCapability.SetParameters(colorSettingCapabilityParams)

				// -- onOffCapability
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				onOffCapability.SetRetrievable(true)
				// -- Devices
				lampTemplate := model.Device{
					Description:  tools.AOS("Hue color lamp"),
					Type:         model.LightDeviceType,
					OriginalType: model.LightDeviceType,
					DeviceInfo: &model.DeviceInfo{
						Manufacturer: tools.AOS("Philips"),
						Model:        tools.AOS("Hue color lamp"),
						HwVersion:    tools.AOS("LCT015"),
						SwVersion:    tools.AOS("1.46.13_r26312"),
					},
					Capabilities: []model.ICapability{
						onOffCapability,
						colorCapability,
					},
					Properties: []model.IProperty{},
					SkillID:    model.PhilipsSkill,
				}

				lamp1 := lampTemplate
				lamp1.Name = "misha"
				lamp1.ExternalName = "misha"
				lamp1.ExternalID = "00:17:88:01:03:03:cf:29-0b"

				lamp2 := lampTemplate
				lamp2.Name = "lamp 1"
				lamp2.ExternalName = "lamp 1"
				lamp2.ExternalID = "00:17:88:01:03:fa:1e:b7-0b"

				lamp3 := lampTemplate
				lamp3.Name = "lamp 2"
				lamp3.ExternalName = "lamp 2"
				lamp3.ExternalID = "00:17:88:01:03:7a:d2:2d-0b"

				// -- expected list:
				expectedDevices := []model.Device{lamp1, lamp2, lamp3}
				return expectedDevices
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			var discoveryResult DiscoveryResult

			err := binder.Bind(tc.createCtx(), []byte(tc.input), &discoveryResult)

			if tc.expectErr == nil {
				assert.NoError(t, err)
				assert.Equal(t, tc.expectValue, discoveryResult)
				assert.Equal(t, tc.getExpectDevices(), discoveryResult.ToDevices(tc.skillID))
			} else {
				assert.EqualError(t, err, tc.expectErr.Error())
			}
		})
	}
}

func TestTrustedDeviceInfo(t *testing.T) {
	t.Skip()
	deviceInfo := &model.DeviceInfo{
		Manufacturer: nil,
		Model:        nil,
		HwVersion:    nil,
		SwVersion:    nil,
	}
	vctx := NewDiscoveryInfoViewValidationContext(context.Background(), zaplogger.NewNop(), "_", true)
	if validFunc, ok := vctx.Get("trusted_device_info"); !ok {
		assert.EqualError(t, validFunc(reflect.ValueOf(deviceInfo), ""), "invalid device info: device does not supply manufacturer; device does not supply model")
	} else {
		t.Errorf("Validation context does not provider trusted_device_info validation tag")
	}
}
