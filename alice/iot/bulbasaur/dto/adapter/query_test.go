package adapter

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

func TestPropertyStateView(t *testing.T) {
	testCases := []struct {
		name        string
		input       string
		expectValue PropertyStateView
		expectErr   error
	}{
		//invalid
		{
			name: "invalid_property_type",
			input: `{
                "type": "quentin_quarantine"
			}`,
			expectErr: valid.Errors{
				xerrors.New("unknown property type: \"quentin_quarantine\""),
			},
		},
		{
			name: "invalid_property_instance",
			input: `{
                "type": "devices.properties.float",
				"state": {
					"instance": "helicopter"
				}
			}`,
			expectErr: valid.Errors{
				xerrors.New("unknown property instance: \"helicopter\""),
			},
		},
		{
			name: "invalid_property_amperage",
			input: `{
                "type": "devices.properties.float",
				"state": {
					"instance": "amperage",
					"value": "invalid-value-type"
				}
			}`,
			expectErr: valid.Errors{
				xerrors.New("json: unexpected type at 132: string"),
			},
		},
		//valid
		{
			name: "valid_property_amperage",
			input: `{
                "type": "devices.properties.float",
				"state": {
					"instance": "amperage",
					"value": 0.05
				}
			}`,
			expectValue: PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.AmperagePropertyInstance,
					Value:    0.05,
				},
			},
		},
		{
			name: "valid_property_timer",
			input: `{
                "type": "devices.properties.float",
				"state": {
					"instance": "timer",
					"value": 50
				}
			}`,
			expectValue: PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.TimerPropertyInstance,
					Value:    50,
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			var p PropertyStateView
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

func TestDeviceStateView(t *testing.T) {
	testCases := []struct {
		name      string
		input     string
		createCtx func() *valid.ValidationCtx

		expectValue     DeviceStateView
		getExpectDevice func() model.Device
		expectErr       error

		getValidationDevice func() model.Device
		expectValidationErr error
	}{
		// validation fail
		{
			name: "validation_fail_device_id_mismatch",
			input: `{
				"id": "unknown-id",
				"capabilities": [],
				"properties": []
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectValue: DeviceStateView{
				ID:           "unknown-id",
				Capabilities: []CapabilityStateView{},
				Properties:   []PropertyStateView{},
			},
			getValidationDevice: func() model.Device {
				return model.Device{
					ExternalID: "ext-id-1",
				}
			},
			expectValidationErr: xerrors.New("device id mismatch: expected 'ext-id-1', got 'unknown-id'"),
		},
		{
			name: "validation_fail_device_invalid_capability_value",
			input: `{
				"id": "ext-id-1",
				"capabilities": [
					{
						"type": "devices.capabilities.range",
						"state": {
							"instance": "brightness",
							"value": 0
						}
					}
				],
				"properties": []
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectValue: DeviceStateView{
				ID: "ext-id-1",
				Capabilities: []CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    0,
						},
					},
				},
				Properties: []PropertyStateView{},
			},
			getValidationDevice: func() model.Device {
				rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
				rangeCapability.SetParameters(
					model.RangeCapabilityParameters{
						Instance: model.BrightnessRangeInstance,
						Range: &model.Range{
							Min: 1,
							Max: 100,
						},
					},
				)
				return model.Device{
					ExternalID: "ext-id-1",
					Capabilities: []model.ICapability{
						rangeCapability,
					},
				}
			},
			expectValidationErr: bulbasaur.Errors{xerrors.New("range brightness instance state value is out of supported range: '0.000000'")},
		},
		// valid
		{
			name: "valid_device_unknown_capability",
			input: `{
				"id": "ext-id-1",
				"capabilities": [
					{
						"type": "devices.capabilities.range",
						"state": {
							"instance": "temperature",
							"value": 25
						}
					}
				],
				"properties": []
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectValue: DeviceStateView{
				ID: "ext-id-1",
				Capabilities: []CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.TemperatureRangeInstance,
							Value:    25,
						},
					},
				},
				Properties: []PropertyStateView{},
			},
			getValidationDevice: func() model.Device {
				return model.Device{
					ExternalID: "ext-id-1",
					Capabilities: []model.ICapability{
						model.MakeCapabilityByType(model.OnOffCapabilityType),
					},
				}
			},
		},
		{
			name: "valid_device_unknown_property",
			input: `{
				"id": "ext-id-1",
				"capabilities": [],
				"properties": [
					{
						"type": "devices.properties.float",
						"state": {
							"instance": "voltage",
							"value": 220
						}
					}
				]
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectValue: DeviceStateView{
				ID:           "ext-id-1",
				Capabilities: []CapabilityStateView{},
				Properties: []PropertyStateView{
					{
						Type: model.FloatPropertyType,
						State: model.FloatPropertyState{
							Instance: model.VoltagePropertyInstance,
							Value:    220,
						},
					},
				},
			},
			getValidationDevice: func() model.Device {
				return model.Device{
					ExternalID:   "ext-id-1",
					Capabilities: []model.ICapability{},
					Properties:   []model.IProperty{},
				}
			},
		},
		{
			name: "valid_light_bulb",
			input: `{
				"id": "ext-id-1",
				"capabilities": [
					{
						"type": "devices.capabilities.on_off",
						"state": {
							"instance": "on",
							"value": true
						}
					},
					{
						"type": "devices.capabilities.range",
						"state": {
							"instance": "brightness",
							"value": 55
						}
					},
					{
						"type": "devices.capabilities.color_setting",
						"state": {
							"instance": "temperature_k",
							"value": 3200
						}
					}
				],
				"properties": []
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectValue: DeviceStateView{
				ID: "ext-id-1",
				Capabilities: []CapabilityStateView{
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
							Value:    55.0,
						},
					},
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.TemperatureKCapabilityInstance,
							Value:    model.TemperatureK(3200),
						},
					},
				},
				Properties: []PropertyStateView{},
			},
			getExpectDevice: func() model.Device {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				onOffCapability.SetState(
					model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				)
				rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
				rangeCapability.SetState(
					model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    55.0,
					},
				)
				colorCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
				colorCapability.SetState(
					model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(3200),
					},
				)
				return model.Device{
					ExternalID: "ext-id-1",
					Capabilities: []model.ICapability{
						onOffCapability,
						rangeCapability,
						colorCapability,
					},
					Properties: model.Properties{},
				}
			},
			getValidationDevice: func() model.Device {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				onOffCapability.SetState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				})
				rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
				rangeCapability.SetParameters(model.RangeCapabilityParameters{
					Instance: model.BrightnessRangeInstance,
					Unit:     model.UnitPercent,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				})
				rangeCapability.SetState(model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    55.0,
				})
				colorCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
				colorCapability.SetParameters(model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 1000,
						Max: 10000,
					},
					ColorSceneParameters: &model.ColorSceneParameters{Scenes: model.ColorScenes{{ID: model.ColorSceneIDMovie}}},
				})
				colorCapability.SetState(model.ColorSettingCapabilityState{
					Instance: model.TemperatureKCapabilityInstance,
					Value:    model.TemperatureK(3200),
				})
				return model.Device{
					ExternalID: "ext-id-1",
					Capabilities: []model.ICapability{
						onOffCapability,
						rangeCapability,
						colorCapability,
					},
					Properties: model.Properties{},
				}
			},
		},
		{
			name: "valid_socket",
			input: `{
				"id": "ext-id-1",
				"capabilities": [
					{
						"type": "devices.capabilities.on_off",
						"state": {
							"instance": "on",
							"value": true
						}
					}
				],
				"properties": [
					{
                		"type": "devices.properties.float",
						"state": {
							"instance": "amperage",
							"value": 0.05
						}
					},
					{
                		"type": "devices.properties.float",
						"state": {
							"instance": "voltage",
							"value": 220
						}
					},
					{
                		"type": "devices.properties.float",
						"state": {
							"instance": "timer",
							"value": 20
						}
					}
				]
			}`,
			createCtx: func() *valid.ValidationCtx { return valid.NewValidationCtx() },
			expectValue: DeviceStateView{
				ID: "ext-id-1",
				Capabilities: []CapabilityStateView{
					{
						Type: model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{
							Instance: model.OnOnOffCapabilityInstance,
							Value:    true,
						},
					},
				},
				Properties: []PropertyStateView{
					{
						Type: model.FloatPropertyType,
						State: model.FloatPropertyState{
							Instance: model.AmperagePropertyInstance,
							Value:    0.05,
						},
					},
					{
						Type: model.FloatPropertyType,
						State: model.FloatPropertyState{
							Instance: model.VoltagePropertyInstance,
							Value:    220,
						},
					},
					{
						Type: model.FloatPropertyType,
						State: model.FloatPropertyState{
							Instance: model.TimerPropertyInstance,
							Value:    20,
						},
					},
				},
			},
			getExpectDevice: func() model.Device {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				onOffCapability.SetState(
					model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				)
				amperageProperty := model.MakePropertyByType(model.FloatPropertyType)
				amperageProperty.SetState(model.FloatPropertyState{
					Instance: model.AmperagePropertyInstance,
					Value:    0.05,
				})
				voltageProperty := model.MakePropertyByType(model.FloatPropertyType)
				voltageProperty.SetState(model.FloatPropertyState{
					Instance: model.VoltagePropertyInstance,
					Value:    220,
				})
				timerProperty := model.MakePropertyByType(model.FloatPropertyType)
				timerProperty.SetState(model.FloatPropertyState{
					Instance: model.TimerPropertyInstance,
					Value:    20,
				})
				return model.Device{
					ExternalID: "ext-id-1",
					Capabilities: []model.ICapability{
						onOffCapability,
					},
					Properties: model.Properties{
						amperageProperty,
						voltageProperty,
						timerProperty,
					},
				}
			},
			getValidationDevice: func() model.Device {
				onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
				onOffCapability.SetState(
					model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				)
				amperageProperty := model.MakePropertyByType(model.FloatPropertyType)
				amperageProperty.SetState(model.FloatPropertyState{
					Instance: model.AmperagePropertyInstance,
					Value:    0.05,
				})
				voltageProperty := model.MakePropertyByType(model.FloatPropertyType)
				voltageProperty.SetState(model.FloatPropertyState{
					Instance: model.VoltagePropertyInstance,
					Value:    220,
				})
				timerProperty := model.MakePropertyByType(model.FloatPropertyType)
				timerProperty.SetState(model.FloatPropertyState{
					Instance: model.TimerPropertyInstance,
					Value:    20,
				})
				return model.Device{
					ExternalID: "ext-id-1",
					Capabilities: []model.ICapability{
						onOffCapability,
					},
					Properties: model.Properties{
						amperageProperty,
						voltageProperty,
						timerProperty,
					},
				}
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			var d DeviceStateView

			err := binder.Bind(tc.createCtx(), []byte(tc.input), &d)

			if tc.expectErr == nil {
				require.NoError(t, err)
				assert.Equal(t, tc.expectValue, d)

				if tc.getExpectDevice != nil {
					assert.Equal(t, tc.getExpectDevice(), d.ToDevice())
				}

				if tc.getValidationDevice != nil {
					vErr := d.ValidateDSV(tc.getValidationDevice())
					if tc.expectValidationErr == nil {
						assert.NoError(t, vErr)
					} else {
						assert.EqualError(t, vErr, tc.expectValidationErr.Error())
					}
				}
			} else {
				assert.EqualError(t, err, tc.expectErr.Error())
			}
		})
	}
}

func TestStatesResult(t *testing.T) {
	testCases := []struct {
		name             string
		input            string
		expectValue      StatesResult
		getExpectDevices func() []model.Device
	}{
		{
			name: "valid_tuya_socket",
			input: `{
				"request_id": "40a84bee-0cbe-4bb3-b6bf-7fe667dbd1ed",
				"payload": {
					"devices": [{
						"id": "bfaec8bf857bd33f3csdm5",
						"capabilities": [{
							"type": "devices.capabilities.on_off",
							"state": {
								"instance": "on",
								"value": false
							}
						}],
						"properties": [{
                			"type": "devices.properties.float",
							"state": {
								"instance": "voltage",
								"value": 231.1
							}
						}, {
                			"type": "devices.properties.float",
							"state": {
								"instance": "power",
								"value": 0
							}
						}, {
                			"type": "devices.properties.float",
							"state": {
								"instance": "amperage",
								"value": 0
							}
						}, {
                			"type": "devices.properties.float",
							"state": {
								"instance": "timer",
								"value": 20
							}
						}]
					}]
				}
			}`,
			expectValue: StatesResult{
				RequestID: "40a84bee-0cbe-4bb3-b6bf-7fe667dbd1ed",
				Payload: StatesResultPayload{
					Devices: []DeviceStateView{
						{
							ID: "bfaec8bf857bd33f3csdm5",
							Capabilities: []CapabilityStateView{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
										Value:    false,
									},
								},
							},
							Properties: []PropertyStateView{
								{
									Type: model.FloatPropertyType,
									State: model.FloatPropertyState{
										Instance: model.VoltagePropertyInstance,
										Value:    231.1,
									},
								},
								{
									Type: model.FloatPropertyType,
									State: model.FloatPropertyState{
										Instance: model.PowerPropertyInstance,
										Value:    0,
									},
								},
								{
									Type: model.FloatPropertyType,
									State: model.FloatPropertyState{
										Instance: model.AmperagePropertyInstance,
										Value:    0,
									},
								},
								{
									Type: model.FloatPropertyType,
									State: model.FloatPropertyState{
										Instance: model.TimerPropertyInstance,
										Value:    20,
									},
								},
							},
						},
					},
				},
			},
			getExpectDevices: func() []model.Device {
				onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
				onOff.SetState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    false,
				})
				voltage := model.MakePropertyByType(model.FloatPropertyType)
				voltage.SetState(model.FloatPropertyState{
					Instance: model.VoltagePropertyInstance,
					Value:    231.1,
				})
				power := model.MakePropertyByType(model.FloatPropertyType)
				power.SetState(model.FloatPropertyState{
					Instance: model.PowerPropertyInstance,
					Value:    0,
				})
				amperage := model.MakePropertyByType(model.FloatPropertyType)
				amperage.SetState(model.FloatPropertyState{
					Instance: model.AmperagePropertyInstance,
					Value:    0,
				})
				timerProperty := model.MakePropertyByType(model.FloatPropertyType)
				timerProperty.SetState(model.FloatPropertyState{
					Instance: model.TimerPropertyInstance,
					Value:    20,
				})
				return []model.Device{
					{
						ExternalID:   "bfaec8bf857bd33f3csdm5",
						Capabilities: model.Capabilities{onOff},
						Properties:   model.Properties{voltage, power, amperage, timerProperty},
					},
				}
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			var sr StatesResult

			err := json.Unmarshal([]byte(tc.input), &sr)
			require.NoError(t, err)
			assert.Equal(t, tc.expectValue, sr)

			if tc.getExpectDevices != nil {
				assert.Equal(t, tc.getExpectDevices(), sr.ToDevicesStates())
			}
		})
	}
}
