package model

import (
	"math/rand"
	"reflect"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestGetGroupCapabilityFromDevicesByTypeAndInstance_OnOffCapabilityType(t *testing.T) {
	capabilityOn := MakeCapabilityByType(OnOffCapabilityType)
	capabilityOn.SetRetrievable(true)
	capabilityOn.SetState(OnOffCapabilityState{
		Instance: "on",
		Value:    true,
	})

	capabilityOnSplit := MakeCapabilityByType(OnOffCapabilityType)
	capabilityOnSplit.SetRetrievable(true)
	capabilityOnSplit.SetState(OnOffCapabilityState{
		Instance: "on",
		Value:    true,
	})
	capabilityOnSplit.SetParameters(OnOffCapabilityParameters{Split: true})

	capabilityOff := MakeCapabilityByType(OnOffCapabilityType)
	capabilityOff.SetRetrievable(true)
	capabilityOff.SetState(OnOffCapabilityState{
		Instance: "on",
		Value:    false,
	})

	capabilityUnknown := MakeCapabilityByType(OnOffCapabilityType)
	capabilityUnknown.SetRetrievable(false)

	capabilityUnknownSplit := MakeCapabilityByType(OnOffCapabilityType)
	capabilityUnknownSplit.SetRetrievable(false)
	capabilityUnknownSplit.SetParameters(OnOffCapabilityParameters{Split: true})

	capabilityColor := MakeCapabilityByType(ColorSettingCapabilityType)
	capabilityColor.SetRetrievable(true)
	capabilityColor.SetState(ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    TemperatureK(3000),
	})

	deviceOn := Device{
		Capabilities: []ICapability{capabilityOn},
	}

	deviceOnSplit := Device{
		Capabilities: []ICapability{capabilityOnSplit},
	}

	deviceOff1 := Device{
		Capabilities: []ICapability{capabilityOff},
	}

	deviceOff2 := Device{
		Capabilities: []ICapability{capabilityOff},
	}

	deviceUnknown := Device{
		Capabilities: []ICapability{capabilityUnknown},
	}

	deviceWrong := Device{
		Capabilities: []ICapability{capabilityColor},
	}

	var capability ICapability
	var ok bool

	// []
	_, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, false, ok, "[]")

	// [wrongDevice]
	_, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceWrong}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, false, ok, "[wrongDevice]")

	// [cap=on]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOn}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOn, capability, "[cap=on]")
	assert.Equal(t, true, ok, "[cap=on]")

	// [cap=on]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOnSplit}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOnSplit, capability, "[cap=on]")
	assert.Equal(t, true, ok, "[cap=on]")

	// [cap=off]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOff1}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOff, capability, "[cap=off]")
	assert.Equal(t, true, ok, "[cap=off]")

	// [cap=unknown]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceUnknown}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityUnknown, capability, "[cap=unknown]")
	assert.Equal(t, true, ok, "[cap=unknown]")

	// [wrongDevice, cap=on]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceWrong, deviceOn}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOn, capability, "[wrongDevice, cap=on]")
	assert.Equal(t, true, ok, "[wrongDevice, cap=on]")

	// [cap=on, wrongDevice]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOn, deviceWrong}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOn, capability, "[cap=on, wrongDevice]")
	assert.Equal(t, true, ok, "[cap=on, wrongDevice]")

	// [cap=on_split, wrongDevice]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOnSplit, deviceWrong}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOnSplit, capability, "[cap=on_split, wrongDevice]")
	assert.Equal(t, true, ok, "[cap=on_split, wrongDevice]")

	// [cap=on, cap=on_split]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOn, deviceOnSplit}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOnSplit, capability, "[cap=on, cap=on_split]")
	assert.Equal(t, true, ok, "[cap=on, cap=on_split]")

	// [cap=off, cap=off]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOff1, deviceOff2}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOff, capability, "[cap=off, cap=off]")
	assert.Equal(t, true, ok, "[cap=off, cap=off]")

	// [cap=on, cap=off]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOn, deviceOff1}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOn, capability, "[cap=on, cap=off]")
	assert.Equal(t, true, ok, "[cap=on, cap=off]")

	// [cap=on_split, cap=off]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOnSplit, deviceOff1}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOnSplit, capability, "[cap=on_split, cap=off]")
	assert.Equal(t, true, ok, "[cap=on_split, cap=off]")

	// [cap=off, cap=on]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOff1, deviceOn}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOn, capability, "[cap=off, cap=on]")
	assert.Equal(t, true, ok, "[cap=off, cap=on]")

	// [cap=off, cap=on_split]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOff1, deviceOnSplit}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityOnSplit, capability, "[cap=off, cap=on_split]")
	assert.Equal(t, true, ok, "[cap=off, cap=on_split]")

	// [cap=on, cap=unknown]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOn, deviceUnknown}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityUnknown, capability, "[cap=on, cap=unknown]")
	assert.Equal(t, true, ok, "[cap=on, cap=unknown]")

	// [cap=on_split, cap=unknown]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOnSplit, deviceUnknown}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityUnknownSplit, capability, "[cap=on_split, cap=unknown]")
	assert.Equal(t, true, ok, "[cap=on_split, cap=unknown]")

	// [cap=unknown, cap=on]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceUnknown, deviceOn}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityUnknown, capability, "[cap=unknown, cap=on]")
	assert.Equal(t, true, ok, "[cap=unknown, cap=on]")

	// [cap=unknown, cap=on_split]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceUnknown, deviceOnSplit}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityUnknownSplit, capability, "[cap=unknown, cap=on_split]")
	assert.Equal(t, true, ok, "[cap=unknown, cap=on_split]")

	// [cap=off, cap=unknown]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOff1, deviceUnknown}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityUnknown, capability, "[cap=off, cap=unknown]")
	assert.Equal(t, true, ok, "[cap=off, cap=unknown]")

	// [cap=unknown, cap=off]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceUnknown, deviceOff1}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityUnknown, capability, "[cap=unknown, cap=off]")
	assert.Equal(t, true, ok, "[cap=unknown, cap=off]")

	// [cap=off, wrongDevice, cap=unknown, cap=on, cap=on_split]
	capability, ok = GetGroupCapabilityFromDevicesByTypeAndInstance([]Device{deviceOff1, deviceWrong, deviceUnknown, deviceOn, deviceOnSplit}, OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	assert.Equal(t, capabilityUnknownSplit, capability, "[cap=off, wrongDevice, cap=unknown, cap=on, cap=on_split]")
	assert.Equal(t, true, ok, "[cap=off, wrongDevice, cap=unknown, cap=on, cap=on_split]")
}

