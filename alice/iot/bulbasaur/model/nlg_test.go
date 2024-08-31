package model

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/library/go/ptr"
)

func TestGetDeviceOnOffCapabilityVoiceStatus(t *testing.T) {
	for _, deviceType := range KnownDeviceTypes {
		if slices.Contains(KnownQuasarDeviceTypes, deviceType) || slices.Contains(KnownRemoteCarDeviceTypes, deviceType) {
			continue
		}
		t.Run(fmt.Sprintf("did not forget GetDeviceOnOffCapabilityVoiceStatus(%s, \"\", bool)", deviceType), func(t *testing.T) {
			onState := OnOffCapabilityState{
				Instance: OnOnOffCapabilityInstance,
				Value:    true,
			}
			offState := OnOffCapabilityState{
				Instance: OnOnOffCapabilityInstance,
				Value:    false,
			}
			assert.NotEqual(t, "", GetDeviceOnOffCapabilityVoiceStatus(onState, DeviceType(deviceType), TargetNLGOptions{UseDeviceTarget: false}))
			assert.NotEqual(t, "", GetDeviceOnOffCapabilityVoiceStatus(offState, DeviceType(deviceType), TargetNLGOptions{UseDeviceTarget: false}))
		})
	}
}

func TestGetModeCapabilityVoiceStatus(t *testing.T) {
	for instance := range KnownModeInstancesNames {
		for value := range KnownModes {
			t.Run(fmt.Sprintf("did not forget GetModeCapabilityVoiceStatus({%s, %s})", instance, value), func(t *testing.T) {
				state := ModeCapabilityState{
					Instance: instance,
					Value:    value,
				}
				assert.NotEqual(t, "", GetModeCapabilityVoiceStatus(state, TargetNLGOptions{UseDeviceTarget: false}))
			})
		}
	}
}

func TestGetRangeCapabilityVoiceStatus(t *testing.T) {
	for instance := range KnownRangeInstanceNames {
		t.Run(fmt.Sprintf("did not forget GetRangeCapabilityVoiceStatus(%s)", instance), func(t *testing.T) {
			state := RangeCapabilityState{Instance: instance, Relative: ptr.Bool(false), Value: 0}
			parameters := RangeCapabilityParameters{Instance: instance}
			assert.NotEqual(t, "", GetRangeCapabilityVoiceStatus(state, parameters, TargetNLGOptions{UseDeviceTarget: false}))
		})
	}
}

func TestGetToggleCapabilityVoiceStatus(t *testing.T) {
	for instance := range KnownToggleInstanceNames {
		t.Run(fmt.Sprintf("did not forget GetToggleCapabilityVoiceStatus(%s)", instance), func(t *testing.T) {
			onState := ToggleCapabilityState{
				Instance: instance,
				Value:    true,
			}
			offState := ToggleCapabilityState{
				Instance: instance,
				Value:    false,
			}
			assert.NotEqual(t, "", GetToggleCapabilityVoiceStatus(onState, TargetNLGOptions{UseDeviceTarget: false}))
			assert.NotEqual(t, "", GetToggleCapabilityVoiceStatus(offState, TargetNLGOptions{UseDeviceTarget: false}))
		})
	}
}

func TestGetUnitFloatValueForVoiceStatus(t *testing.T) {
	for unit := range KnownUnits {
		t.Run(fmt.Sprintf("did not forget GetUnitFloatValueForVoiceStatus(0, %s)", unit), func(t *testing.T) {
			// this code expects not equal to 0 to show that every unit is pronounced
			assert.NotEqual(t, "0", GetUnitFloatValueForVoiceStatus(0, unit))
		})
	}
}

