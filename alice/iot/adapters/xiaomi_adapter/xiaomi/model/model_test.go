package model

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
)

func TestCustomData(t *testing.T) {
	rawDevice := []byte(`{"id": "trololo", "capabilities": [], "custom_data": {"region":"china", "type": "ololo", "cloudId":17}}`)

	var d adapter.DeviceActionRequestView
	err := json.Unmarshal(rawDevice, &d)
	assert.NoError(t, err)

	var xiaomiDevice Device
	xiaomiDevice.PopulateCustomData(d.CustomData)
	expectedCustomData := XiaomiCustomData{
		Type:    "ololo",
		Region:  Region(iotapi.ChinaRegion),
		IsSplit: false,
		CloudID: 17,
	}
	assert.Equal(t, expectedCustomData, xiaomiDevice.GetCustomData())
}

func TestActionVacuumCleanerModeMapping(t *testing.T) {
	testCases := []struct {
		name           string
		value          model.ModeValue
		propertyValues []miotspec.Value
		expectedValue  float64
		expectedPID    string
	}{
		{
			name:  "100e values",
			value: model.FastMode,
			propertyValues: []miotspec.Value{
				{Value: 101, Description: "Silent"},
				{Value: 102, Description: "Basic"},
				{Value: 103, Description: "Strong"},
				{Value: 104, Description: "Full Speed"},
			},
			expectedValue: 103,
			expectedPID:   "DD.2.1",
		},
		{
			name:  "regular values",
			value: model.QuietMode,
			propertyValues: []miotspec.Value{
				{Value: 1, Description: "Silent"},
				{Value: 2, Description: "Basic"},
				{Value: 3, Description: "Strong"},
				{Value: 4, Description: "Full Speed"},
			},
			expectedValue: 1,
			expectedPID:   "DD.2.1",
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			device := Device{
				DID: "DD",
				Services: []Service{
					{
						Iid: 2,
						Properties: []Property{
							{
								Iid:       1,
								Type:      "urn:miot-spec-v2:property:mode:00000008:roborock-a08:1",
								Format:    "uint8",
								Access:    []string{"read", "notify", "write"},
								ValueList: tc.propertyValues,
							},
						},
					},
				},
			}

			bulbasaurState := adapter.DeviceActionRequestView{
				Capabilities: []adapter.CapabilityActionView{
					{
						Type: model.ModeCapabilityType,
						State: model.ModeCapabilityState{
							Instance: model.WorkSpeedModeInstance,
							Value:    tc.value,
						},
					},
				},
			}

			expected := []PropertyState{
				{
					Type:     model.ModeCapabilityType,
					Instance: string(model.WorkSpeedModeInstance),
					Property: iotapi.Property{
						Pid:   tc.expectedPID,
						Value: tc.expectedValue,
					},
				},
			}
			assert.Equal(t, expected, device.GetPropertyStates(bulbasaurState))
		})
	}
}

func TestQueryVacuumCleanerModeMappingForModeProperty(t *testing.T) {
	testCases := []struct {
		name           string
		value          float64
		propertyValues []miotspec.Value
		expectedMode   model.ModeValue
	}{
		{
			name:  "regular models quiet",
			value: 1,
			propertyValues: []miotspec.Value{
				{Value: 1, Description: "Silent"},
				{Value: 2, Description: "Basic"},
				{Value: 3, Description: "Strong"},
				{Value: 4, Description: "Full Speed"},
			},
			expectedMode: model.QuietMode,
		},
		{
			name:  "regular models turbo",
			value: 4,
			propertyValues: []miotspec.Value{
				{Value: 1, Description: "Silent"},
				{Value: 2, Description: "Basic"},
				{Value: 3, Description: "Strong"},
				{Value: 4, Description: "Full Speed"},
			},
			expectedMode: model.TurboMode,
		},
		{
			name:  "100 models turbo",
			value: 104,
			propertyValues: []miotspec.Value{
				{Value: 101, Description: "Silent"},
				{Value: 102, Description: "Basic"},
				{Value: 103, Description: "Strong"},
				{Value: 104, Description: "Full Speed"},
			},
			expectedMode: model.TurboMode,
		},
		{
			name:  "100 models quiet",
			value: 101,
			propertyValues: []miotspec.Value{
				{Value: 101, Description: "Silent"},
				{Value: 102, Description: "Basic"},
				{Value: 103, Description: "Strong"},
				{Value: 104, Description: "Full Speed"},
			},
			expectedMode: model.QuietMode,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			device := Device{
				DID: "DD",
				Services: []Service{
					{
						Iid:  2,
						Type: "urn:miot-spec-v2:service:vacuum:00007810:rockrobo-v1:1",
						Properties: []Property{
							{
								Iid:       1,
								Type:      "urn:miot-spec-v2:property:mode:00000008:roborock-a08:1",
								Format:    "uint8",
								Access:    []string{"read", "notify", "write"},
								ValueList: tc.propertyValues,
							},
						},
					},
				},
				PropertyStates: []PropertyState{
					{
						Property: iotapi.Property{
							Pid:         "DD.2.1",
							Status:      0,
							Description: "",
							Value:       tc.value,
							IsSplit:     false,
						},
					},
				},
			}

			expected := []adapter.CapabilityStateView{
				{
					Type: model.ModeCapabilityType,
					State: model.ModeCapabilityState{
						Instance: model.WorkSpeedModeInstance,
						Value:    tc.expectedMode,
					},
				},
			}
			assert.Equal(t, expected, device.GetVacuumCapabilitiesStates())
		})
	}
}