func TestGroupStateFromDevicesByTypeAndInstance(t *testing.T) {
	capabilityOn := MakeCapabilityByType(OnOffCapabilityType)
	capabilityOn.SetRetrievable(true)
	capabilityOn.SetState(OnOffCapabilityState{
		Instance: "on",
		Value:    true,
	})

	capabilityOff := MakeCapabilityByType(OnOffCapabilityType)
	capabilityOff.SetRetrievable(true)
	capabilityOff.SetState(OnOffCapabilityState{
		Instance: "on",
		Value:    false,
	})

	capabilityUnknown := MakeCapabilityByType(OnOffCapabilityType)
	capabilityUnknown.SetRetrievable(false)

	capabilityColor := MakeCapabilityByType(ColorSettingCapabilityType)
	capabilityColor.SetRetrievable(true)
	capabilityColor.SetState(ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    TemperatureK(3000),
	})

	deviceOn1 := Device{
		Capabilities: []ICapability{capabilityOn},
	}

	deviceOn2 := Device{
		Capabilities: []ICapability{capabilityOn},
	}

	deviceOff1 := Device{
		Capabilities: []ICapability{capabilityOff},
	}

	deviceOff2 := Device{
		Capabilities: []ICapability{capabilityOff},
	}

	deviceUnknown := Device{
		Capabilities: []ICapability{capabilityUnknown},
	}

	deviceWrong := Device{
		Capabilities: []ICapability{capabilityColor},
	}

	var state DeviceStatus

	// []
	state = GroupOnOffStateFromDevices([]Device{})
	assert.Equal(t, OnlineDeviceStatus, state, "[]")

	// [wrongDevice]
	state = GroupOnOffStateFromDevices([]Device{deviceWrong})
	assert.Equal(t, OnlineDeviceStatus, state, "[wrongDevice]")

	// [cap=on]
	state = GroupOnOffStateFromDevices([]Device{deviceOn1})
	assert.Equal(t, OnlineDeviceStatus, state, "[cap=on]")

	// [cap=unknown]
	state = GroupOnOffStateFromDevices([]Device{deviceUnknown})
	assert.Equal(t, OnlineDeviceStatus, state, "[cap=unknown]")

	// [cap=on, cap=on]
	state = GroupOnOffStateFromDevices([]Device{deviceOn1, deviceOn2})
	assert.Equal(t, OnlineDeviceStatus, state, "[cap=on, cap=on]")

	// [cap=off, cap=off]
	state = GroupOnOffStateFromDevices([]Device{deviceOff1, deviceOff2})
	assert.Equal(t, OnlineDeviceStatus, state, "[cap=off, cap=off]")

	// [wrongDevice, cap=on]
	state = GroupOnOffStateFromDevices([]Device{deviceWrong, deviceOn1})
	assert.Equal(t, OnlineDeviceStatus, state, "[wrongDevice, cap=on]")

	// [cap=on, wrongDevice]
	state = GroupOnOffStateFromDevices([]Device{deviceOn1, deviceWrong})
	assert.Equal(t, OnlineDeviceStatus, state, "[cap=on, wrongDevice]")

	// [cap=on, cap=off]
	state = GroupOnOffStateFromDevices([]Device{deviceOn1, deviceOff1})
	assert.Equal(t, SplitStatus, state, "[cap=on, cap=off]")

	// [cap=on, cap=unknown]
	state = GroupOnOffStateFromDevices([]Device{deviceOn1, deviceUnknown})
	assert.Equal(t, SplitStatus, state, "[cap=on, cap=unknown]")

	// [cap=off, cap=unknown]
	state = GroupOnOffStateFromDevices([]Device{deviceOff1, deviceUnknown})
	assert.Equal(t, SplitStatus, state, "[cap=off, cap=unknown]")
}

