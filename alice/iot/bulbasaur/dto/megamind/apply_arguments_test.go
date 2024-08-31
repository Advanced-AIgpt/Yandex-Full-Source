package megamind

import (
	"math"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/ptr"
)

func TestApplyArguments(t *testing.T) {
	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)
	onOffCapability.SetLastUpdated(timestamp.Now())
	onOffCapability.SetParameters(model.OnOffCapabilityParameters{Split: true})
	onOffCapability.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	colorCapabilityOne := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorCapabilityOne.SetRetrievable(true)
	colorCapabilityOne.SetLastUpdated(timestamp.Now())
	colorCapabilityOne.SetParameters(
		model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.RgbModelType),
		})
	colorCapabilityOne.SetState(model.ColorSettingCapabilityState{
		Instance: model.RgbColorCapabilityInstance,
		Value:    model.ColorPalette[model.ColorIDRed].ValueRGB,
	})

	colorCapabilityTwo := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorCapabilityTwo.SetRetrievable(true)
	colorCapabilityTwo.SetLastUpdated(timestamp.Now())
	colorCapabilityTwo.SetParameters(
		model.ColorSettingCapabilityParameters{
			TemperatureK: &model.TemperatureKParameters{
				Min: 0,
				Max: math.MaxInt32,
			},
		})
	colorCapabilityTwo.SetState(model.ColorSettingCapabilityState{
		Instance: model.TemperatureKCapabilityInstance,
		Value:    model.ColorPalette[model.ColorIDColdWhite].Temperature,
	})

	colorCapabilityThree := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorCapabilityThree.SetRetrievable(true)
	colorCapabilityThree.SetLastUpdated(timestamp.Now())
	colorCapabilityThree.SetParameters(
		model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.HsvModelType),
			TemperatureK: &model.TemperatureKParameters{
				Min: 0,
				Max: math.MaxInt32,
			},
		})
	colorCapabilityThree.SetState(model.ColorSettingCapabilityState{
		Instance: model.HsvColorCapabilityInstance,
		Value:    model.ColorPalette[model.ColorIDRed].ValueHSV,
	})

	modeCapabilityOne := model.MakeCapabilityByType(model.ModeCapabilityType)
	modeCapabilityOne.SetRetrievable(true)
	modeCapabilityOne.SetLastUpdated(timestamp.Now())
	modeCapabilityOne.SetParameters(
		model.ModeCapabilityParameters{
			Instance: model.ThermostatModeInstance,
			Modes: []model.Mode{
				{
					Value: model.AutoMode,
					Name:  tools.AOS("Авто"),
				},
				{
					Value: model.DryMode,
					Name:  tools.AOS("Сухо"),
				},
				{
					Value: model.EcoMode,
					Name:  tools.AOS("Эко"),
				},
			},
		})
	modeCapabilityOne.SetState(model.ModeCapabilityState{
		Instance: model.ThermostatModeInstance,
		Value:    model.DryMode,
	})

	modeCapabilityTwo := model.MakeCapabilityByType(model.ModeCapabilityType)
	modeCapabilityTwo.SetRetrievable(true)
	modeCapabilityTwo.SetLastUpdated(timestamp.Now())
	modeCapabilityTwo.SetParameters(
		model.ModeCapabilityParameters{
			Instance: model.ThermostatModeInstance,
			Modes: []model.Mode{
				{
					Value: model.AutoMode,
					Name:  tools.AOS("Авто"),
				},
				{
					Value: model.HeatMode,
					Name:  tools.AOS("Жарко"),
				},
				{
					Value: model.CoolMode,
					Name:  tools.AOS("Холодно"),
				},
			},
		})
	modeCapabilityTwo.SetState(nil) // explicitly

	rangeCapabilityOne := model.MakeCapabilityByType(model.RangeCapabilityType)
	rangeCapabilityOne.SetRetrievable(true)
	rangeCapabilityOne.SetLastUpdated(timestamp.Now())
	rangeCapabilityOne.SetParameters(
		model.RangeCapabilityParameters{
			Instance:     model.ChannelRangeInstance,
			Unit:         "",
			RandomAccess: true,
			Looped:       true,
			Range:        nil,
		})
	rangeCapabilityOne.SetState(model.RangeCapabilityState{
		Instance: model.ChannelRangeInstance,
		Relative: nil,
		Value:    11.5,
	})

	rangeCapabilityTwo := model.MakeCapabilityByType(model.RangeCapabilityType)
	rangeCapabilityTwo.SetRetrievable(true)
	rangeCapabilityTwo.SetLastUpdated(timestamp.Now())
	rangeCapabilityTwo.SetParameters(
		model.RangeCapabilityParameters{
			Instance: model.BrightnessRangeInstance,
			Unit:     model.UnitAmpere,
			Range: &model.Range{
				Min:       math.SmallestNonzeroFloat64,
				Max:       math.MaxFloat64,
				Precision: 1,
			},
		})
	rangeCapabilityTwo.SetState(nil) // explicitly

	rangeCapabilityThree := model.MakeCapabilityByType(model.RangeCapabilityType)
	rangeCapabilityThree.SetRetrievable(true)
	rangeCapabilityThree.SetLastUpdated(timestamp.Now())
	rangeCapabilityThree.SetParameters(
		model.RangeCapabilityParameters{
			Instance: model.ChannelRangeInstance,
			Range:    nil,
		})
	rangeCapabilityThree.SetState(model.RangeCapabilityState{
		Instance: model.ChannelRangeInstance,
		Relative: ptr.Bool(true),
		Value:    11.5,
	})

	toggleCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	toggleCapability.SetRetrievable(true)
	toggleCapability.SetLastUpdated(timestamp.Now())
	toggleCapability.SetParameters(
		model.ToggleCapabilityParameters{
			Instance: model.PauseToggleCapabilityInstance,
		})
	toggleCapability.SetState(model.ToggleCapabilityState{
		Instance: model.PauseToggleCapabilityInstance,
		Value:    true,
	})

	customButtonCapability := model.MakeCapabilityByType(model.CustomButtonCapabilityType)
	customButtonCapability.SetRetrievable(true)
	customButtonCapability.SetLastUpdated(timestamp.Now())
	customButtonCapability.SetParameters(
		model.CustomButtonCapabilityParameters{
			Instance:      "cb_2",
			InstanceNames: []string{"Мои маленькие пони"},
		})
	customButtonCapability.SetState(model.CustomButtonCapabilityState{
		Instance: "cb_2",
		Value:    true,
	})

	phraseServerActionCapability := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
	phraseServerActionCapability.SetRetrievable(false)
	phraseServerActionCapability.SetLastUpdated(timestamp.Now())
	phraseServerActionCapability.SetParameters(
		model.QuasarServerActionCapabilityParameters{
			Instance: model.PhraseActionCapabilityInstance,
		},
	)
	phraseServerActionCapability.SetState(
		model.QuasarServerActionCapabilityState{
			Instance: model.PhraseActionCapabilityInstance,
			Value:    "эхэхэх",
		})

	textServerActionCapability := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
	textServerActionCapability.SetRetrievable(false)
	textServerActionCapability.SetLastUpdated(timestamp.Now())
	textServerActionCapability.SetParameters(
		model.QuasarServerActionCapabilityParameters{
			Instance: model.TextActionCapabilityInstance,
		},
	)
	textServerActionCapability.SetState(
		model.QuasarServerActionCapabilityState{
			Instance: model.TextActionCapabilityInstance,
			Value:    "эхэхэх",
		})

	stopEverything := model.MakeCapabilityByType(model.QuasarCapabilityType)
	stopEverything.SetRetrievable(false)
	stopEverything.SetLastUpdated(timestamp.Now())
	stopEverything.SetParameters(model.QuasarCapabilityParameters{Instance: model.StopEverythingCapabilityInstance})

	temperatureFloatProperty := model.MakePropertyByType(model.FloatPropertyType)
	temperatureFloatProperty.SetRetrievable(true)
	temperatureFloatProperty.SetLastUpdated(timestamp.Now())
	temperatureFloatProperty.SetParameters(
		model.FloatPropertyParameters{
			Instance: model.TemperaturePropertyInstance,
			Unit:     model.UnitTemperatureCelsius,
		})
	temperatureFloatProperty.SetState(model.FloatPropertyState{
		Instance: model.TemperaturePropertyInstance,
		Value:    -273.15,
	})

	groupOne := model.Group{
		ID:      "group-id-1",
		Name:    "Lustra",
		Devices: []string{"device-1", "device-2"},
		Type:    model.LightDeviceType,
	}
	roomOne := model.Room{
		ID:      "my-room-1",
		Name:    "Bedroom",
		Devices: []string{"device-1"},
	}
	roomTwo := model.Room{
		ID:      "my-room-2",
		Name:    "Kitchen",
		Devices: []string{"device-2"},
	}
	deviceInfo := model.DeviceInfo{
		Manufacturer: ptr.String("manufacturer"),
		Model:        ptr.String("model"),
		HwVersion:    ptr.String("hw_version"),
		SwVersion:    ptr.String("sw_version"),
	}
	devices := []model.Device{
		{
			ID:           "device-1",
			ExternalID:   "device-1",
			Name:         "My lamp",
			ExternalName: "My lamp",
			SkillID:      "my_super_skill",
			Description:  ptr.String("this is the description of My Lamp"),
			Room:         &roomOne,
			Groups:       []model.Group{groupOne},
			Type:         model.LightDeviceType,
			OriginalType: model.LightDeviceType,
			Capabilities: []model.ICapability{onOffCapability, colorCapabilityOne, customButtonCapability},
			Updated:      timestamp.Now(),
		},
		{
			ID:           "device-2",
			Name:         "My lamp 2",
			DeviceInfo:   &deviceInfo,
			Room:         &roomTwo,
			Groups:       []model.Group{groupOne},
			Type:         model.LightDeviceType,
			OriginalType: model.LightDeviceType,
			Capabilities: []model.ICapability{onOffCapability, colorCapabilityTwo, toggleCapability},
			Updated:      timestamp.Now(),
		},
		{
			ID:           "device-3",
			Name:         "My thermostat 1",
			Type:         model.OtherDeviceType,
			OriginalType: model.OtherDeviceType,
			Capabilities: []model.ICapability{onOffCapability, colorCapabilityThree, modeCapabilityOne},
			CustomData: map[string]interface{}{ // that way we got decoded JSON where we're not quite sure of the struct initially
				"string": "this is simple custom data",
				"float":  11.7,
				"bool":   true,
			},
		},
		{
			ID:           "device-4",
			Name:         "My thermostat 2",
			Type:         model.ThermostatDeviceType,
			OriginalType: model.ThermostatDeviceType,
			Capabilities: []model.ICapability{onOffCapability, modeCapabilityTwo, rangeCapabilityOne},
			Properties:   []model.IProperty{temperatureFloatProperty},
			Updated:      timestamp.Now(),
		},
		{
			ID:           "device-5",
			Name:         "My thermostat with brightness",
			Type:         model.OtherDeviceType,
			OriginalType: model.ThermostatDeviceType,
			Capabilities: []model.ICapability{modeCapabilityTwo, rangeCapabilityTwo, rangeCapabilityThree},
			Updated:      timestamp.Now(),
		},
		{
			ID:           "device-6",
			Name:         "My speaker with server action",
			Type:         model.YandexStationDeviceType,
			OriginalType: model.YandexStationDeviceType,
			Capabilities: []model.ICapability{phraseServerActionCapability, textServerActionCapability, stopEverything},
			Updated:      timestamp.Now(),
		},
	}
	scenarios := []model.Scenario{
		{
			ID:       "scenario-id-1",
			Name:     "My Super Scenario 1",
			Icon:     model.ScenarioIconDay,
			Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "My Super Scenario 1"}},
			Steps: model.ScenarioSteps{
				model.MakeScenarioStepByType(model.ScenarioStepActionsType).
					WithParameters(model.ScenarioStepActionsParameters{
						Devices: model.ScenarioLaunchDevices{
							model.ScenarioLaunchDevice{
								ID:           "scenario_device-id-1",
								Name:         "имечко",
								Type:         model.LightDeviceType,
								Capabilities: model.Capabilities{onOffCapability},
							},
						},
						RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
					}),
			},
		},
		{
			ID:       "scenario-id-2",
			Name:     "My Super Scenario 2",
			Icon:     model.ScenarioIconRomantic,
			Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "My Super Scenario 2"}},
			Devices: []model.ScenarioDevice{
				{
					ID: "ololololoDeviceID",
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.QuasarServerActionCapabilityType,
							State: model.QuasarServerActionCapabilityState{
								Instance: model.TextActionCapabilityInstance,
								Value:    "мультики",
							},
						},
					},
				},
			},
		},
		{
			ID:       "scenario-id-3",
			Name:     "My Super Scenario 3",
			Icon:     model.ScenarioIconRomantic,
			Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "My Super Scenario 3"}},
			RequestedSpeakerCapabilities: []model.ScenarioCapability{
				{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.PhraseActionCapabilityInstance,
						Value:    "пыщ пыщ ололо, я водитель нло",
					},
				},
			},
		},
		{
			ID:       "scenario-id-4",
			Name:     "My Super Scenario 4",
			Icon:     model.ScenarioIconRomantic,
			Devices:  []model.ScenarioDevice{},
			Triggers: []model.ScenarioTrigger{},
		},
		{
			ID:       "scenario-id-5",
			Name:     "My Super Scenario 5",
			Icon:     model.ScenarioIconRomantic,
			Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "My Super Scenario 5"}},
			Devices: []model.ScenarioDevice{
				{
					ID: "device-6",
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.QuasarServerActionCapabilityType,
							State: model.QuasarServerActionCapabilityState{
								Instance: model.PhraseActionCapabilityInstance,
								Value:    "новое состояние",
							},
						},
					},
				},
			},
		},
		{
			ID:       "scenario-id-6",
			Name:     "My Super Scenario 6",
			Icon:     model.ScenarioIconRomantic,
			Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "My Super Scenario 6"}},
			RequestedSpeakerCapabilities: []model.ScenarioCapability{
				{
					Type: model.QuasarCapabilityType,
					State: model.QuasarCapabilityState{
						Instance: model.StopEverythingCapabilityInstance,
						Value:    model.StopEverythingQuasarCapabilityValue{},
					},
				},
			},
		},
	}

	extractedActions := []model.ExtractedAction{
		{
			ID:       0,
			Devices:  []model.Device{devices[0], devices[1]},
			Scenario: scenarios[0],
		},
		{
			ID:       17,
			Devices:  []model.Device{devices[2], devices[4]},
			Scenario: scenarios[1],
		},
		{
			ID:       300,
			Devices:  []model.Device{devices[3]},
			Scenario: scenarios[2],
		},
		{
			ID:       1,
			Devices:  []model.Device{},
			Scenario: scenarios[3],
		},
		{
			ID:       6,
			Devices:  []model.Device{devices[5]},
			Scenario: scenarios[4],
		},
		{
			ID:       100,
			Devices:  []model.Device{devices[3]},
			Scenario: scenarios[5],
		},
	}

	aaDTO := DeviceActionsApplyArguments{}
	aaDTO.Devices = devices
	aaDTO.ExtractedActions = extractedActions

	protoAA := aaDTO.ToProto()

	aaFromProto := DeviceActionsApplyArguments{}
	aaFromProto.FromProto(protoAA)

	assert.ElementsMatch(t, aaDTO.Devices, aaFromProto.Devices)
	assert.ElementsMatch(t, aaDTO.ExtractedActions, aaFromProto.ExtractedActions)
	assert.Equal(t, aaDTO, aaFromProto)
}