func TestDiscoveryBattery(t *testing.T) {
	testCases := []struct {
		name               string
		valueList          []miotspec.Value
		valueRange         []float64
		unit               string
		expectedProperties []adapter.PropertyInfoView
	}{
		{
			name:       "percentage property",
			valueList:  nil,
			valueRange: []float64{1, 100, 1},
			unit:       percentUnit,
			expectedProperties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.BatteryLevelPropertyInstance,
						Unit:     model.UnitPercent,
					},
				},
			},
		},
		{
			name: "event property",
			valueList: []miotspec.Value{
				{Value: 1, Description: "Normal"},
				{Value: 2, Description: "Low"},
			},
			unit: noneUnit,
			expectedProperties: []adapter.PropertyInfoView{
				{
					Type:        model.EventPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters: model.EventPropertyParameters{
						Instance: model.BatteryLevelPropertyInstance,
						Events: model.Events{
							{Value: model.NormalEvent, Name: tools.AOS("обычный уровень")},
							{Value: model.LowEvent, Name: tools.AOS("низкий уровень")},
						},
					},
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			device := Device{
				DID: "DD",
				Services: []Service{
					{
						Iid:  3,
						Type: "urn:miot-spec-v2:service:battery:00007805:rockrobo-v1:1",
						Properties: []Property{
							{
								Iid:        1,
								Type:       "urn:miot-spec-v2:property:battery-level:00000014:rockrobo-v1:1",
								Format:     "uint8",
								Access:     []string{"read", "notify", "write"},
								ValueList:  tc.valueList,
								ValueRange: tc.valueRange,
								Unit:       tc.unit,
							},
						},
					},
				},
			}

			_, _, definition, err := device.ToDeviceInfoViewWithSubscriptions()
			assert.NoError(t, err)
			assert.Equal(t, tc.expectedProperties, definition.Properties)
		})
	}
}

