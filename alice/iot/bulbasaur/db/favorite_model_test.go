package db

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestMakeFavorites(t *testing.T) {
	humidity := model.MakePropertyByType(model.FloatPropertyType)
	humidity.SetReportable(true)
	humidity.SetRetrievable(true)
	humidity.SetParameters(model.FloatPropertyParameters{
		Instance: model.HumidityPropertyInstance,
		Unit:     model.UnitPercent,
	})
	humidity.SetState(model.FloatPropertyState{
		Instance: model.HumidityPropertyInstance,
		Value:    50,
	})

	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)

	humidifier := model.Device{
		ID:   "device-1",
		Type: model.HumidifierDeviceType,
		Capabilities: model.Capabilities{
			onOff,
		},
		Properties: model.Properties{
			humidity,
		},
	}
	userInfo := model.UserInfo{
		Devices: model.Devices{humidifier},
		Scenarios: model.Scenarios{
			{
				ID:                           "scenario-1",
				Name:                         "Любимый сценарий",
				Devices:                      model.ScenarioDevices{},
				RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
			},
			{
				ID:                           "scenario-2",
				Name:                         "Нелюбимый сценарий",
				Devices:                      model.ScenarioDevices{},
				RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
			},
		},
		Groups: model.Groups{
			{
				ID:      "group-1",
				Name:    "Любимая группа",
				Aliases: []string{},
				Devices: []string{},
			},
			{
				ID:      "group-2",
				Name:    "Нелюбимая группа",
				Aliases: []string{},
				Devices: []string{},
			},
		},
	}
	rawFavorites := rawFavorites{
		{
			TargetID: "device-1",
			Type:     model.DeviceFavoriteType,
			Key:      "device-1",
		},
		{
			TargetID: "scenario-1",
			Type:     model.ScenarioFavoriteType,
			Key:      "scenario-1",
		},
		{
			TargetID: "group-1",
			Type:     model.GroupFavoriteType,
			Key:      "group-1",
		},
		{
			TargetID: "device-1",
			Type:     model.DevicePropertyFavoriteType,
			Key:      humidity.Key(),
			Parameters: devicePropertyRawFavoriteParameters{
				Type:     model.FloatPropertyType,
				Instance: model.HumidityPropertyInstance,
			},
		},
	}

	expected := model.Favorites{
		Groups: model.Groups{
			{
				ID:      "group-1",
				Name:    "Любимая группа",
				Aliases: []string{},
				Devices: []string{},
			},
		},
		Scenarios: model.Scenarios{
			{
				ID:                           "scenario-1",
				Name:                         "Любимый сценарий",
				Devices:                      model.ScenarioDevices{},
				RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
				Triggers:                     model.ScenarioTriggers{},
			},
		},
		Devices: model.Devices{
			humidifier,
		},
		Properties: []model.FavoritesDeviceProperty{
			{
				DeviceID: humidifier.ID,
				Property: humidity.Clone(),
			},
		},
	}
	actual, err := rawFavorites.MakeFavorites(userInfo)
	assert.NoError(t, err)
	assert.Equal(t, expected, actual)
}
