package model

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/library/go/jsonmatcher"
)

func TestProperty_MarshalJSON(t *testing.T) {
	property := MakePropertyByType(FloatPropertyType)
	property.SetReportable(true)
	property.SetRetrievable(true)
	property.SetParameters(FloatPropertyParameters{
		Instance: AmperagePropertyInstance,
		Unit:     UnitPercent,
	})
	property.SetState(FloatPropertyState{
		Instance: AmperagePropertyInstance,
		Value:    322.223,
	})
	property.SetLastUpdated(2086)

	actualBytes, err := property.MarshalJSON()
	assert.NoError(t, err)

	expectedBytes := `
		{
			"type": "devices.properties.float",
			"reportable": true,
			"retrievable": true,
			"parameters": {
				"instance": "amperage",
				"unit": "unit.percent"
			},
			"state": {
				"instance": "amperage",
				"value": 322.223
			},
			"last_updated": 2086
		}
	`
	assert.NoError(t, jsonmatcher.JSONContentsMatch(expectedBytes, string(actualBytes)))
}

func TestProperty_UnmarshalJSON(t *testing.T) {
	// without virtual events
	bytes := `
		{
			"type": "devices.properties.float",
			"reportable": true,
			"retrievable": true,
			"parameters": {
				"instance": "water_level",
				"unit": "unit.percent"
			},
			"state": {
				"instance": "water_level",
				"value": 32.2
			},
			"last_updated": 2086
		}
	`
	expectedProperty := MakePropertyByType(FloatPropertyType)
	expectedProperty.SetReportable(true)
	expectedProperty.SetRetrievable(true)
	expectedProperty.SetParameters(FloatPropertyParameters{
		Instance: WaterLevelPropertyInstance,
		Unit:     UnitPercent,
	})
	expectedProperty.SetState(FloatPropertyState{
		Instance: WaterLevelPropertyInstance,
		Value:    32.2,
	})
	expectedProperty.SetLastUpdated(2086)

	actualProperty, err := JSONUnmarshalProperty([]byte(bytes))
	assert.NoError(t, err)
	assert.Equal(t, expectedProperty, actualProperty)
}

func TestProperty_Clone(t *testing.T) {
	expectedProperty := MakePropertyByType(FloatPropertyType)
	expectedProperty.SetParameters(FloatPropertyParameters{
		Instance: WaterLevelPropertyInstance,
		Unit:     UnitPercent,
	})
	expectedProperty.SetState(FloatPropertyState{
		Instance: WaterLevelPropertyInstance,
		Value:    32.2,
	})
	expectedProperty.SetLastUpdated(2086)

	actualProperty := expectedProperty.Clone()
	assert.Equal(t, expectedProperty, actualProperty)
	actualProperty.SetState(FloatPropertyState{
		Instance: WaterLevelPropertyInstance,
		Value:    666.666,
	})
	assert.NotEqual(t, expectedProperty, actualProperty)
}
