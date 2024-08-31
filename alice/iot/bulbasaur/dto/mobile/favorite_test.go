package mobile

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/ptr"
)

func TestFavoriteListViewFrom(t *testing.T) {
	group := model.Group{
		ID:          "group-1",
		Name:        "Группа",
		Type:        model.LightDeviceType,
		HouseholdID: "household-1",
		Favorite:    true,
	}
	scenario := model.Scenario{
		ID:   "scenario-1",
		Name: "Я сценарист в нем я режиссер",
		Icon: model.ScenarioIconAlarm,
		Triggers: model.ScenarioTriggers{
			model.VoiceScenarioTrigger{Phrase: "Враг мой бойся меня"},
		},
		Steps: model.ScenarioSteps{
			model.MakeScenarioStepByType(model.ScenarioStepActionsType).
				WithParameters(
					model.ScenarioStepActionsParameters{
						Devices: model.ScenarioLaunchDevices{
							model.ScenarioLaunchDevice{
								ID:   "device-1",
								Name: "Устройство",
								Type: model.LightDeviceType,
								Capabilities: model.Capabilities{
									model.MakeCapabilityByType(model.OnOffCapabilityType).
										WithState(
											model.OnOffCapabilityState{
												Value:    true,
												Instance: model.OnOnOffCapabilityInstance,
											}),
								},
							},
						},
						RequestedSpeakerCapabilities: model.ScenarioCapabilities{
							{
								Type: model.QuasarServerActionCapabilityType,
								State: model.QuasarServerActionCapabilityState{
									Instance: model.PhraseActionCapabilityInstance,
									Value:    "фразочка",
								},
							},
							{
								Type: model.QuasarCapabilityType,
								State: model.QuasarCapabilityState{
									Instance: model.StopEverythingCapabilityInstance,
									Value:    model.StopEverythingQuasarCapabilityValue{},
								},
							},
						},
					}),
			model.MakeScenarioStepByType(model.ScenarioStepDelayType).
				WithParameters(model.ScenarioStepDelayParameters{DelayMs: 5000}),
		},
		Favorite: true,
		IsActive: true,
	}
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)

	humidity := model.MakePropertyByType(model.FloatPropertyType)
	humidity.SetParameters(model.FloatPropertyParameters{
		Instance: model.HumidityPropertyInstance,
		Unit:     model.UnitPercent,
	})
	humidity.SetState(model.FloatPropertyState{
		Instance: model.HumidityPropertyInstance,
		Value:    20.0,
	})

	humidityStateView := PropertyStateView{
		Type: model.FloatPropertyType,
		Parameters: FloatPropertyParameters{
			Instance:     model.HumidityPropertyInstance,
			InstanceName: model.KnownPropertyInstanceNames[model.HumidityPropertyInstance],
			Unit:         model.UnitPercent,
		},
		State: FloatPropertyState{
			Percent: ptr.Float64(20),
			Status:  model.PS(model.WarningStatus),
			Value:   20,
		},
	}

	motion := model.MakePropertyByType(model.EventPropertyType)
	motion.SetParameters(model.EventPropertyParameters{
		Instance: model.MotionPropertyInstance,
		Events: model.Events{
			model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
			model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
		},
	})

	motionStateView := PropertyStateView{
		Type: model.EventPropertyType,
		Parameters: EventPropertyParameters{
			Instance:     model.MotionPropertyInstance,
			InstanceName: model.KnownPropertyInstanceNames[model.MotionPropertyInstance],
			Events: model.Events{
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
				{
					Value: model.NotDetectedWithinMinute,
					Name:  ptr.String("нет движения последнюю минуту"),
				},
				{
					Value: model.NotDetectedWithin2Minutes,
					Name:  ptr.String("нет движения последние 2 минуты"),
				},
				{
					Value: model.NotDetectedWithin5Minutes,
					Name:  ptr.String("нет движения последние 5 минут"),
				},
				{
					Value: model.NotDetectedWithin10Minutes,
					Name:  ptr.String("нет движения последние 10 минут"),
				},
			},
		},
	}

	device := model.Device{
		ID:           "device-1",
		Name:         "Устройство",
		Capabilities: model.Capabilities{onOff},
		Properties:   model.Properties{humidity, motion},
		HouseholdID:  "household-1",
		SkillID:      model.XiaomiSkill,
		Type:         model.LightDeviceType,
		Groups:       model.Groups{group},
		Favorite:     true,
	}
	household := model.Household{
		ID:   "household-1",
		Name: "Дача",
	}

	userInfo := model.UserInfo{
		Devices:   model.Devices{device},
		Groups:    model.Groups{group},
		Scenarios: model.Scenarios{scenario},
		FavoriteRelations: model.FavoriteRelations{
			FavoriteDevicePropertyKeys: map[model.DevicePropertyKey]bool{
				{DeviceID: device.ID, PropertyKey: humidity.Key()}: true,
				{DeviceID: device.ID, PropertyKey: motion.Key()}:   true,
			},
		},
		Households:         model.Households{household},
		CurrentHouseholdID: household.ID,
	}
	expected := FavoriteListView{
		Properties: []FavoriteDevicePropertyListView{
			{
				DeviceID:      device.ID,
				Property:      motionStateView,
				HouseholdName: "Дача",
			},
			{
				DeviceID:      device.ID,
				Property:      humidityStateView,
				HouseholdName: "Дача",
			},
		},
		Items: []FavoriteListItemView{
			{
				Type: model.ScenarioFavoriteType,
				Parameters: ScenarioListView{
					ID:         "scenario-1",
					Name:       "Я сценарист в нем я режиссер",
					Icon:       string(model.ScenarioIconAlarm),
					IconURL:    model.ScenarioIconAlarm.URL(),
					Devices:    []string{"Любая колонка", "Устройство"},
					Executable: true,
					Triggers: []ScenarioTriggerEditView{
						VoiceScenarioTriggerEditView{
							Trigger: model.VoiceScenarioTrigger{Phrase: "Враг мой бойся меня"},
						},
					},
					IsActive: true,
				},
			},
			{
				Type: model.DeviceFavoriteType,
				Parameters: ItemInfoView{
					ID:           "device-1",
					Name:         "Устройство",
					Type:         model.LightDeviceType,
					IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
					Capabilities: model.Capabilities{onOff},
					Properties:   []PropertyStateView{humidityStateView, motionStateView},
					ItemType:     DeviceItemInfoViewType,
					SkillID:      model.XiaomiSkill,
					Unconfigured: true,
					Created:      formatTimestamp(0),
					GroupsIDs:    []string{group.ID},
					Parameters:   DeviceItemInfoViewParameters{},
				},
			},
			{
				Type: model.GroupFavoriteType,
				Parameters: ItemInfoView{
					ID:           "group-1",
					Name:         "Группа",
					Type:         model.LightDeviceType,
					IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
					Capabilities: model.Capabilities{onOff},
					Properties:   []PropertyStateView{},
					RoomNames:    []string{},
					State:        model.OnlineDeviceStatus,
					ItemType:     GroupItemInfoViewType,
					DevicesCount: 1,
					DevicesIDs:   []string{device.ID},
				},
			},
		},
		BackgroundImage: NewBackgroundImageView(model.FavoriteBackgroundImageID),
	}
	var favoritesView FavoriteListView
	favoritesView.From(context.Background(), userInfo)
	assert.Equal(t, expected, favoritesView)
}

