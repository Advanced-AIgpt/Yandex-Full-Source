package model

import (
	"encoding/json"
	"sort"
	"testing"

	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/sorting"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid"
)

func TestRangeCapabilityState_Validate(t *testing.T) {
	t.Run("brightness", func(t *testing.T) {
		capability := MakeCapabilityByType(RangeCapabilityType)
		capability.SetParameters(RangeCapabilityParameters{
			Instance: BrightnessRangeInstance,
			Range: &Range{
				Min:       1,
				Max:       100,
				Precision: 1,
			},
		})

		valid := RangeCapabilityState{
			Instance: BrightnessRangeInstance,
			Value:    35,
		}

		invalidValue := RangeCapabilityState{
			Instance: BrightnessRangeInstance,
			Value:    0,
		}

		invalidValue2 := RangeCapabilityState{
			Instance: BrightnessRangeInstance,
			Value:    999,
		}

		invalidInstance := RangeCapabilityState{
			Instance: RangeCapabilityInstance("monkey_tail"),
			Value:    35,
		}

		actualDefaultValue := capability.DefaultState()
		expectedDefaultValue := RangeCapabilityState{
			Instance: BrightnessRangeInstance,
			Value:    100,
		}

		assert.NoError(t, valid.ValidateState(capability))
		assert.NoError(t, actualDefaultValue.ValidateState(capability))

		assert.Equal(t, expectedDefaultValue, actualDefaultValue)

		assert.EqualError(t, invalidValue.ValidateState(capability), "range brightness instance state value is out of supported range: '0.000000'")
		assert.EqualError(t, invalidValue2.ValidateState(capability), "range brightness instance state value is out of supported range: '999.000000'")
		assert.EqualError(t, invalidInstance.ValidateState(capability), "unsupported by current device range state instance: 'monkey_tail'")
	})

	t.Run("humidity", func(t *testing.T) {
		capability := MakeCapabilityByType(RangeCapabilityType)
		capability.SetParameters(RangeCapabilityParameters{
			Instance: HumidityRangeInstance,
			Range: &Range{
				Min:       1,
				Max:       100,
				Precision: 1,
			},
		})

		valid := RangeCapabilityState{
			Instance: HumidityRangeInstance,
			Value:    35,
		}

		invalidValue := RangeCapabilityState{
			Instance: HumidityRangeInstance,
			Value:    -1,
		}

		invalidValue2 := RangeCapabilityState{
			Instance: HumidityRangeInstance,
			Value:    999,
		}

		invalidInstance := RangeCapabilityState{
			Instance: RangeCapabilityInstance("monkey_tail"),
			Value:    35,
		}

		actualDefaultValue := capability.DefaultState()
		expectedDefaultValue := RangeCapabilityState{
			Instance: HumidityRangeInstance,
			Value:    1,
		}

		assert.NoError(t, valid.ValidateState(capability))
		assert.NoError(t, actualDefaultValue.ValidateState(capability))

		assert.Equal(t, expectedDefaultValue, actualDefaultValue)

		assert.EqualError(t, invalidValue.ValidateState(capability), "range humidity instance state value is out of supported range: '-1.000000'")
		assert.EqualError(t, invalidValue2.ValidateState(capability), "range humidity instance state value is out of supported range: '999.000000'")
		assert.EqualError(t, invalidInstance.ValidateState(capability), "unsupported by current device range state instance: 'monkey_tail'")
	})
}

