package mobile

import (
	"fmt"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/valid"
)

func TestSuggest(t *testing.T) {
	t.Run("roomSuggest", func(t *testing.T) {
		for _, suggest := range suggestions.RoomNames {
			room := model.Room{Name: suggest}
			assert.NoError(t, room.AssertName(), "Failed at \"%s\"", suggest)
		}
	})

	// FIXME: Group name has no checks
	t.Run("groupSuggest", func(t *testing.T) {})

	t.Run("deviceSuggest", func(t *testing.T) {
		for deviceType, suggests := range suggestions.DeviceNames {
			t.Run(fmt.Sprintf("type=%s", deviceType), func(t *testing.T) {
				for _, suggest := range suggests {
					device := model.Device{Name: suggest}
					assert.NoError(t, device.AssertName(), "Failed at \"%s\"", suggest)
				}
			})
		}
	})

	t.Run("deviceSuggestDefault", func(t *testing.T) {
		for _, suggest := range suggestions.DefaultDeviceNames {
			device := model.Device{Name: suggest}
			assert.NoError(t, device.AssertName(), "Failed at \"%s\"", suggest)
		}
	})

	t.Run("scenarioVoiceTriggerTypeSuggests", func(t *testing.T) {
		for _, suggest := range scenarioVoiceTriggerTypeSuggests {
			name := model.ScenarioName(suggest)
			vctx := valid.NewValidationCtx()
			_, err := name.Validate(vctx)
			assert.NoError(t, err, "Failed at \"%s\"", suggest)
		}
	})
}

func TestRenameSuggestsExist(t *testing.T) {
	for _, knownDeviceType := range model.KnownDeviceTypes {
		if model.DeviceType(knownDeviceType).IsSmartSpeakerOrModule() {
			continue
		}
		suggests, ok := suggestions.DeviceNames[model.DeviceType(knownDeviceType)]
		assert.Truef(t, ok, "Forgot to add device suggests for %s", knownDeviceType)
		assert.Truef(t, len(suggests) > 0, "Empty device suggests for %s", knownDeviceType)
		groupSuggests, ok := suggestions.GroupNames[model.DeviceType(knownDeviceType)]
		assert.Truef(t, ok, "Forgot to add group device suggests for %s", knownDeviceType)
		assert.Truef(t, len(groupSuggests) > 0, "Empty group device suggests for %s", knownDeviceType)
	}
}

func TestModeInstanceInflectionSuggestsExist(t *testing.T) {
	for knownModeInstance := range model.KnownModeInstancesNames {
		_, ok := modeInstanceInflections[model.ModeCapabilityInstance(knownModeInstance)]
		assert.Truef(t, ok, "Forgot to add inflection for suggestions for %s", knownModeInstance)
	}
}

func TestFavoriteTypesSortingMap(t *testing.T) {
	for _, knownFavoriteType := range model.KnownFavoriteTypes {
		_, exist := favoriteTypesSortingMap[model.FavoriteType(knownFavoriteType)]
		assert.True(t, exist, "forgot to add favorite type in sorting map: %s", knownFavoriteType)
	}
}

func TestSpeakerNewsTopicsNameMap(t *testing.T) {
	for _, knownTopic := range model.KnownSpeakerNewsTopics {
		_, exist := speakerNewsTopicsNameMap[model.SpeakerNewsTopic(knownTopic)]
		assert.True(t, exist, "forgot to add speaker news topic name: %s", knownTopic)
	}
}
