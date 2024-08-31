package mobile

import (
	"encoding/json"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestUserStorageUpdateRequestUnmarshal(t *testing.T) {
	rawStorageConfigBytes := `
	{
		"bool-event": {
			"type": "storage.value.bool",
			"value": true
		},
		"string-event": {
			"type": "storage.value.string",
			"value": "string"
		},
		"float-event": {
			"type": "storage.value.float",
			"value": 1.0
		},
		"struct-event": {
			"type": "storage.value.struct",
			"value": { "kek": "lol" }
		}
	}
	`
	expectedConfig := UserStorageUpdateRequest{
		"bool-event": {
			Type:  model.BoolUserStorageValueType,
			Value: model.BoolUserStorageValue(true),
		},
		"string-event": {
			Type:  model.StringUserStorageValueType,
			Value: model.StringUserStorageValue("string"),
		},
		"float-event": {
			Type:  model.FloatUserStorageValueType,
			Value: model.FloatUserStorageValue(1.0),
		},
		"struct-event": {
			Type:  model.StructUserStorageValueType,
			Value: model.StructUserStorageValue(`{ "kek": "lol" }`),
		},
	}
	var config UserStorageUpdateRequest
	err := json.Unmarshal([]byte(rawStorageConfigBytes), &config)
	assert.NoError(t, err)
	assert.Equal(t, expectedConfig, config)
}

func TestUserStorageUpdateRequestUnmarshalEmpty(t *testing.T) {
	rawStorageConfigBytes := `{}`
	expectedConfig := make(UserStorageUpdateRequest)
	var config UserStorageUpdateRequest
	err := json.Unmarshal([]byte(rawStorageConfigBytes), &config)
	assert.NoError(t, err)
	assert.Equal(t, expectedConfig, config)
}