func TestQueryBatteryValue(t *testing.T) {
	testCases := []struct {
		name               string
		valueList          []miotspec.Value
		valueRange         []float64
		sourceValue        float64
		unit               string
		expectedProperties []adapter.PropertyStateView
	}{
		{
			name:        "percentage property",
			valueList:   nil,
			valueRange:  []float64{1, 100, 1},
			unit:        percentUnit,
			sourceValue: 23,
			expectedProperties: []adapter.PropertyStateView{
				{
					Type: model.FloatPropertyType,
					State: model.FloatPropertyState{
						Instance: model.BatteryLevelPropertyInstance,
						Value:    23,
					},
				},
			},
		},
		{
			name: "event property normal",
			valueList: []miotspec.Value{
				{Value: 1, Description: "Normal"},
				{Value: 2, Description: "Low"},
			},
			unit:        noneUnit,
			sourceValue: 1,
			expectedProperties: []adapter.PropertyStateView{
				{
					Type: model.EventPropertyType,
					State: model.EventPropertyState{
						Instance: model.BatteryLevelPropertyInstance,
						Value:    model.NormalEvent,
					},
				},
			},
		},
		{
			name: "event property low",
			valueList: []miotspec.Value{
				{Value: 1, Description: "Normal"},
				{Value: 2, Description: "Low"},
			},
			unit:        noneUnit,
			sourceValue: 2,
			expectedProperties: []adapter.PropertyStateView{
				{
					Type: model.EventPropertyType,
					State: model.EventPropertyState{
						Instance: model.BatteryLevelPropertyInstance,
						Value:    model.LowEvent,
					},
				},
			},
		},
		{
			name: "event property unknown",
			valueList: []miotspec.Value{
				{Value: 1, Description: "Normal"},
				{Value: 2, Description: "Low"},
			},
			unit:               noneUnit,
			sourceValue:        10,
			expectedProperties: []adapter.PropertyStateView{},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			device := Device{
				DID: "DD",
				Services: []Service{
					{
						Iid:  3,
						Type: "urn:miot-spec-v2:service:battery:00007805:rockrobo-v1:1",
						Properties: []Property{
							{
								Iid:        1,
								Type:       "urn:miot-spec-v2:property:battery-level:00000014:rockrobo-v1:1",
								Format:     "uint8",
								Access:     []string{"read", "notify", "write"},
								ValueList:  tc.valueList,
								ValueRange: tc.valueRange,
								Unit:       tc.unit,
							},
						},
					},
				},
				PropertyStates: []PropertyState{
					{
						Property: iotapi.Property{
							Pid:         "DD.3.1",
							Status:      0,
							Description: "",
							Value:       tc.sourceValue,
							IsSplit:     false,
						},
					},
				},
			}
			assert.Equal(t, tc.expectedProperties, device.ToPropertyStateViews(false))
		})
	}
}

func TestQueryMagnetContactState(t *testing.T) {
	testCases := []struct {
		name               string
		deviceType         string
		sourceValue        bool
		fromCallback       bool
		expectedProperties []adapter.PropertyStateView
	}{
		{
			name:         "event property closed",
			deviceType:   "urn:miot-spec-v2:device:magnet-sensor:0000A016:lumi-v2:1",
			sourceValue:  true,
			fromCallback: false,
			expectedProperties: []adapter.PropertyStateView{
				{
					Type: model.EventPropertyType,
					State: model.EventPropertyState{
						Instance: model.OpenPropertyInstance,
						Value:    model.ClosedEvent,
					},
				},
			},
		},
		{
			name:         "event property open",
			deviceType:   "urn:miot-spec-v2:device:magnet-sensor:0000A016:lumi-v2:1",
			sourceValue:  false,
			fromCallback: false,
			expectedProperties: []adapter.PropertyStateView{
				{
					Type: model.EventPropertyType,
					State: model.EventPropertyState{
						Instance: model.OpenPropertyInstance,
						Value:    model.OpenedEvent,
					},
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			device := Device{
				DID:  "DD",
				Type: tc.deviceType,
				Services: []Service{
					{
						Iid:  3,
						Type: "urn:miot-spec-v2:service:magnet-sensor:00007827:lumi-v2:1",
						Properties: []Property{
							{
								Iid:    1,
								Type:   "urn:miot-spec-v2:property:contact-state:0000007C:lumi-v2:1",
								Access: []string{"read", "notify"},
							},
						},
					},
				},
				PropertyStates: []PropertyState{
					{
						Property: iotapi.Property{
							Pid:         "DD.3.1",
							Status:      0,
							Description: "",
							Value:       tc.sourceValue,
							IsSplit:     false,
						},
					},
				},
			}
			assert.Equal(t, tc.expectedProperties, device.ToPropertyStateViews(tc.fromCallback))
		})
	}
}

func TestDiscoveryPMDensity(t *testing.T) {
	testCases := []struct {
		name               string
		propertyType       string
		expectedProperties []adapter.PropertyInfoView
	}{
		{
			name:         "pm1 density",
			propertyType: "urn:miot-spec-v2:property:pm1-density:00000034:zhimi-ma2:1",
			expectedProperties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.PM1DensityPropertyInstance,
						Unit:     model.UnitDensityMcgM3,
					},
				},
			},
		},
		{
			name:         "pm2.5 density",
			propertyType: "urn:miot-spec-v2:property:pm2.5-density:00000034:zhimi-ma2:1",
			expectedProperties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.PM2p5DensityPropertyInstance,
						Unit:     model.UnitDensityMcgM3,
					},
				},
			},
		},
		{
			name:         "pm10 density",
			propertyType: "urn:miot-spec-v2:property:pm10-density:00000034:zhimi-ma2:1",
			expectedProperties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.PM10DensityPropertyInstance,
						Unit:     model.UnitDensityMcgM3,
					},
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			device := Device{
				DID: "DD",
				Services: []Service{
					{
						Iid:  3,
						Type: "urn:miot-spec-v2:service:environment:0000780A:zhimi-ma2:1",
						Properties: []Property{
							{
								Iid:        1,
								Type:       tc.propertyType,
								Format:     "float64",
								Access:     []string{"read", "notify"},
								ValueRange: []float64{0, 600, 1},
							},
						},
					},
				},
			}

			_, _, definition, err := device.ToDeviceInfoViewWithSubscriptions()
			assert.NoError(t, err)
			assert.Equal(t, tc.expectedProperties, definition.Properties)
		})
	}
}