func TestGetCapabilityParametersMerge(t *testing.T) {
	t.Run("OnOff", func(t *testing.T) {
		on1 := OnOffCapabilityParameters{}
		t.Run("Merge", func(t *testing.T) {
			actual := on1.Merge(OnOffCapabilityParameters{})
			expected := OnOffCapabilityParameters{}
			assert.Equal(t, expected, actual)
		})
		t.Run("NoMerge", func(t *testing.T) {
			actual := OnOffCapabilityParameters{}.Merge(nil)
			assert.Nil(t, actual)
		})
	})
	t.Run("Toggle", func(t *testing.T) {
		mute1 := ToggleCapabilityParameters{Instance: MuteToggleCapabilityInstance}
		mute2 := ToggleCapabilityParameters{Instance: MuteToggleCapabilityInstance}
		otherToggle := ToggleCapabilityParameters{Instance: "otherToggleInstance"}

		t.Run("Merge", func(t *testing.T) {
			actual := mute1.Merge(mute2)
			expected := ToggleCapabilityParameters{Instance: MuteToggleCapabilityInstance}
			assert.Equal(t, expected, actual)
		})
		t.Run("NoMerge", func(t *testing.T) {
			actual := mute1.Merge(otherToggle)
			assert.Nil(t, actual)

			actual = mute1.Merge(nil)
			assert.Nil(t, actual)
		})
	})
	t.Run("Mode", func(t *testing.T) {
		richThermostat1 := ModeCapabilityParameters{
			Instance: ThermostatModeInstance,
			Modes:    []Mode{{Value: AutoMode}, {Value: CoolMode}, {Value: HeatMode}, {Value: DryMode}},
		}
		richThermostat2 := ModeCapabilityParameters{
			Instance: ThermostatModeInstance,
			Modes:    []Mode{{Value: CoolMode}, {Value: AutoMode}, {Value: DryMode}, {Value: HeatMode}},
		}
		poorThermostat1 := ModeCapabilityParameters{
			Instance: ThermostatModeInstance,
			Modes:    []Mode{{Value: EcoMode}, {Value: AutoMode}},
		}
		ecoThermostat1 := ModeCapabilityParameters{
			Instance: ThermostatModeInstance,
			Modes:    []Mode{{Value: EcoMode}},
		}
		fan1 := ModeCapabilityParameters{
			Instance: FanSpeedModeInstance,
			Modes:    []Mode{{Value: AutoMode}},
		}
		t.Run("Merge", func(t *testing.T) {
			actual := richThermostat1.Merge(richThermostat2)
			expected := ModeCapabilityParameters{
				Instance: ThermostatModeInstance,
				Modes:    []Mode{KnownModes[AutoMode], KnownModes[CoolMode], KnownModes[HeatMode], KnownModes[DryMode]},
			}
			assert.Equal(t, expected.Instance, actual.(ModeCapabilityParameters).Instance)
			assert.ElementsMatch(t, expected.Modes, actual.(ModeCapabilityParameters).Modes)

			actual = richThermostat1.Merge(poorThermostat1)
			expected = ModeCapabilityParameters{
				Instance: ThermostatModeInstance,
				Modes:    []Mode{KnownModes[AutoMode]},
			}
			assert.Equal(t, expected, actual)

			actual = poorThermostat1.Merge(richThermostat1)
			expected = ModeCapabilityParameters{
				Instance: ThermostatModeInstance,
				Modes:    []Mode{KnownModes[AutoMode]},
			}
			assert.Equal(t, expected, actual)
		})
		t.Run("NoMerge", func(t *testing.T) {
			actual := richThermostat1.Merge(ecoThermostat1)
			assert.Nil(t, actual)

			actual = richThermostat1.Merge(fan1)
			assert.Nil(t, actual)

			actual = richThermostat1.Merge(nil)
			assert.Nil(t, actual)
		})
	})
	t.Run("ColorSetting", func(t *testing.T) {
		lampRgbTempK1 := ColorSettingCapabilityParameters{
			ColorModel: CM(RgbModelType),
			TemperatureK: &TemperatureKParameters{
				Min: 2700,
				Max: 6500,
			},
		}
		lampRgbTempK2 := ColorSettingCapabilityParameters{
			ColorModel: CM(RgbModelType),
			TemperatureK: &TemperatureKParameters{
				Min: 2700,
				Max: 6500,
			},
		}
		lampHsvTempK1 := ColorSettingCapabilityParameters{
			ColorModel: CM(HsvModelType),
			TemperatureK: &TemperatureKParameters{
				Min: 2700,
				Max: 6500,
			},
		}
		lampHsv1 := ColorSettingCapabilityParameters{
			ColorModel: CM(HsvModelType),
		}
		lampRgb1 := ColorSettingCapabilityParameters{
			ColorModel: CM(RgbModelType),
		}
		lampTempK1 := ColorSettingCapabilityParameters{
			TemperatureK: &TemperatureKParameters{
				Min: 2700,
				Max: 6500,
			},
		}
		lampTempK2 := ColorSettingCapabilityParameters{
			TemperatureK: &TemperatureKParameters{
				Min: 3400,
				Max: 5600,
			},
		}
		lampTempK3 := ColorSettingCapabilityParameters{
			TemperatureK: &TemperatureKParameters{
				Min: 2700,
				Max: 3400,
			},
		}
		lampTempK4 := ColorSettingCapabilityParameters{
			TemperatureK: &TemperatureKParameters{
				Min: 5600,
				Max: 6500,
			},
		}

		t.Run("Merge", func(t *testing.T) {
			actual := lampRgbTempK1.Merge(lampRgbTempK2)
			expected := ColorSettingCapabilityParameters{
				ColorModel: CM(HsvModelType),
				TemperatureK: &TemperatureKParameters{
					Min: 2700,
					Max: 6500,
				},
			}
			assert.Equal(t, expected, actual)

			actual = lampRgbTempK1.Merge(lampHsvTempK1)
			expected = ColorSettingCapabilityParameters{
				ColorModel: CM(HsvModelType),
				TemperatureK: &TemperatureKParameters{
					Min: 2700,
					Max: 6500,
				},
			}
			assert.Equal(t, expected, actual)

			actual = lampRgbTempK1.Merge(lampTempK1)
			expected = ColorSettingCapabilityParameters{
				TemperatureK: &TemperatureKParameters{
					Min: 2700,
					Max: 6500,
				},
			}
			assert.Equal(t, expected, actual)

			actual = lampRgbTempK1.Merge(lampTempK2)
			expected = ColorSettingCapabilityParameters{
				TemperatureK: &TemperatureKParameters{
					Min: 3400,
					Max: 5600,
				},
			}
			assert.Equal(t, expected, actual)

			actual = lampRgbTempK1.Merge(lampRgb1)
			expected = ColorSettingCapabilityParameters{
				ColorModel: CM(HsvModelType),
			}
			assert.Equal(t, expected, actual)

			actual = lampHsvTempK1.Merge(lampHsv1)
			expected = ColorSettingCapabilityParameters{
				ColorModel: CM(HsvModelType),
			}
			assert.Equal(t, expected, actual)
		})
		t.Run("NoMerge", func(t *testing.T) {
			actual := lampHsvTempK1.Merge(nil)
			assert.Nil(t, actual)

			actual = lampTempK3.Merge(lampTempK4)
			assert.Nil(t, actual)

			actual = lampHsv1.Merge(lampTempK1)
			assert.Nil(t, actual)

			actual = lampRgb1.Merge(lampTempK1)
			assert.Nil(t, actual)
		})
	})
	t.Run("Range", func(t *testing.T) {
		brightness1 := RangeCapabilityParameters{
			Instance:     BrightnessRangeInstance,
			Unit:         UnitPercent,
			RandomAccess: true,
			Looped:       false,
			Range: &Range{
				Min:       1,
				Max:       100,
				Precision: 1,
			},
		}
		kelvinTemp1 := RangeCapabilityParameters{
			Instance:     TemperatureRangeInstance,
			Unit:         UnitTemperatureKelvin,
			RandomAccess: true,
			Looped:       false,
			Range: &Range{
				Min:       1,
				Max:       100,
				Precision: 1,
			},
		}
		celcisusTemp1 := RangeCapabilityParameters{
			Instance:     TemperatureRangeInstance,
			Unit:         UnitTemperatureCelsius,
			RandomAccess: false,
			Looped:       false,
			Range: &Range{
				Min:       1,
				Max:       100,
				Precision: 1,
			},
		}
		celcisusTemp2 := RangeCapabilityParameters{
			Instance:     TemperatureRangeInstance,
			Unit:         UnitTemperatureCelsius,
			RandomAccess: true,
			Looped:       true,
			Range: &Range{
				Min:       1,
				Max:       100,
				Precision: 1,
			},
		}
		celcisusTemp3 := RangeCapabilityParameters{
			Instance:     TemperatureRangeInstance,
			Unit:         UnitTemperatureCelsius,
			RandomAccess: true,
			Looped:       true,
			Range: &Range{
				Min:       20,
				Max:       30,
				Precision: 1,
			},
		}
		t.Run("Merge", func(t *testing.T) {
			actual := brightness1.Merge(brightness1)
			expected := RangeCapabilityParameters{
				Instance:     BrightnessRangeInstance,
				Unit:         UnitPercent,
				RandomAccess: true,
				Looped:       false,
				Range: &Range{
					Min:       1,
					Max:       100,
					Precision: 1,
				},
			}
			assert.Equal(t, expected, actual)

			actual = celcisusTemp1.Merge(celcisusTemp2)
			expected = RangeCapabilityParameters{
				Instance:     TemperatureRangeInstance,
				Unit:         UnitTemperatureCelsius,
				RandomAccess: false,
				Looped:       false,
				Range: &Range{
					Min:       1,
					Max:       100,
					Precision: 1,
				},
			}
			assert.Equal(t, expected, actual)

			actual = celcisusTemp1.Merge(celcisusTemp3)
			expected = RangeCapabilityParameters{
				Instance:     TemperatureRangeInstance,
				Unit:         UnitTemperatureCelsius,
				RandomAccess: false,
				Looped:       false,
				Range: &Range{
					Min:       20,
					Max:       30,
					Precision: 1,
				},
			}
			assert.Equal(t, expected, actual)
		})
		t.Run("NoMerge", func(t *testing.T) {
			actual := brightness1.Merge(kelvinTemp1)
			assert.Nil(t, actual)

			actual = celcisusTemp1.Merge(kelvinTemp1)
			assert.Nil(t, actual)
		})
	})
	t.Run("Different", func(t *testing.T) {
		params := []ICapabilityParameters{
			OnOffCapabilityParameters{}, ToggleCapabilityParameters{}, ModeCapabilityParameters{}, ColorSettingCapabilityParameters{}, RangeCapabilityParameters{},
		}
		for i := 0; i < len(params); i++ {
			for j := i + 1; j < len(params); j++ {
				actual := params[i].Merge(params[j])
				assert.Nil(t, actual)
			}
		}
	})
}

