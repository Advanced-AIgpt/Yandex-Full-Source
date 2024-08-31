package model

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestUserStorageConfigMerge(t *testing.T) {
	closedStoriesTooltip := UserStorageValue{
		Type:    BoolUserStorageValueType,
		Created: 1,
		Updated: 2,
		Value:   BoolUserStorageValue(true),
	}
	householdsTooltip := UserStorageValue{
		Type:    BoolUserStorageValueType,
		Created: 1,
		Updated: 1,
		Value:   BoolUserStorageValue(true),
	}
	closedStoriesTooltipOlder := UserStorageValue{
		Type:    BoolUserStorageValueType,
		Created: 1,
		Updated: 1,
		Value:   BoolUserStorageValue(true),
	}
	appStoryTS := UserStorageValue{
		Type:    FloatUserStorageValueType,
		Created: 1,
		Updated: 1,
		Value:   FloatUserStorageValue(1.0),
	}
	somethingTooltip := UserStorageValue{
		Type:    StringUserStorageValueType,
		Created: 2,
		Updated: 2,
		Value:   StringUserStorageValue("something"),
	}
	structValue := UserStorageValue{
		Type:    StructUserStorageValueType,
		Created: 3,
		Updated: 3,
		Value:   StructUserStorageValue(`{ "kek": "lol" }`),
	}
	aConfig := UserStorageConfig{
		"closed-stories": closedStoriesTooltipOlder,
		"households":     householdsTooltip,
		"app-story":      appStoryTS,
		"something":      somethingTooltip,
		"struct":         structValue,
	}
	bConfig := UserStorageConfig{
		"closed-stories": closedStoriesTooltip,
		"households":     householdsTooltip,
	}
	result, err := aConfig.MergeConfig(bConfig)
	assert.NoError(t, err)
	expected := UserStorageConfig{
		"closed-stories": closedStoriesTooltip,
		"households":     householdsTooltip,
		"app-story":      appStoryTS,
		"something":      somethingTooltip,
		"struct":         structValue,
	}
	assert.Equal(t, expected, result)

	result, err = bConfig.MergeConfig(aConfig)
	assert.NoError(t, err)
	assert.Equal(t, expected, result)
}

func TestUserStorageConfigUnmarshal(t *testing.T) {
	rawStorageConfigBytes := `
	{
		"bool-event": {
			"type": "storage.value.bool",
			"created": 1.0,
			"updated": 1.0,
			"value": true
		},
		"string-event": {
			"type": "storage.value.string",
			"created": 1.0,
			"updated": 1.0,
			"value": "string"
		},
		"float-event": {
			"type": "storage.value.float",
			"created": 1.0,
			"updated": 1.0,
			"value": 1.0
		},
		"struct-event": {
			"type": "storage.value.struct",
			"created": 3.0,
			"updated": 3.0,
			"value": { "kek": "lol" }
		}
	}
	`
	expectedConfig := UserStorageConfig{
		"bool-event": UserStorageValue{
			Type:    BoolUserStorageValueType,
			Created: 1,
			Updated: 1,
			Value:   BoolUserStorageValue(true),
		},
		"string-event": UserStorageValue{
			Type:    StringUserStorageValueType,
			Created: 1,
			Updated: 1,
			Value:   StringUserStorageValue("string"),
		},
		"float-event": UserStorageValue{
			Type:    FloatUserStorageValueType,
			Created: 1,
			Updated: 1,
			Value:   FloatUserStorageValue(1.0),
		},
		"struct-event": UserStorageValue{
			Type:    StructUserStorageValueType,
			Created: 3,
			Updated: 3,
			Value:   StructUserStorageValue(`{ "kek": "lol" }`),
		},
	}
	var config UserStorageConfig
	err := json.Unmarshal([]byte(rawStorageConfigBytes), &config)
	assert.NoError(t, err)
	assert.Equal(t, expectedConfig, config)
}