func TestQueryPMDensity(t *testing.T) {
	testCases := []struct {
		name               string
		propertyType       string
		sourceValue        float64
		expectedProperties []adapter.PropertyStateView
	}{
		{
			name:         "pm1 density",
			propertyType: "urn:miot-spec-v2:property:pm1-density:00000034:zhimi-ma2:1",
			sourceValue:  20,
			expectedProperties: []adapter.PropertyStateView{
				{
					Type: model.FloatPropertyType,
					State: model.FloatPropertyState{
						Instance: model.PM1DensityPropertyInstance,
						Value:    20,
					},
				},
			},
		},
		{
			name:         "pm2.5 density",
			propertyType: "urn:miot-spec-v2:property:pm2.5-density:00000034:zhimi-ma2:1",
			sourceValue:  234,
			expectedProperties: []adapter.PropertyStateView{
				{
					Type: model.FloatPropertyType,
					State: model.FloatPropertyState{
						Instance: model.PM2p5DensityPropertyInstance,
						Value:    234,
					},
				},
			},
		},
		{
			name:         "pm10 density",
			propertyType: "urn:miot-spec-v2:property:pm10-density:00000034:zhimi-ma2:1",
			sourceValue:  600,
			expectedProperties: []adapter.PropertyStateView{
				{
					Type: model.FloatPropertyType,
					State: model.FloatPropertyState{
						Instance: model.PM10DensityPropertyInstance,
						Value:    600,
					},
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			device := Device{
				DID: "DD",
				Services: []Service{
					{
						Iid:  3,
						Type: "urn:miot-spec-v2:service:environment:0000780A:zhimi-ma2:1",
						Properties: []Property{
							{
								Iid:    1,
								Type:   tc.propertyType,
								Access: []string{"read", "notify"},
							},
						},
					},
				},
				PropertyStates: []PropertyState{
					{
						Property: iotapi.Property{
							Pid:         "DD.3.1",
							Status:      0,
							Description: "",
							Value:       tc.sourceValue,
							IsSplit:     false,
						},
					},
				},
			}
			assert.Equal(t, tc.expectedProperties, device.ToPropertyStateViews(false))
		})
	}
}

func TestQuerySubmersionSensor(t *testing.T) {
	testCases := []struct {
		name               string
		props              []Property
		propStates         []PropertyState
		expectedProperties []adapter.PropertyStateView
	}{
		{
			name: "submersion sensor dry property",
			props: []Property{
				{
					Iid:    1,
					Type:   "urn:miot-spec-v2:property:submersion-state:0000007E:lumi-bmcn01:1",
					Access: []string{"read"},
				},
			},
			propStates: []PropertyState{
				{
					Property: iotapi.Property{
						Pid:   "DD.2.1",
						Value: false,
					},
				},
			},
			expectedProperties: []adapter.PropertyStateView{
				{
					Type: model.EventPropertyType,
					State: model.EventPropertyState{
						Instance: model.WaterLeakPropertyInstance,
						Value:    model.DryEvent,
					},
				},
			},
		},
		{
			name: "submersion-sensor prop leak",
			props: []Property{
				{
					Iid:    1,
					Type:   "urn:miot-spec-v2:property:submersion-state:0000007E:lumi-bmcn01:1",
					Access: []string{"read"},
				},
			},
			propStates: []PropertyState{
				{
					Property: iotapi.Property{
						Pid:   "DD.2.1",
						Value: true,
					},
				},
			},
			expectedProperties: []adapter.PropertyStateView{
				{
					Type: model.EventPropertyType,
					State: model.EventPropertyState{
						Instance: model.WaterLeakPropertyInstance,
						Value:    model.LeakEvent,
					},
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			device := Device{
				DID: "DD",
				Services: []Service{
					{
						Iid:        2,
						Type:       "urn:miot-spec-v2:service:submersion-sensor:00007839:lumi-bmcn01:1",
						Properties: tc.props,
					},
				},
				PropertyStates: tc.propStates,
			}
			assert.Equal(t, tc.expectedProperties, device.ToPropertyStateViews(false))
		})
	}
}

func TestDiscoveryIllumination(t *testing.T) {
	testCases := []struct {
		name               string
		serviceName        string
		valueList          []miotspec.Value
		valueRange         []float64
		unit               string
		expectedProperties []adapter.PropertyInfoView
	}{
		{
			name:        "percentage property in motion service",
			serviceName: "urn:miot-spec-v2:service:motion-sensor:00007825:",
			valueList:   nil,
			valueRange:  []float64{1, 100, 1},
			unit:        "lux",
			expectedProperties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.IlluminationPropertyInstance,
						Unit:     model.UnitIlluminationLux,
					},
				},
			},
		},
		{
			name:        "percentage property in illumination service",
			serviceName: "urn:miot-spec-v2:service:illumination-sensor:",
			valueList:   nil,
			valueRange:  []float64{1, 100, 1},
			unit:        "lux",
			expectedProperties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.IlluminationPropertyInstance,
						Unit:     model.UnitIlluminationLux,
					},
				},
			},
		},
		{
			name:        "event property in motion service",
			serviceName: "urn:miot-spec-v2:service:motion-sensor:00007825:",
			valueList: []miotspec.Value{
				{Value: 1, Description: "Weak"},
				{Value: 2, Description: "Strong"},
			},
			unit:               noneUnit,
			expectedProperties: []adapter.PropertyInfoView{},
		},
		{
			name:        "event property in motion service",
			serviceName: "urn:miot-spec-v2:service:illumination-sensor:",
			valueList: []miotspec.Value{
				{Value: 1, Description: "Weak"},
				{Value: 2, Description: "Strong"},
			},
			unit:               noneUnit,
			expectedProperties: []adapter.PropertyInfoView{},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			device := Device{
				DID: "DD",
				Services: []Service{
					{
						Iid:  3,
						Type: tc.serviceName,
						Properties: []Property{
							{
								Iid:        1,
								Type:       "urn:miot-spec-v2:property:illumination:0000004E:",
								Format:     "float",
								Access:     []string{"read", "notify", "write"},
								ValueList:  tc.valueList,
								ValueRange: tc.valueRange,
								Unit:       tc.unit,
							},
						},
					},
				},
			}

			_, _, definition, err := device.ToDeviceInfoViewWithSubscriptions()
			assert.NoError(t, err)
			assert.Equal(t, tc.expectedProperties, definition.Properties)
		})
	}
}