func TestGetCapabilityStatesMerge(t *testing.T) {
	t.Run("OnOff", func(t *testing.T) {
		on1 := OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true}
		on2 := OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true}
		off1 := OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: false}
		off2 := OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: false}

		t.Run("Merge", func(t *testing.T) {
			actual := on1.Merge(on2)
			expected := OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true}
			assert.Equal(t, expected, actual)

			actual = off1.Merge(off2)
			expected = OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: false}
			assert.Equal(t, expected, actual)

			actual = on1.Merge(off1)
			expected = OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true}
			assert.Equal(t, expected, actual)
		})
		t.Run("NoMerge", func(t *testing.T) {
			actual := on1.Merge(nil)
			assert.Nil(t, actual)
		})
	})
	t.Run("Toggle", func(t *testing.T) {
		muteOn1 := ToggleCapabilityState{Instance: MuteToggleCapabilityInstance, Value: true}
		muteOn2 := ToggleCapabilityState{Instance: MuteToggleCapabilityInstance, Value: true}
		muteOff1 := ToggleCapabilityState{Instance: MuteToggleCapabilityInstance, Value: false}
		muteOff2 := ToggleCapabilityState{Instance: MuteToggleCapabilityInstance, Value: false}

		t.Run("Merge", func(t *testing.T) {
			actual := muteOn1.Merge(muteOn2)
			expected := ToggleCapabilityState{Instance: MuteToggleCapabilityInstance, Value: true}
			assert.Equal(t, expected, actual)

			actual = muteOff1.Merge(muteOff2)
			expected = ToggleCapabilityState{Instance: MuteToggleCapabilityInstance, Value: false}
			assert.Equal(t, expected, actual)
		})
		t.Run("Split", func(t *testing.T) {
			actual := muteOn1.Merge(muteOff1)
			assert.Nil(t, actual)
		})
		t.Run("NoMerge", func(t *testing.T) {
			actual := muteOn1.Merge(nil)
			assert.Nil(t, actual)
		})
	})
	t.Run("Modes", func(t *testing.T) {
		coolMode1 := ModeCapabilityState{Instance: ThermostatModeInstance, Value: CoolMode}
		coolMode2 := ModeCapabilityState{Instance: ThermostatModeInstance, Value: CoolMode}
		heatMode1 := ModeCapabilityState{Instance: ThermostatModeInstance, Value: HeatMode}
		autoMode1 := ModeCapabilityState{Instance: FanSpeedModeInstance, Value: AutoMode}

		t.Run("Merge", func(t *testing.T) {
			actual := coolMode1.Merge(coolMode2)
			expected := ModeCapabilityState{Instance: ThermostatModeInstance, Value: CoolMode}
			assert.Equal(t, expected, actual)
		})
		t.Run("Split", func(t *testing.T) {
			actual := coolMode1.Merge(heatMode1)
			assert.Nil(t, actual)
		})
		t.Run("NoMerge", func(t *testing.T) {
			actual := heatMode1.Merge(autoMode1)
			assert.Nil(t, actual)

			actual = coolMode1.Merge(nil)
			assert.Nil(t, actual)
		})
	})
	t.Run("ColorSetting", func(t *testing.T) {
		rgb1 := ColorSettingCapabilityState{Instance: RgbColorCapabilityInstance, Value: RGB(1000)}
		rgb2 := ColorSettingCapabilityState{Instance: RgbColorCapabilityInstance, Value: RGB(1000)}
		rgb3 := ColorSettingCapabilityState{Instance: RgbColorCapabilityInstance, Value: RGB(4700)}
		tempK1 := ColorSettingCapabilityState{Instance: TemperatureKCapabilityInstance, Value: TemperatureK(6500)}
		tempK2 := ColorSettingCapabilityState{Instance: TemperatureKCapabilityInstance, Value: TemperatureK(6500)}
		tempK3 := ColorSettingCapabilityState{Instance: TemperatureKCapabilityInstance, Value: TemperatureK(4700)}
		hsv1 := ColorSettingCapabilityState{Instance: HsvColorCapabilityInstance, Value: HSV{H: 10, S: 0, V: 0}}
		hsv2 := ColorSettingCapabilityState{Instance: HsvColorCapabilityInstance, Value: HSV{H: 10, S: 0, V: 0}}
		hsv3 := ColorSettingCapabilityState{Instance: HsvColorCapabilityInstance, Value: HSV{H: 10, S: 0, V: 100}}

		t.Run("Merge", func(t *testing.T) {
			actual := rgb1.Merge(rgb2)
			expected := ColorSettingCapabilityState{Instance: "hsv", Value: HSV{H: 239, S: 100, V: 91}}
			assert.Equal(t, expected, actual)

			actual = tempK1.Merge(tempK2)
			expected = ColorSettingCapabilityState{Instance: TemperatureKCapabilityInstance, Value: TemperatureK(6500)}
			assert.Equal(t, expected, actual)

			actual = hsv1.Merge(hsv2)
			expected = ColorSettingCapabilityState{Instance: HsvColorCapabilityInstance, Value: HSV{H: 10, S: 0, V: 0}}
			assert.Equal(t, expected, actual)
		})
		t.Run("Split", func(t *testing.T) {
			actual := rgb1.Merge(rgb3)
			assert.Nil(t, actual)

			actual = tempK1.Merge(tempK3)
			assert.Nil(t, actual)

			actual = hsv1.Merge(hsv3)
			assert.Nil(t, actual)
		})
		t.Run("NoMerge", func(t *testing.T) {
			actual := hsv1.Merge(rgb1)
			assert.Nil(t, actual)

			actual = tempK1.Merge(rgb1)
			assert.Nil(t, actual)

			actual = tempK1.Merge(hsv1)
			assert.Nil(t, actual)

			actual = rgb3.Merge(hsv3)
			assert.Nil(t, actual)

			actual = rgb1.Merge(nil)
			assert.Nil(t, actual)
		})
	})
	t.Run("Range", func(t *testing.T) {
		brightness1 := RangeCapabilityState{
			Instance: BrightnessRangeInstance,
			Value:    5,
		}
		temp1 := RangeCapabilityState{
			Instance: TemperatureRangeInstance,
			Value:    5,
		}
		temp2 := RangeCapabilityState{
			Instance: TemperatureRangeInstance,
			Value:    2,
		}
		t.Run("Merge", func(t *testing.T) {
			actual := brightness1.Merge(brightness1)
			expected := RangeCapabilityState{
				Instance: BrightnessRangeInstance,
				Value:    5,
			}
			assert.Equal(t, expected, actual)

			actual = temp1.Merge(temp1)
			expected = RangeCapabilityState{
				Instance: TemperatureRangeInstance,
				Value:    5,
			}
			assert.Equal(t, expected, actual)
		})
		t.Run("NoMerge", func(t *testing.T) {
			actual := brightness1.Merge(temp1)
			assert.Nil(t, actual)

			actual = temp2.Merge(temp1)
			assert.Nil(t, actual)
		})
	})
	t.Run("Different", func(t *testing.T) {
		states := []ICapabilityState{
			OnOffCapabilityState{}, ToggleCapabilityState{}, ModeCapabilityState{}, ColorSettingCapabilityState{}, RangeCapabilityState{},
		}
		for i := 0; i < len(states); i++ {
			for j := i + 1; j < len(states); j++ {
				actual := states[i].Merge(states[j])
				assert.Nil(t, actual)
			}
		}
	})
}

