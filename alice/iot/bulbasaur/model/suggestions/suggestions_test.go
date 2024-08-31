package suggestions_test

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	"github.com/stretchr/testify/assert"
)

func TestGroupNameSuggests(t *testing.T) {
	t.Run("LightDeviceType", func(t *testing.T) {
		expectedSuggestions := suggestions.GroupNames[model.LightDeviceType]
		actualSuggestions := suggestions.GroupNameSuggests(model.LightDeviceType)
		assert.Equal(t, expectedSuggestions, actualSuggestions)
	})
	t.Run("YandexStationMini2DeviceType", func(t *testing.T) {
		expectedSuggestions := suggestions.MultiroomGroupNames
		actualSuggestions := suggestions.GroupNameSuggests(model.YandexStationMini2DeviceType)
		assert.Equal(t, expectedSuggestions, actualSuggestions)
	})
	t.Run("SmartSpeakerDeviceType", func(t *testing.T) {
		expectedSuggestions := suggestions.MultiroomGroupNames
		actualSuggestions := suggestions.GroupNameSuggests(model.SmartSpeakerDeviceType)
		assert.Equal(t, expectedSuggestions, actualSuggestions)
	})
	t.Run("DexpSmartBoxDeviceType", func(t *testing.T) {
		expectedSuggestions := suggestions.DefaultGroupNames
		actualSuggestions := suggestions.GroupNameSuggests(model.DexpSmartBoxDeviceType)
		assert.Equal(t, expectedSuggestions, actualSuggestions)
	})
}
