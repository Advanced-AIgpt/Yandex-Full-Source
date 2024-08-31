package common

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func TestExtendProperties(t *testing.T) {
	inputs := []struct {
		name                  string
		intentParametersSlice QueryIntentParametersSlice
		expected              QueryIntentParametersSlice
	}{
		{
			name: "no_air_quality",
			intentParametersSlice: QueryIntentParametersSlice{
				{
					Target: StateTarget,
				},
				{
					Target:           PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.CO2LevelPropertyInstance),
				},
				{
					Target:           CapabilityTarget,
					PropertyType:     string(model.RangeCapabilityType),
					PropertyInstance: string(model.TemperatureRangeInstance),
				},
			},
			expected: QueryIntentParametersSlice{
				{
					Target: StateTarget,
				},
				{
					Target:           PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.CO2LevelPropertyInstance),
				},
				{
					Target:           CapabilityTarget,
					PropertyType:     string(model.RangeCapabilityType),
					PropertyInstance: string(model.TemperatureRangeInstance),
				},
			},
		},
		{
			name: "tvoc",
			intentParametersSlice: QueryIntentParametersSlice{
				{
					Target: StateTarget,
				},
				{
					Target:           PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.TvocPropertyInstance),
				},
				{
					Target:           CapabilityTarget,
					PropertyType:     string(model.RangeCapabilityType),
					PropertyInstance: string(model.TemperatureRangeInstance),
				},
			},
			expected: QueryIntentParametersSlice{
				{
					Target: StateTarget,
				},
				{
					Target:           PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.TvocPropertyInstance),
				},
				{
					Target:           PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.PM1DensityPropertyInstance),
				},
				{
					Target:           PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.PM2p5DensityPropertyInstance),
				},
				{
					Target:           PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.PM10DensityPropertyInstance),
				},
				{
					Target:           CapabilityTarget,
					PropertyType:     string(model.RangeCapabilityType),
					PropertyInstance: string(model.TemperatureRangeInstance),
				},
			},
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			extendedSlice := input.intentParametersSlice.ExtendProperties()
			assert.ElementsMatch(t, input.expected, extendedSlice)
		})
	}
}