func TestGetCapabilityMerge(t *testing.T) {
	t.Run("OnOff", func(t *testing.T) {
		on1 := MakeCapabilityByType(OnOffCapabilityType)
		on1.SetRetrievable(true)
		on1.SetState(OnOffCapabilityState{
			Instance: OnOnOffCapabilityInstance,
			Value:    true,
		})

		on2 := MakeCapabilityByType(OnOffCapabilityType)
		on2.SetRetrievable(true)
		on2.SetState(OnOffCapabilityState{
			Instance: OnOnOffCapabilityInstance,
			Value:    true,
		})

		retrievableOff1 := MakeCapabilityByType(OnOffCapabilityType)
		retrievableOff1.SetRetrievable(true)
		retrievableOff1.SetState(OnOffCapabilityState{
			Instance: OnOnOffCapabilityInstance,
			Value:    false,
		})

		unretrievableOff2 := MakeCapabilityByType(OnOffCapabilityType)
		unretrievableOff2.SetRetrievable(false)
		unretrievableOff2.SetState(OnOffCapabilityState{
			Instance: OnOnOffCapabilityInstance,
			Value:    false,
		})

		t.Run("Merge", func(t *testing.T) {
			actual, canMerge := on1.Merge(on2)
			assert.True(t, canMerge)
			expected := MakeCapabilityByType(OnOffCapabilityType)
			expected.SetRetrievable(true)
			expected.SetState(OnOffCapabilityState{
				Instance: OnOnOffCapabilityInstance,
				Value:    true,
			})

			assert.Equal(t, expected, actual)

			actual, canMerge = retrievableOff1.Merge(unretrievableOff2)
			assert.True(t, canMerge)
			expected = MakeCapabilityByType(OnOffCapabilityType)
			expected.SetRetrievable(false)
			expected.SetState(OnOffCapabilityState{
				Instance: OnOnOffCapabilityInstance,
				Value:    false,
			})
			assert.Equal(t, expected, actual)
		})
		t.Run("Split", func(t *testing.T) {
			actual, canMerge := on1.Merge(unretrievableOff2)
			assert.True(t, canMerge)
			expected := MakeCapabilityByType(OnOffCapabilityType)
			expected.SetRetrievable(false)
			expected.SetState(OnOffCapabilityState{
				Instance: OnOnOffCapabilityInstance,
				Value:    true,
			})
			assert.Equal(t, expected, actual)
		})
		t.Run("MultipleMerge", func(t *testing.T) {
			actual, canMerge := Capabilities{on1, on1, on1, on2, on2, on2}.Merge()
			assert.True(t, canMerge)
			expected := MakeCapabilityByType(OnOffCapabilityType)
			expected.SetRetrievable(true)
			expected.SetState(OnOffCapabilityState{
				Instance: OnOnOffCapabilityInstance,
				Value:    true,
			})
			assert.Equal(t, expected, actual)
		})
	})

	t.Run("Mode", func(t *testing.T) {
		richThermostat1 := MakeCapabilityByType(ModeCapabilityType)
		richThermostat1.SetRetrievable(true)
		richThermostat1.SetState(ModeCapabilityState{
			Instance: ThermostatModeInstance,
			Value:    CoolMode,
		})
		richThermostat1.SetParameters(ModeCapabilityParameters{
			Instance: ThermostatModeInstance,
			Modes:    []Mode{{Value: AutoMode}, {Value: CoolMode}, {Value: HeatMode}, {Value: DryMode}},
		})

		richThermostat2 := MakeCapabilityByType(ModeCapabilityType)
		richThermostat2.SetRetrievable(true)
		richThermostat2.SetState(
			ModeCapabilityState{
				Instance: ThermostatModeInstance,
				Value:    CoolMode,
			})
		richThermostat2.SetParameters(
			ModeCapabilityParameters{
				Instance: ThermostatModeInstance,
				Modes:    []Mode{{Value: AutoMode}, {Value: CoolMode}, {Value: HeatMode}, {Value: DryMode}},
			})

		poorThermostat1 := MakeCapabilityByType(ModeCapabilityType)
		poorThermostat1.SetRetrievable(true)
		poorThermostat1.SetState(
			ModeCapabilityState{
				Instance: ThermostatModeInstance,
				Value:    EcoMode,
			})
		poorThermostat1.SetParameters(
			ModeCapabilityParameters{
				Instance: ThermostatModeInstance,
				Modes:    []Mode{{Value: AutoMode}, {Value: EcoMode}},
			})

		ecoThermostat1 := MakeCapabilityByType(ModeCapabilityType)
		ecoThermostat1.SetRetrievable(true)
		ecoThermostat1.SetState(
			ModeCapabilityState{
				Instance: ThermostatModeInstance,
				Value:    EcoMode,
			})
		ecoThermostat1.SetParameters(
			ModeCapabilityParameters{
				Instance: ThermostatModeInstance,
				Modes:    []Mode{{Value: EcoMode}},
			})

		fan1 := MakeCapabilityByType(ModeCapabilityType)
		fan1.SetRetrievable(true)
		fan1.SetState(ModeCapabilityState{
			Instance: FanSpeedModeInstance,
			Value:    AutoMode,
		})
		fan1.SetParameters(ModeCapabilityParameters{
			Instance: FanSpeedModeInstance,
			Modes:    []Mode{{Value: AutoMode}},
		})

		t.Run("Merge", func(t *testing.T) {
			actual, canMerge := richThermostat1.Merge(richThermostat2)
			assert.True(t, canMerge)
			expected := MakeCapabilityByType(ModeCapabilityType)
			expected.SetRetrievable(true)
			expected.SetState(ModeCapabilityState{
				Instance: ThermostatModeInstance,
				Value:    CoolMode,
			})
			expected.SetParameters(ModeCapabilityParameters{
				Instance: ThermostatModeInstance,
				Modes:    []Mode{KnownModes[AutoMode], KnownModes[CoolMode], KnownModes[HeatMode], KnownModes[DryMode]},
			})
			assert.Equal(t, expected.Retrievable(), actual.Retrievable())
			assert.Equal(t, expected.Type(), actual.Type())
			assert.Equal(t, expected.State(), actual.State())
			assert.Equal(t, expected.Instance(), actual.Instance())
			assert.ElementsMatch(t, expected.Parameters().(ModeCapabilityParameters).Modes, actual.Parameters().(ModeCapabilityParameters).Modes)
		})
		t.Run("Split", func(t *testing.T) {
			actual, canMerge := richThermostat1.Merge(poorThermostat1)
			assert.True(t, canMerge)
			expected := MakeCapabilityByType(ModeCapabilityType)
			expected.SetRetrievable(true)
			expected.SetParameters(ModeCapabilityParameters{
				Instance: ThermostatModeInstance,
				Modes:    []Mode{KnownModes[AutoMode]},
			})
			assert.Equal(t, expected, actual)
		})
		t.Run("NoMerge", func(t *testing.T) {
			actual, canMerge := richThermostat1.Merge(ecoThermostat1)
			assert.False(t, canMerge)
			assert.Nil(t, actual)

			actual, canMerge = richThermostat1.Merge(fan1)
			assert.False(t, canMerge)
			assert.Nil(t, actual)
		})
		t.Run("MultipleNoMerge", func(t *testing.T) {
			capabilities := Capabilities{richThermostat1, ecoThermostat1, poorThermostat1, richThermostat2, richThermostat2}
			for i := 0; i < 10; i++ {
				rand.Shuffle(len(capabilities), func(i, j int) { capabilities[i], capabilities[j] = capabilities[j], capabilities[i] })
				actual, canMerge := capabilities.Merge()
				assert.False(t, canMerge)
				assert.Nil(t, actual)
			}

			actual, canMerge := Capabilities(nil).Merge()
			assert.False(t, canMerge)
			assert.Nil(t, actual)
		})
	})
}