func TestGetHouseholdAddition(t *testing.T) {
	type testCase struct {
		inflection  inflector.Inflection
		forSuggests bool
		expected    string
	}
	testCases := []testCase{
		{
			inflection: inflector.Inflection{
				Im:   "родители",
				Rod:  "родителей",
				Dat:  "родителям",
				Vin:  "родителей",
				Tvor: "родителями",
				Pr:   "родителях",
			},
			expected: "у родителей",
		},
		{
			inflection: inflector.Inflection{
				Im:   "квартира",
				Rod:  "квартиры",
				Dat:  "квартире",
				Vin:  "квартиру",
				Tvor: "квартирой",
				Pr:   "квартире",
			},
			expected: "в квартире",
		},
		{
			inflection: inflector.Inflection{
				Im:   "дом",
				Rod:  "дома",
				Dat:  "дому",
				Vin:  "дом",
				Tvor: "домом",
				Pr:   "доме",
			},
			expected: "в доме",
		},
		{
			inflection: inflector.Inflection{
				Im:   "Дача",
				Rod:  "Даче",
				Dat:  "Даче",
				Vin:  "Дачу",
				Tvor: "Дачей",
				Pr:   "Даче",
			},
			expected: "на даче",
		},
		{
			inflection: inflector.Inflection{
				Im:   "офис",
				Rod:  "офиса",
				Dat:  "офису",
				Vin:  "офис",
				Tvor: "офисом",
				Pr:   "офисе",
			},
			expected: "в офисе",
		},
		{
			inflection: inflector.Inflection{
				Im:   "мой дом",
				Rod:  "моего дома",
				Dat:  "моему дому",
				Vin:  "мой дом",
				Tvor: "моим домом",
				Pr:   "моем доме",
			},
			expected: "в вашем доме",
		},
		{
			inflection: inflector.Inflection{
				Im:   "мой дом",
				Rod:  "моего дома",
				Dat:  "моему дому",
				Vin:  "мой дом",
				Tvor: "моим домом",
				Pr:   "моем доме",
			},
			expected:    "в моем доме",
			forSuggests: true,
		},
	}
	for _, tc := range testCases {
		assert.Equal(t, tc.expected, GetHouseholdAddition(tc.inflection, tc.forSuggests))
	}
}

func TestGetDeviceStateVoiceStatus(t *testing.T) {
	type testCase struct {
		nlg    string
		device Device
	}

	testCases := []testCase{
		{
			nlg: "Датчик сейчас в сети, потребление тока 0.33 ампера, температура мне пока неизвестна",
			device: Device{
				Name: "Датчик",
				Properties: Properties{
					MakePropertyByType(FloatPropertyType).WithParameters(FloatPropertyParameters{
						Instance: TemperaturePropertyInstance,
						Unit:     UnitTemperatureCelsius,
					}),
					MakePropertyByType(FloatPropertyType).WithParameters(FloatPropertyParameters{
						Instance: AmperagePropertyInstance,
						Unit:     UnitAmpere,
					}).WithState(FloatPropertyState{
						Instance: AmperagePropertyInstance,
						Value:    0.33,
					}),
				},
			},
		},
		{
			nlg: "Монитор сейчас в сети, данные по датчикам будут позже, но всё работает в штатном режиме!",
			device: Device{
				Name: "Монитор",
				Properties: Properties{
					MakePropertyByType(FloatPropertyType).WithParameters(FloatPropertyParameters{
						Instance: TemperaturePropertyInstance,
						Unit:     UnitTemperatureCelsius,
					}),
					MakePropertyByType(FloatPropertyType).WithParameters(FloatPropertyParameters{
						Instance: HumidityPropertyInstance,
						Unit:     UnitPercent,
					}),
					MakePropertyByType(FloatPropertyType).WithParameters(FloatPropertyParameters{
						Instance: PressurePropertyInstance,
						Unit:     UnitPressureMmHg,
					}),
				},
			},
		},
		{
			nlg: "Монитор сейчас в сети, температура 27 градусов цельсия, влажность, давление мне пока неизвестны",
			device: Device{
				Name: "Монитор",
				Properties: Properties{
					MakePropertyByType(FloatPropertyType).WithParameters(FloatPropertyParameters{
						Instance: TemperaturePropertyInstance,
						Unit:     UnitTemperatureCelsius,
					}).WithState(FloatPropertyState{
						Instance: TemperaturePropertyInstance,
						Value:    27,
					}),
					MakePropertyByType(FloatPropertyType).WithParameters(FloatPropertyParameters{
						Instance: HumidityPropertyInstance,
						Unit:     UnitPercent,
					}),
					MakePropertyByType(FloatPropertyType).WithParameters(FloatPropertyParameters{
						Instance: PressurePropertyInstance,
						Unit:     UnitPressureMmHg,
					}),
				},
			},
		},
	}
	for _, tc := range testCases {
		options := TargetNLGOptions{
			UseDeviceTarget:  false,
			DeviceName:       tc.device.Name,
			DeviceInflection: inflector.Inflection{},
			UseRoomTarget:    false,
			RoomName:         "",
			RoomInflection:   inflector.Inflection{},
		}
		assert.Equal(t, tc.nlg, GetDeviceStateVoiceStatus(tc.device, options))
	}
}