func TestRangeCapabilityParameters_UnmarshalJSON(t *testing.T) {
	t.Run("WithPrecision", func(t *testing.T) {
		withPrecisionData := `
			{
				"instance": "brightness",
				"unit": "unit.percent",
				"range": {
					"min": 0,
					"max": 100,
					"precision": 10
				}
			}
		`
		expectedParameters := RangeCapabilityParameters{
			Instance:     BrightnessRangeInstance,
			Unit:         UnitPercent,
			RandomAccess: true,
			Looped:       false,
			Range: &Range{
				Min:       0,
				Max:       100,
				Precision: 10,
			},
		}
		var actualParameters RangeCapabilityParameters
		if err := json.Unmarshal([]byte(withPrecisionData), &actualParameters); err != nil {
			assert.NoError(t, err)
		}
		assert.Equal(t, expectedParameters, actualParameters)

		vctx := valid.NewValidationCtx()
		_, err := actualParameters.Validate(vctx)
		assert.NoError(t, err)
	})
	t.Run("WithoutPrecision", func(t *testing.T) {
		withoutPrecisionData := `
			{
				"instance": "brightness",
				"unit": "unit.percent",
				"range": {
					"min": 0,
					"max": 100
				}
			}
		`
		expectedParameters := RangeCapabilityParameters{
			Instance:     BrightnessRangeInstance,
			Unit:         UnitPercent,
			RandomAccess: true,
			Looped:       false,
			Range: &Range{
				Min:       0,
				Max:       100,
				Precision: 1,
			},
		}
		var actualParameters RangeCapabilityParameters
		if err := json.Unmarshal([]byte(withoutPrecisionData), &actualParameters); err != nil {
			assert.NoError(t, err)
		}
		assert.Equal(t, expectedParameters, actualParameters)

		vctx := valid.NewValidationCtx()
		_, err := actualParameters.Validate(vctx)
		assert.NoError(t, err)
	})
	t.Run("NegativeTemperature", func(t *testing.T) {
		negativeTemperatureData := `
			{
				"instance": "temperature",
				"unit": "unit.temperature.celsius",
				"range": {
					"min": -24.0,
					"max": -16.0
				}
			}
		`
		expectedParameters := RangeCapabilityParameters{
			Instance:     TemperatureRangeInstance,
			Unit:         UnitTemperatureCelsius,
			RandomAccess: true,
			Looped:       false,
			Range: &Range{
				Min:       -24.0,
				Max:       -16.0,
				Precision: 1.0,
			},
		}
		var actualParameters RangeCapabilityParameters
		if err := json.Unmarshal([]byte(negativeTemperatureData), &actualParameters); err != nil {
			assert.NoError(t, err)
		}
		assert.Equal(t, expectedParameters, actualParameters)

		vctx := valid.NewValidationCtx()
		_, err := actualParameters.Validate(vctx)
		assert.NoError(t, err)
	})
	t.Run("NegativeTemperatureLargePrecision", func(t *testing.T) {
		negativeTemperatureData := `
			{
				"instance": "temperature",
				"unit": "unit.temperature.celsius",
				"range": {
					"min": -24.0,
					"max": -16,
					"precision": 50.0
				}
			}
		`
		expectedParameters := RangeCapabilityParameters{
			Instance:     TemperatureRangeInstance,
			Unit:         UnitTemperatureCelsius,
			RandomAccess: true,
			Looped:       false,
			Range: &Range{
				Min:       -24.0,
				Max:       -16.0,
				Precision: 50.0,
			},
		}
		var actualParameters RangeCapabilityParameters
		if err := json.Unmarshal([]byte(negativeTemperatureData), &actualParameters); err != nil {
			assert.NoError(t, err)
		}
		assert.Equal(t, expectedParameters, actualParameters)

		vctx := valid.NewValidationCtx()
		_, err := actualParameters.Validate(vctx)
		assert.Error(t, err)
	})

	t.Run("VolumeWithPercent", func(t *testing.T) {
		volumePercentData := `
			{
				"instance": "volume",
				"unit": "unit.percent",
				"range": {
					"min": 0,
					"max": 100
				}
			}
		`
		expectedParameters := RangeCapabilityParameters{
			Instance:     VolumeRangeInstance,
			Unit:         UnitPercent,
			RandomAccess: true,
			Looped:       false,
			Range: &Range{
				Min:       0,
				Max:       100,
				Precision: 1.0,
			},
		}
		var actualParameters RangeCapabilityParameters
		if err := json.Unmarshal([]byte(volumePercentData), &actualParameters); err != nil {
			assert.NoError(t, err)
		}
		assert.Equal(t, expectedParameters, actualParameters)

		vctx := valid.NewValidationCtx()
		_, err := actualParameters.Validate(vctx)
		assert.NoError(t, err)
	})
}

func TestRangeCapabilityBasicSuggestions(t *testing.T) {
	// check for kettle and thermostat
	temperature := MakeCapabilityByType(RangeCapabilityType)
	temperature.SetParameters(RangeCapabilityParameters{
		Instance:     TemperatureRangeInstance,
		Unit:         UnitTemperatureCelsius,
		RandomAccess: true,
		Looped:       false,
	})
	kettleInflection := inflector.Inflection{
		Im:   "чайник",
		Rod:  "чайника",
		Dat:  "чайнику",
		Vin:  "чайник",
		Tvor: "чайником",
		Pr:   "чайнике",
	}
	type testCase struct {
		dt         DeviceType
		inflection inflector.Inflection
		options    SuggestionsOptions
		expected   []string
	}
	testCases := []testCase{
		{
			dt:         KettleDeviceType,
			inflection: kettleInflection,
			expected:   []string{},
		},
		{
			dt:         ThermostatDeviceType,
			inflection: kettleInflection,
			expected:   []string{"Сделай прохладнее"},
		},
	}
	for _, tc := range testCases {
		suggests := temperature.BasicSuggestions(tc.dt, tc.inflection, tc.options)
		sort.Sort(sorting.CaseInsensitiveStringsSorting(suggests))
		sort.Sort(sorting.CaseInsensitiveStringsSorting(tc.expected))
		assert.Equal(t, tc.expected, suggests)
	}
}