func TestGroupCapabilitiesFromDevices(t *testing.T) {
	on := MakeCapabilityByType(OnOffCapabilityType)
	on.SetRetrievable(true)
	on.SetState(OnOffCapabilityState{
		Instance: OnOnOffCapabilityInstance,
		Value:    true,
	})

	off := MakeCapabilityByType(OnOffCapabilityType)
	off.SetRetrievable(true)
	off.SetState(OnOffCapabilityState{
		Instance: OnOnOffCapabilityInstance,
		Value:    false,
	})

	richThermostatModes := MakeCapabilityByType(ModeCapabilityType)
	richThermostatModes.SetRetrievable(true)
	richThermostatModes.SetState(ModeCapabilityState{
		Instance: ThermostatModeInstance,
		Value:    CoolMode,
	})
	richThermostatModes.SetParameters(ModeCapabilityParameters{
		Instance: ThermostatModeInstance,
		Modes:    []Mode{{Value: AutoMode}, {Value: CoolMode}, {Value: HeatMode}, {Value: DryMode}},
	})

	poorThermostatModes := MakeCapabilityByType(ModeCapabilityType)
	poorThermostatModes.SetRetrievable(true)
	poorThermostatModes.SetState(ModeCapabilityState{
		Instance: ThermostatModeInstance,
		Value:    EcoMode,
	})
	poorThermostatModes.SetParameters(ModeCapabilityParameters{
		Instance: ThermostatModeInstance,
		Modes:    []Mode{{Value: AutoMode}, {Value: EcoMode}},
	})

	ecoThermostatModes := MakeCapabilityByType(ModeCapabilityType)
	ecoThermostatModes.SetRetrievable(true)
	ecoThermostatModes.SetState(ModeCapabilityState{
		Instance: ThermostatModeInstance,
		Value:    EcoMode,
	})
	ecoThermostatModes.SetParameters(ModeCapabilityParameters{
		Instance: ThermostatModeInstance,
		Modes:    []Mode{{Value: EcoMode}},
	})

	fanSpeedModes := MakeCapabilityByType(ModeCapabilityType)
	fanSpeedModes.SetRetrievable(true)
	fanSpeedModes.SetState(ModeCapabilityState{
		Instance: FanSpeedModeInstance,
		Value:    AutoMode,
	})
	fanSpeedModes.SetParameters(ModeCapabilityParameters{
		Instance: FanSpeedModeInstance,
		Modes:    []Mode{{Value: AutoMode}},
	})

	muteOnToggle := MakeCapabilityByType(ToggleCapabilityType)
	muteOnToggle.SetRetrievable(false)
	muteOnToggle.SetState(ToggleCapabilityState{
		Instance: MuteToggleCapabilityInstance,
		Value:    true,
	})
	muteOnToggle.SetParameters(ToggleCapabilityParameters{Instance: MuteToggleCapabilityInstance})

	muteOffToggle := MakeCapabilityByType(ToggleCapabilityType)
	muteOffToggle.SetRetrievable(false)
	muteOffToggle.SetState(ToggleCapabilityState{
		Instance: MuteToggleCapabilityInstance,
		Value:    false,
	})
	muteOffToggle.SetParameters(ToggleCapabilityParameters{Instance: MuteToggleCapabilityInstance})

	richColorSettings := MakeCapabilityByType(ColorSettingCapabilityType)
	richColorSettings.SetRetrievable(true)
	richColorSettings.SetState(ColorSettingCapabilityState{
		Instance: RgbColorCapabilityInstance,
		Value:    RGB(1000),
	})
	richColorSettings.SetParameters(ColorSettingCapabilityParameters{
		ColorModel: CM(RgbModelType),
		TemperatureK: &TemperatureKParameters{
			Min: TemperatureK(2700),
			Max: TemperatureK(6500),
		},
	})

	poorColorSettings := MakeCapabilityByType(ColorSettingCapabilityType)
	poorColorSettings.SetRetrievable(true)
	poorColorSettings.SetState(ColorSettingCapabilityState{
		Instance: TemperatureKCapabilityInstance,
		Value:    TemperatureK(4500),
	})
	poorColorSettings.SetParameters(ColorSettingCapabilityParameters{
		TemperatureK: &TemperatureKParameters{
			Min: TemperatureK(4500),
			Max: TemperatureK(4500),
		},
	})

	t.Run("Lamps", func(t *testing.T) {
		richLamp := Device{Capabilities: []ICapability{on, richColorSettings}}
		poorLamp := Device{Capabilities: []ICapability{off, poorColorSettings}}

		actual := GroupCapabilitiesFromDevices([]Device{richLamp, poorLamp})

		expectedOnOffCapability := MakeCapabilityByType(OnOffCapabilityType)
		expectedOnOffCapability.SetRetrievable(true)
		expectedOnOffCapability.SetState(OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true})

		expectedColorCapability := MakeCapabilityByType(ColorSettingCapabilityType)
		expectedColorCapability.SetRetrievable(true)
		expectedColorCapability.SetParameters(ColorSettingCapabilityParameters{
			ColorModel:   nil,
			TemperatureK: &TemperatureKParameters{Min: 4500, Max: 4500},
		})

		expected := CapabilitiesMap{
			on.Key():                expectedOnOffCapability,
			richColorSettings.Key(): expectedColorCapability,
		}
		assert.True(t, reflect.DeepEqual(expected, actual))

		actual = GroupCapabilitiesFromDevices([]Device{richLamp, richLamp})

		expectedColorCapability2 := MakeCapabilityByType(ColorSettingCapabilityType)
		expectedColorCapability2.SetRetrievable(true)
		expectedColorCapability2.SetState(ColorSettingCapabilityState{Instance: HsvColorCapabilityInstance, Value: HSV{H: 239, S: 100, V: 91}})
		expectedColorCapability2.SetParameters(ColorSettingCapabilityParameters{
			ColorModel:   CM(HsvModelType),
			TemperatureK: &TemperatureKParameters{Min: 2700, Max: 6500},
		})

		expected = CapabilitiesMap{
			on.Key():                on,
			richColorSettings.Key(): expectedColorCapability2,
		}
		assert.True(t, reflect.DeepEqual(expected, actual))
	})

	t.Run("TvSets", func(t *testing.T) {
		tv1 := Device{Capabilities: []ICapability{on, muteOffToggle}}
		tv2 := Device{Capabilities: []ICapability{on, muteOnToggle}}

		actual := GroupCapabilitiesFromDevices([]Device{tv1, tv2})

		expectedOnOffCapability := MakeCapabilityByType(OnOffCapabilityType)
		expectedOnOffCapability.SetRetrievable(true)
		expectedOnOffCapability.SetState(OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true})

		expectedToggleCapability := MakeCapabilityByType(ToggleCapabilityType)
		expectedToggleCapability.SetRetrievable(false)
		expectedToggleCapability.SetParameters(ToggleCapabilityParameters{
			Instance: MuteToggleCapabilityInstance,
		})

		expected := CapabilitiesMap{
			on.Key():           expectedOnOffCapability,
			muteOnToggle.Key(): expectedToggleCapability,
		}
		assert.True(t, reflect.DeepEqual(expected, actual))
	})

	t.Run("Thermostats", func(t *testing.T) {
		richThermostat := Device{Capabilities: []ICapability{on, richThermostatModes}}
		poorThermostat := Device{Capabilities: []ICapability{on, poorThermostatModes}}
		ecoThermostat := Device{Capabilities: []ICapability{on, ecoThermostatModes}}

		t.Run("ModesIntersection", func(t *testing.T) {
			actual := GroupCapabilitiesFromDevices([]Device{richThermostat, poorThermostat})
			expectedOnOffCapability := MakeCapabilityByType(OnOffCapabilityType)
			expectedOnOffCapability.SetRetrievable(true)
			expectedOnOffCapability.SetState(OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true})

			expectedModeCapability := MakeCapabilityByType(ModeCapabilityType)
			expectedModeCapability.SetRetrievable(true)
			expectedModeCapability.SetParameters(ModeCapabilityParameters{
				Instance: ThermostatModeInstance,
				Modes:    []Mode{KnownModes[AutoMode]},
			})

			expected := CapabilitiesMap{
				on.Key():                  expectedOnOffCapability,
				richThermostatModes.Key(): expectedModeCapability,
			}
			assert.True(t, reflect.DeepEqual(expected, actual))
		})
		t.Run("NoModesIntersection", func(t *testing.T) {
			actual := GroupCapabilitiesFromDevices([]Device{richThermostat, poorThermostat, ecoThermostat})

			expectedOnOffCapability := MakeCapabilityByType(OnOffCapabilityType)
			expectedOnOffCapability.SetRetrievable(true)
			expectedOnOffCapability.SetState(OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true})

			expected := CapabilitiesMap{
				on.Key(): expectedOnOffCapability,
			}
			assert.True(t, reflect.DeepEqual(expected, actual))
		})
	})

	t.Run("Fans", func(t *testing.T) {
		t.Run("FirstHasMute", func(t *testing.T) {
			fan1 := Device{Capabilities: []ICapability{on, fanSpeedModes, muteOnToggle}}
			fan2 := Device{Capabilities: []ICapability{on, fanSpeedModes, richThermostatModes}}

			actual := GroupCapabilitiesFromDevices([]Device{fan1, fan2})

			expectedOnOffCapability := MakeCapabilityByType(OnOffCapabilityType)
			expectedOnOffCapability.SetRetrievable(true)
			expectedOnOffCapability.SetState(OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true})

			expectedModeCapability := MakeCapabilityByType(ModeCapabilityType)
			expectedModeCapability.SetRetrievable(true)
			expectedModeCapability.SetParameters(ModeCapabilityParameters{
				Instance: FanSpeedModeInstance,
				Modes:    []Mode{KnownModes[AutoMode]},
			})
			expectedModeCapability.SetState(ModeCapabilityState{
				Instance: FanSpeedModeInstance,
				Value:    AutoMode,
			})

			expected := CapabilitiesMap{
				on.Key():            expectedOnOffCapability,
				fanSpeedModes.Key(): expectedModeCapability,
			}
			assert.True(t, reflect.DeepEqual(expected, actual))
		})
		t.Run("SomeHaveMute", func(t *testing.T) {
			fan1 := Device{Capabilities: []ICapability{on, fanSpeedModes, muteOnToggle}}
			fan2 := Device{Capabilities: []ICapability{on, fanSpeedModes, richThermostatModes}}
			fan3 := Device{Capabilities: []ICapability{on, fanSpeedModes, muteOnToggle}}

			actual := GroupCapabilitiesFromDevices([]Device{fan1, fan2, fan3})

			expectedOnOffCapability := MakeCapabilityByType(OnOffCapabilityType)
			expectedOnOffCapability.SetRetrievable(true)
			expectedOnOffCapability.SetState(OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true})

			expectedModeCapability := MakeCapabilityByType(ModeCapabilityType)
			expectedModeCapability.SetRetrievable(true)
			expectedModeCapability.SetParameters(ModeCapabilityParameters{
				Instance: FanSpeedModeInstance,
				Modes:    []Mode{KnownModes[AutoMode]},
			})
			expectedModeCapability.SetState(ModeCapabilityState{
				Instance: FanSpeedModeInstance,
				Value:    AutoMode,
			})

			expected := CapabilitiesMap{
				on.Key():            expectedOnOffCapability,
				fanSpeedModes.Key(): expectedModeCapability,
			}
			assert.True(t, reflect.DeepEqual(expected, actual))
		})
	})

	t.Run("Different", func(t *testing.T) {
		device1 := Device{Capabilities: []ICapability{on, fanSpeedModes, muteOnToggle}}
		device2 := Device{Capabilities: []ICapability{richThermostatModes}}
		expected := CapabilitiesMap{}
		assert.True(t, reflect.DeepEqual(expected, GroupCapabilitiesFromDevices([]Device{device1, device2})))
	})
}