func TestFavoritesProperties(t *testing.T) {
	household := model.Household{
		ID:   "household-1",
		Name: "Дача",
	}

	humidity := model.MakePropertyByType(model.FloatPropertyType)
	humidity.SetParameters(model.FloatPropertyParameters{
		Instance: model.HumidityPropertyInstance,
		Unit:     model.UnitPercent,
	})
	humidity.SetState(model.FloatPropertyState{
		Instance: model.HumidityPropertyInstance,
		Value:    20.0,
	})

	humidityStateView := PropertyStateView{
		Type: model.FloatPropertyType,
		Parameters: FloatPropertyParameters{
			Instance:     model.HumidityPropertyInstance,
			InstanceName: model.KnownPropertyInstanceNames[model.HumidityPropertyInstance],
			Unit:         model.UnitPercent,
		},
		State: FloatPropertyState{
			Percent: ptr.Float64(20),
			Status:  model.PS(model.WarningStatus),
			Value:   20,
		},
	}

	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)

	motion := model.MakePropertyByType(model.EventPropertyType)
	motion.SetParameters(model.EventPropertyParameters{
		Instance: model.MotionPropertyInstance,
		Events: model.Events{
			model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
			model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
		},
	})
	motionStateView := PropertyStateView{
		Type: model.EventPropertyType,
		Parameters: EventPropertyParameters{
			Instance:     model.MotionPropertyInstance,
			InstanceName: model.KnownPropertyInstanceNames[model.MotionPropertyInstance],
			Events: model.Events{
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
				{
					Value: model.NotDetectedWithinMinute,
					Name:  ptr.String("нет движения последнюю минуту"),
				},
				{
					Value: model.NotDetectedWithin2Minutes,
					Name:  ptr.String("нет движения последние 2 минуты"),
				},
				{
					Value: model.NotDetectedWithin5Minutes,
					Name:  ptr.String("нет движения последние 5 минут"),
				},
				{
					Value: model.NotDetectedWithin10Minutes,
					Name:  ptr.String("нет движения последние 10 минут"),
				},
			},
		},
	}
	device := model.Device{
		ID:           "device-1",
		Name:         "Устройство",
		Capabilities: model.Capabilities{onOff},
		Properties:   model.Properties{humidity, motion},
		HouseholdID:  "household-1",
		SkillID:      model.XiaomiSkill,
		Type:         model.LightDeviceType,
		Groups:       model.Groups{},
	}

	expected := HouseholdFavoritePropertiesAvailableView{
		HouseholdFavoriteView: HouseholdFavoriteView{
			ID:   household.ID,
			Name: household.Name,
		},
		Rooms: []RoomFavoritePropertiesAvailableView{},
		WithoutRoom: []FavoritePropertyAvailableView{
			{
				Property:   motionStateView,
				DeviceID:   device.ID,
				DeviceName: device.Name,
				IsSelected: false,
			},
			{
				Property:   humidityStateView,
				DeviceID:   device.ID,
				DeviceName: device.Name,
				IsSelected: false,
			},
		},
	}
	var actual HouseholdFavoritePropertiesAvailableView
	actual.From("", household, model.Devices{device}, model.Favorites{})
	assert.Equal(t, expected, actual)

	// check event property showing with the experiment
	expected.WithoutRoom = []FavoritePropertyAvailableView{
		{
			Property:   motionStateView,
			DeviceID:   device.ID,
			DeviceName: device.Name,
			IsSelected: false,
		},
		{
			Property:   humidityStateView,
			DeviceID:   device.ID,
			DeviceName: device.Name,
			IsSelected: false,
		},
	}
	actual = HouseholdFavoritePropertiesAvailableView{}
	actual.From("", household, model.Devices{device}, model.Favorites{})
	assert.Equal(t, expected, actual)
}

func TestFavoriteGroupAvailableView(t *testing.T) {
	group := model.Group{
		ID:          "group-1",
		Name:        "Люстра",
		Type:        model.LightDeviceType,
		Devices:     []string{"device-1", "device-2", "device-3"},
		HouseholdID: "household-1",
		Favorite:    false,
	}
	expected := FavoriteGroupAvailableView{
		ID:           "group-1",
		Name:         "Люстра",
		Type:         model.LightDeviceType,
		IsSelected:   false,
		DevicesCount: 3,
	}
	var actual FavoriteGroupAvailableView
	actual.FromGroup(group)
	assert.Equal(t, expected, actual)
}
