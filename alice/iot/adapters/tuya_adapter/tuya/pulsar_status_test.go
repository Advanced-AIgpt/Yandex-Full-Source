package tuya

import (
	"encoding/json"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestPulsarStatuses_ToCapabilityStateView(t *testing.T) {
	check := func(oldStatus map[TuyaCommandName]TuyaCommand, rawJSON string, expectedCapabilities []adapter.CapabilityStateView) func(*testing.T) {
		return func(t *testing.T) {
			var statuses PulsarStatuses
			err := json.Unmarshal([]byte(rawJSON), &statuses)
			if assert.NoError(t, err) {
				deviceStatus := make([]TuyaCommand, 0, len(oldStatus))
				for _, command := range oldStatus {
					deviceStatus = append(deviceStatus, command)
				}
				device := UserDevice{Status: deviceStatus, Category: string(TuyaLightDeviceType), ProductID: LampYandexProductID}

				actualCapabilities, _ := statuses.ToCapabilityStateView(func() (UserDevice, error) {
					return device, nil
				})
				assert.ElementsMatch(t, expectedCapabilities, actualCapabilities)
			}
		}
	}

	t.Run("empty", check(nil, "[]", []adapter.CapabilityStateView{}))

	t.Run("timestamp_overrides", check(
		nil,
		`[
			{
				"3": "0",
				"code": "bright_value",
				"t": 1,
				"value": 0
			},
			{
				"3": "255",
				"code": "bright_value",
				"t": 2,
				"value": 255
			},
			{
				"3": "128",
				"code": "bright_value",
				"t": 1,
				"value": 128
			}
		]`,
		[]adapter.CapabilityStateView{
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    100.0,
				},
			},
		},
	))

	t.Run("RGB+CCT Smart WiFi bulb", func(t *testing.T) {
		// Yandex.Lamp v1
		// onOff + color(hsv, temp_k[2700..6500]) + brightness

		t.Run("work_mode", func(t *testing.T) {
			t.Run("colour", func(t *testing.T) {
				t.Run("extra_data", check(
					nil,
					`[
						{
							"code": "bright_value",
							"t": 1574778312305,
							"3": "128",
							"value": 128
						},
						{
							"code": "temp_value",
							"t": 1574778312305,
							"4": "255",
							"value": 255
						},
						{
							"2": "colour",
							"code": "work_mode",
							"t": 1574778312305,
							"value": "colour"
						},
						{
							"code": "colour_data",
							"t": 1574778312305,
							"5": "102700005fff27",
							"value": "{\"h\":95.0,\"s\":255.0,\"v\":38.0}"
						}
					]`,
					[]adapter.CapabilityStateView{
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.HsvColorCapabilityInstance,
								Value:    model.HSV{H: 135, S: 96, V: 15},
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    15.0,
							},
						},
					},
				))

				t.Run("with_data", check(
					nil,
					`[
						{
							"2": "colour",
							"code": "work_mode",
							"t": 1574778312305,
							"value": "colour"
						},
						{
							"code": "colour_data",
							"t": 1574778312305,
							"5": "102700005fff27",
							"value": "{\"h\":95.0,\"s\":255.0,\"v\":39.0}"
						}
					]`,
					[]adapter.CapabilityStateView{
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.HsvColorCapabilityInstance,
								Value:    model.HSV{H: 135, S: 96, V: 15},
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    15.0,
							},
						},
					},
				))

				t.Run("old_data", check(
					map[TuyaCommandName]TuyaCommand{
						ColorDataCommand: {
							Code:  string(ColorDataCommand),
							Value: model.HSV{H: 128, S: 50, V: 75},
						},
					},
					`[
						{
							"2": "colour",
							"code": "work_mode",
							"t": 1574778312305,
							"value": "colour"
						}
					]`,
					[]adapter.CapabilityStateView{
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.HsvColorCapabilityInstance,
								Value:    model.HSV{H: 128, S: 19, V: 29},
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    29.0,
							},
						},
					},
				))

				t.Run("no_data", check(
					nil,
					`[
						{
							"2": "colour",
							"code": "work_mode",
							"t": 1574778312305,
							"value": "colour"
						}
					]`,
					[]adapter.CapabilityStateView{},
				))
			})

			t.Run("white", func(t *testing.T) {
				t.Run("extra_data", check(
					nil,
					`[
						{
							"code": "bright_value",
							"t": 1574778312305,
							"3": "27",
							"value": 27
						},
						{
							"code": "temp_value",
							"t": 1574778312305,
							"4": "0",
							"value": 0
						},
						{
							"2": "white",
							"code": "work_mode",
							"t": 1574778312305,
							"value": "white"
						},
						{
							"code": "colour_data",
							"t": 1574778312305,
							"5": "102700005fff27",
							"value": "{\"h\":95.0,\"s\":255.0,\"v\":39.0}"
						}
					]`,
					[]adapter.CapabilityStateView{
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.TemperatureKCapabilityInstance,
								Value:    model.TemperatureK(2700),
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    1.0,
							},
						},
					},
				))

				t.Run("all_data", check(
					nil,
					`[
						{
							"2": "white",
							"code": "work_mode",
							"t": 1574779550133,
							"value": "white"
						},
						{
							"3": "27",
							"code": "bright_value",
							"t": 1574779685842,
							"value": 27
						},
						{
							"code": "temp_value",
							"4": "0",
							"t": 1574779550133,
							"value": 0
						}
					]`,
					[]adapter.CapabilityStateView{
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.TemperatureKCapabilityInstance,
								Value:    model.TemperatureK(2700),
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    1.0,
							},
						},
					},
				))

				t.Run("bright+old_temp", check(
					map[TuyaCommandName]TuyaCommand{
						TempValueCommand: {
							Code:  string(TempValueCommand),
							Value: 60,
						},
					},
					`[
						{
							"2": "white",
							"code": "work_mode",
							"t": 1574779550133,
							"value": "white"
						},
						{
							"3": "128",
							"code": "bright_value",
							"t": 1574779685842,
							"value": 128
						}
					]`,
					[]adapter.CapabilityStateView{
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.TemperatureKCapabilityInstance,
								Value:    model.TemperatureK(3400),
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    58.0,
							},
						},
					},
				))

				t.Run("bright", check(
					nil,
					`[
						{
							"2": "white",
							"code": "work_mode",
							"t": 1574779550133,
							"value": "white"
						},
						{
							"3": "128",
							"code": "bright_value",
							"t": 1574779685842,
							"value": 128
						}
					]`,
					[]adapter.CapabilityStateView{
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    58.0,
							},
						},
					},
				))

				t.Run("old_bright+temp", check(
					map[TuyaCommandName]TuyaCommand{
						BrightnessCommand: {
							Code:  string(BrightnessCommand),
							Value: 60,
						},
					},
					`[
						{
							"2": "white",
							"code": "work_mode",
							"t": 1574779550133,
							"value": "white"
						},
						{
							"3": "120",
							"code": "temp_value",
							"t": 1574779685842,
							"value": 120
						}
					]`,
					[]adapter.CapabilityStateView{
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.TemperatureKCapabilityInstance,
								Value:    model.TemperatureK(4500),
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    20.0,
							},
						},
					},
				))

				t.Run("temp", check(
					nil,
					`[
						{
							"2": "white",
							"code": "work_mode",
							"t": 1574779550133,
							"value": "white"
						},
						{
							"3": "120",
							"code": "temp_value",
							"t": 1574779685842,
							"value": 120
						}
					]`,
					[]adapter.CapabilityStateView{
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.TemperatureKCapabilityInstance,
								Value:    model.TemperatureK(4500),
							},
						},
					},
				))

				t.Run("old_bright+old_temp", check(
					map[TuyaCommandName]TuyaCommand{
						BrightnessCommand: {
							Code:  string(BrightnessCommand),
							Value: 60,
						},
						TempValueCommand: {
							Code:  string(TempValueCommand),
							Value: 60,
						},
					},
					`[
						{
							"2": "white",
							"code": "work_mode",
							"t": 1574779550133,
							"value": "white"
						}
					]`,
					[]adapter.CapabilityStateView{
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.TemperatureKCapabilityInstance,
								Value:    model.TemperatureK(3400),
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    20.0,
							},
						},
					},
				))

				t.Run("no_data", check(
					nil,
					`[
						{
							"2": "white",
							"code": "work_mode",
							"t": 1574779550133,
							"value": "white"
						}
					]`,
					[]adapter.CapabilityStateView{},
				))
			})
		})

		t.Run("switch_led", func(t *testing.T) {
			t.Run("on", check(
				nil,
				`[
					{
						"1": "true",
						"code": "switch_led",
						"t": 1574768305313,
						"value": true
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{
							Instance: model.OnOnOffCapabilityInstance,
							Value:    true,
						},
					},
				},
			))

			t.Run("off", check(
				nil,
				`[
					{
						"1": "false",
						"code": "switch_led",
						"t": 1574768300474,
						"value": false
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{
							Instance: model.OnOnOffCapabilityInstance,
							Value:    false,
						},
					},
				},
			))
		})

		t.Run("colour_data", func(t *testing.T) {
			t.Run("red", check(
				nil,
				`[
					{
						"code": "colour_data",
						"t": 1574770293411,
						"5": "0400000000ff04",
						"value": "{\"h\":0.0,\"s\":255.0,\"v\":4.1}"
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.HsvColorCapabilityInstance,
							Value:    model.HSV{H: 0, S: 96, V: 1},
						},
					},
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    1.0,
						},
					},
				},
			))

			t.Run("green", check(
				nil,
				`[
					{
						"code": "colour_data",
						"t": 1574770942523,
						"5": "204d00005fff4d",
						"value": "{\"h\":95.0,\"s\":255.0,\"v\":77.0}"
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.HsvColorCapabilityInstance,
							Value:    model.HSV{H: 135, S: 96, V: 30},
						},
					},
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    30,
						},
					},
				},
			))

			t.Run("moon_white", check(
				nil,
				`[
					{
						"code": "colour_data",
						"t": 1574770437214,
						"5": "04040400e71904",
						"value": "{\"h\":231.0,\"s\":25.0,\"v\":4.1}"
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.HsvColorCapabilityInstance,
							Value:    model.HSV{H: 231, S: 9, V: 1},
						},
					},
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    1,
						},
					},
				},
			))

			t.Run("with_ignored_bright_value", check(
				nil,
				`[
					{
						"code": "colour_data",
						"t": 1574770437214,
						"5": "04040400e71904",
						"value": "{\"h\":231.0,\"s\":25.0,\"v\":4.1}"
					},
					{
						"3": "54",
						"code": "bright_value",
						"t": 1574779980637,
						"value": 54
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.HsvColorCapabilityInstance,
							Value:    model.HSV{H: 231, S: 9, V: 1},
						},
					},
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    1.0,
						},
					},
				},
			))
		})

		t.Run("bright_value", func(t *testing.T) {
			t.Run("-1/255<1%", check(
				nil,
				`[
					{
						"3": "-1",
						"code": "bright_value",
						"t": 1574781236155,
						"value": -1
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    1.0,
						},
					},
				},
			))

			t.Run("0/255<1%", check(
				nil,
				`[
					{
						"3": "0",
						"code": "bright_value",
						"t": 1574781236155,
						"value": 0
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    1.0,
						},
					},
				},
			))

			t.Run("1/255<1%", check(
				nil,
				`[
					{
						"3": "1",
						"code": "bright_value",
						"t": 1574781236155,
						"value": 1
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    1.0,
						},
					},
				},
			))

			t.Run("27/255=1%", check(
				nil,
				`[
					{
						"3": "27",
						"code": "bright_value",
						"t": 1574781236155,
						"value": 27
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    1.0,
						},
					},
				},
			))

			t.Run("141/255=65%", check(
				nil,
				`[
					{
						"3": "141",
						"code": "bright_value",
						"t": 1574781236155,
						"value": 141
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    65.0,
						},
					},
				},
			))

			t.Run("203/255=99%", check(
				nil,
				`[
					{
						"3": "203",
						"code": "bright_value",
						"t": 1574781236155,
						"value": 203
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    99.0,
						},
					},
				},
			))

			t.Run("204/255=100%", check(
				nil,
				`[
					{
						"3": "204",
						"code": "bright_value",
						"t": 1574781236155,
						"value": 204
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    100.0,
						},
					},
				},
			))

			t.Run("240/255>100%", check(
				nil,
				`[
					{
						"3": "240",
						"code": "bright_value",
						"t": 1574781236155,
						"value": 240
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    100.0,
						},
					},
				},
			))

			t.Run("255/255>100%", check(
				nil,
				`[
					{
						"3": "255",
						"code": "bright_value",
						"t": 1574781236155,
						"value": 255
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    100.0,
						},
					},
				},
			))

			t.Run("257/255>100%", check(
				nil,
				`[
					{
						"3": "257",
						"code": "bright_value",
						"t": 1574781236155,
						"value": 257
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    100.0,
						},
					},
				},
			))
		})

		t.Run("temp_value", func(t *testing.T) {
			t.Run("0=2700K", check(
				nil,
				`[
					{
						"code": "temp_value",
						"4": "0",
						"t": 1574783099084,
						"value": 0
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.TemperatureKCapabilityInstance,
							Value:    model.TemperatureK(2700),
						},
					},
				},
			))

			t.Run("30=3156", check(
				nil,
				`[
					{
						"code": "temp_value",
						"4": "30",
						"t": 1574783099084,
						"value": 30
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.TemperatureKCapabilityInstance,
							Value:    model.TemperatureK(3156),
						},
					},
				},
			))

			t.Run("60=3400K", check(
				nil,
				`[
					{
						"code": "temp_value",
						"4": "60",
						"t": 1574783099084,
						"value": 60
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.TemperatureKCapabilityInstance,
							Value:    model.TemperatureK(3400),
						},
					},
				},
			))

			t.Run("90=4030K", check(
				nil,
				`[
					{
						"code": "temp_value",
						"4": "90",
						"t": 1574783099084,
						"value": 90
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.TemperatureKCapabilityInstance,
							Value:    model.TemperatureK(4030),
						},
					},
				},
			))

			t.Run("120=4500K", check(
				nil,
				`[
					{
						"code": "temp_value",
						"4": "120",
						"t": 1574783099084,
						"value": 120
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.TemperatureKCapabilityInstance,
							Value:    model.TemperatureK(4500),
						},
					},
				},
			))

			t.Run("190=5600K", check(
				nil,
				`[
					{
						"code": "temp_value",
						"4": "190",
						"t": 1574783099084,
						"value": 190
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.TemperatureKCapabilityInstance,
							Value:    model.TemperatureK(5600),
						},
					},
				},
			))

			t.Run("255=6500K", check(
				nil,
				`[
					{
						"code": "temp_value",
						"4": "255",
						"t": 1574783099084,
						"value": 255
					}
				]`,
				[]adapter.CapabilityStateView{
					{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.TemperatureKCapabilityInstance,
							Value:    model.TemperatureK(6500),
						},
					},
				},
			))
		})
	})
}