func TestDiscoveryHeaterCapabilities(t *testing.T) {
	device := Device{
		DID: "DD",
		Services: []Service{
			{
				Iid:  2,
				Type: "urn:miot-spec-v2:service:heater:0000782E:zhimi-mc2:1",
				Properties: []Property{
					{
						Iid:        1,
						Type:       "urn:miot-spec-v2:property:on:00000006:zhimi-mc2:1",
						Format:     "bool",
						Access:     []string{"read", "notify", "write"},
						ValueList:  nil,
						ValueRange: nil,
					},
					{
						Iid:        5,
						Type:       "urn:miot-spec-v2:property:target-temperature:00000021:zhimi-mc2:1",
						Format:     "float",
						Access:     []string{"read", "notify", "write"},
						ValueList:  nil,
						ValueRange: []float64{18, 28, 1},
						Unit:       "celsius",
					},
					{
						Iid:    2,
						Type:   "urn:miot-spec-v2:property:mode:00000021:zhimi-mc2:1",
						Format: "float",
						Access: []string{"read", "notify", "write"},
						ValueList: []miotspec.Value{
							{
								Value:       0,
								Description: "constant temperature",
							},
							{
								Value:       1,
								Description: "heat",
							},
							{
								Value:       2,
								Description: "warm",
							},
							{
								Value:       3,
								Description: "natural wind",
							},
						},
						ValueRange: nil,
					},
				},
			},
		},
	}

	_, _, definition, err := device.ToDeviceInfoViewWithSubscriptions()
	assert.NoError(t, err)

	expectedCapabilities := []adapter.CapabilityInfoView{
		{
			Type:        model.OnOffCapabilityType,
			Reportable:  true,
			Retrievable: true,
			Parameters:  model.OnOffCapabilityParameters{},
			State:       nil,
		},
		{
			Type:        model.RangeCapabilityType,
			Reportable:  true,
			Retrievable: true,
			Parameters: model.RangeCapabilityParameters{
				Instance:     model.TemperatureRangeInstance,
				Unit:         model.UnitTemperatureCelsius,
				RandomAccess: true,
				Range: &model.Range{
					Min:       18,
					Max:       28,
					Precision: 1,
				},
			},
			State: nil,
		},
		{
			Type:        model.ModeCapabilityType,
			Reportable:  true,
			Retrievable: true,
			Parameters: model.ModeCapabilityParameters{
				Instance: model.HeatModeInstance,
				Modes: []model.Mode{
					model.KnownModes[model.AutoMode],
					model.KnownModes[model.MaxMode],
					model.KnownModes[model.NormalMode],
					model.KnownModes[model.MinMode],
				},
			},
			State: nil,
		},
	}
	assert.Equal(t, expectedCapabilities, definition.Capabilities)
}