func TestGroupCompatibleWithDevices(t *testing.T) {
	multiroomSpeakers := Devices{{Type: YandexStationDeviceType}, {Type: YandexStationMini2DeviceType}}
	notSameTypeDevices := Devices{{Type: LightDeviceType}, {Type: SocketDeviceType}}
	lightDevices := Devices{{Type: LightDeviceType}, {Type: LightDeviceType}}
	mixedSpeakers := Devices{{Type: DexpSmartBoxDeviceType}, {Type: YandexStationMini2DeviceType}}
	emptyDevices := Devices{}
	t.Run("no-type", func(t *testing.T) {
		group := Group{}
		assert.True(t, group.CompatibleWithDevices(multiroomSpeakers))
		assert.True(t, group.CompatibleWithDevices(lightDevices))
		assert.False(t, group.CompatibleWithDevices(notSameTypeDevices))
		assert.False(t, group.CompatibleWithDevices(mixedSpeakers))
		assert.True(t, group.CompatibleWithDevices(emptyDevices))
	})
	t.Run("multiroom", func(t *testing.T) {
		group := Group{Type: SmartSpeakerDeviceType}
		assert.True(t, group.CompatibleWithDevices(multiroomSpeakers))
		assert.False(t, group.CompatibleWithDevices(lightDevices))
		assert.False(t, group.CompatibleWithDevices(notSameTypeDevices))
		assert.False(t, group.CompatibleWithDevices(mixedSpeakers))
		assert.True(t, group.CompatibleWithDevices(emptyDevices))
	})
	t.Run("lightDevices", func(t *testing.T) {
		group := Group{Type: LightDeviceType}
		assert.False(t, group.CompatibleWithDevices(multiroomSpeakers))
		assert.True(t, group.CompatibleWithDevices(lightDevices))
		assert.False(t, group.CompatibleWithDevices(notSameTypeDevices))
		assert.False(t, group.CompatibleWithDevices(mixedSpeakers))
		assert.True(t, group.CompatibleWithDevices(emptyDevices))
	})
}
