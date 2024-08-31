package widget

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestUserScenariosViewFromScenarios(t *testing.T) {
	testScenarios := model.Scenarios{
		{
			ID:   "1",
			Name: "Неактивная вечеринка",
			Icon: model.ScenarioIconAlarm,
			Devices: model.ScenarioDevices{
				{
					ID: "lamp",
					Capabilities: model.ScenarioCapabilities{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
			},
			IsActive: false,
		},
		{
			ID:   "2",
			Name: "Норм вечеринка",
			Icon: model.ScenarioIconAlarm,
			Devices: model.ScenarioDevices{
				{
					ID: "lamp",
					Capabilities: model.ScenarioCapabilities{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
			},
			IsActive: true,
		},
		{
			ID:       "3",
			Name:     "Незапускаемая вечеринка",
			Icon:     model.ScenarioIconAlarm,
			Devices:  model.ScenarioDevices{},
			IsActive: true,
		},
	}
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	testDevices := model.Devices{
		{
			ID:           "lamp",
			Capabilities: model.Capabilities{onOff},
		},
	}
	usv := UserScenariosView{
		Status:    "ok",
		RequestID: "1",
	}
	usv.FromScenarios(testScenarios, testDevices)
	expected := UserScenariosView{
		Status:    "ok",
		RequestID: "1",
		Scenarios: []ScenarioListView{
			{
				ID:      "2",
				Name:    "Норм вечеринка",
				IconURL: model.ScenarioIconAlarm.URL(),
			},
		},
	}
	assert.Equal(t, expected, usv)
}