func TestQueryHeaterCapabilitiesState(t *testing.T) {
	device := Device{
		DID: "DD",
		Services: []Service{
			{
				Iid:  2,
				Type: "urn:miot-spec-v2:service:heater:0000782E:zhimi-mc2:1",
				Properties: []Property{
					{
						Iid:    1,
						Type:   "urn:miot-spec-v2:property:on:00000006:zhimi-mc2:1",
						Access: []string{"read", "write", "notify"},
					},
					{
						Iid:    5,
						Type:   "urn:miot-spec-v2:property:target-temperature:00000021:zhimi-mc2:1",
						Access: []string{"read", "write", "notify"},
						Unit:   "celsius",
					},
					{
						Iid:    2,
						Type:   "urn:miot-spec-v2:property:mode:00000021:zhimi-mc2:1",
						Format: "float",
						Access: []string{"read", "notify", "write"},
						ValueList: []miotspec.Value{
							{
								Value:       0,
								Description: "constant temperature",
							},
							{
								Value:       1,
								Description: "heat",
							},
							{
								Value:       2,
								Description: "warm",
							},
							{
								Value:       3,
								Description: "natural wind",
							},
						},
						ValueRange: nil,
					},
				},
			},
		},
		PropertyStates: []PropertyState{
			{
				Property: iotapi.Property{
					Pid:   "DD.2.1",
					Value: true,
				},
			},
			{
				Property: iotapi.Property{
					Pid:   "DD.2.5",
					Value: float64(21),
				},
			},
			{
				Property: iotapi.Property{
					Pid:   "DD.2.2",
					Value: float64(3),
				},
			},
		},
	}

	expectedCapabilities := []adapter.CapabilityStateView{
		{
			Type: model.OnOffCapabilityType,
			State: model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    true,
				Relative: nil,
			},
		},
		{
			Type: model.ModeCapabilityType,
			State: model.ModeCapabilityState{
				Instance: model.HeatModeInstance,
				Value:    model.MinMode,
			},
		},
		{
			Type: model.RangeCapabilityType,
			State: model.RangeCapabilityState{
				Instance: model.TemperatureRangeInstance,
				Value:    21,
			},
		},
	}

	assert.Equal(t, expectedCapabilities, device.ToCapabilityStateViews())
}