func TestPulsarStatuses_ToPropertyStateView(t *testing.T) {
	check := func(rawJSON string, expectedProperties []adapter.PropertyStateView) func(*testing.T) {
		return func(t *testing.T) {
			var statuses PulsarStatuses
			err := json.Unmarshal([]byte(rawJSON), &statuses)
			if assert.NoError(t, err) {
				actualProperties := statuses.ToPropertyStateView()

				if assert.NoError(t, err) {
					assert.ElementsMatch(t, expectedProperties, actualProperties)
				}
			}
		}
	}

	t.Run("empty", check("[]", []adapter.PropertyStateView{}))

	t.Run("timestamp_overrides", check(
		`[
			{
				"3": "0",
				"code": "cur_voltage",
				"t": 1,
				"value": 100
			},
			{
				"3": "0",
				"code": "cur_voltage",
				"t": 2,
				"value": 2200
			}
		]`,
		[]adapter.PropertyStateView{
			{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.VoltagePropertyInstance,
					Value:    220,
				},
			},
		},
	))

	t.Run("smart_socket", check(
		`[
			{
			  "code": "cur_current",
			  "t": 1584375231398,
			  "18": "116",
			  "value": 116
			},
			{
			  "code": "cur_power",
			  "t": 1584375231398,
			  "19": "119",
			  "value": 119
			},
			{
			  "code": "cur_voltage",
			  "t": 1584375231398,
			  "value": 2200,
			  "20": "2200"
			}
		]`,
		[]adapter.PropertyStateView{
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
					Instance: model.PowerPropertyInstance,
					Value:    11.9,
				},
			},
			{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.AmperagePropertyInstance,
					Value:    0.116,
				},
			},
		},
	))
}
