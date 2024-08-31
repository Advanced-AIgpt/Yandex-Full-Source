package uniproxy

import (
	"math"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/ptr"
	"github.com/stretchr/testify/assert"
)

func TestUserInfoViewPopulateDevices(t *testing.T) {
	infoView := UserInfoView{}

	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)

	colorCapabilityOne := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorCapabilityOne.SetRetrievable(true)
	colorCapabilityOne.SetParameters(
		model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.RgbModelType),
		})

	colorCapabilityTwo := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorCapabilityTwo.SetRetrievable(true)
	colorCapabilityTwo.SetParameters(
		model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.RgbModelType),
			TemperatureK: &model.TemperatureKParameters{
				Min: 0,
				Max: math.MaxInt32,
			},
		})

	modeCapabilityOne := model.MakeCapabilityByType(model.ModeCapabilityType)
	modeCapabilityOne.SetRetrievable(true)
	modeCapabilityOne.SetParameters(
		model.ModeCapabilityParameters{
			Instance: model.ThermostatModeInstance,
			Modes: []model.Mode{
				{
					Value: model.AutoMode,
					Name:  ptr.String("Авто"),
				},
				{
					Value: model.DryMode,
					Name:  ptr.String("Сухо"),
				},
				{
					Value: model.EcoMode,
					Name:  ptr.String("Эко"),
				},
			},
		})

	modeCapabilityTwo := model.MakeCapabilityByType(model.ModeCapabilityType)
	modeCapabilityTwo.SetRetrievable(true)
	modeCapabilityTwo.SetParameters(
		model.ModeCapabilityParameters{
			Instance: model.ThermostatModeInstance,
			Modes: []model.Mode{
				{
					Value: model.AutoMode,
					Name:  ptr.String("Авто"),
				},
				{
					Value: model.HeatMode,
					Name:  ptr.String("Жарко"),
				},
				{
					Value: model.CoolMode,
					Name:  ptr.String("Холодно"),
				},
			},
		})

	customButton := model.MakeCapabilityByType(model.CustomButtonCapabilityType)
	customButton.SetParameters(model.CustomButtonCapabilityParameters{
		Instance:      "custom_button_1",
		InstanceNames: []string{"Май литтл кнопка"},
	})

	quasarText := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
	quasarText.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance})

	quasarPhrase := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
	quasarPhrase.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

	stopEverything := model.MakeCapabilityByType(model.QuasarCapabilityType)
	stopEverything.SetParameters(model.QuasarCapabilityParameters{Instance: model.StopEverythingCapabilityInstance})

	propertyCO2 := model.MakePropertyByType(model.FloatPropertyType)
	propertyCO2.SetRetrievable(true)
	propertyCO2.SetParameters(model.FloatPropertyParameters{
		Instance: model.CO2LevelPropertyInstance,
		Unit:     model.UnitPPM,
	})

	propertyTimer := model.MakePropertyByType(model.FloatPropertyType)
	propertyTimer.SetRetrievable(true)
	propertyTimer.SetParameters(model.FloatPropertyParameters{
		Instance: model.TimerPropertyInstance,
		Unit:     model.UnitTimeSeconds,
	})

	group := model.Group{
		ID:      "group-id-1",
		Name:    "Lustra",
		Devices: []string{"device-1", "device-2"},
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
	devices := []model.Device{
		{
			ID:           "device-1",
			Name:         "My lamp",
			Aliases:      []string{},
			Room:         &roomOne,
			Groups:       []model.Group{group},
			Type:         model.LightDeviceType,
			OriginalType: model.LightDeviceType,
			Capabilities: []model.ICapability{onOffCapability, colorCapabilityOne},
		},
		{
			ID:           "device-2",
			Name:         "My lamp 2",
			Aliases:      []string{},
			Room:         &roomTwo,
			Groups:       []model.Group{group},
			Type:         model.LightDeviceType,
			Capabilities: []model.ICapability{onOffCapability, colorCapabilityTwo},
		},
		{
			ID:           "device-3",
			Name:         "My thermostat 1",
			Aliases:      []string{},
			Type:         model.OtherDeviceType,
			Capabilities: []model.ICapability{modeCapabilityOne},
			Properties:   []model.IProperty{propertyCO2, propertyTimer},
		},
		{
			ID:           "device-4",
			ExternalID:   "external-device-4",
			Name:         "My thermostat 2",
			Aliases:      []string{},
			Type:         model.OtherDeviceType,
			Capabilities: []model.ICapability{modeCapabilityTwo, customButton},
		},
		{
			ID:           "device-5",
			ExternalID:   "virtual-device",
			Name:         "My virtual speaker",
			Aliases:      []string{},
			Type:         model.YandexStationDeviceType,
			SkillID:      model.VIRTUAL,
			Capabilities: []model.ICapability{quasarText, quasarPhrase, stopEverything},
		},
	}

	infoView.PopulateDevices(devices)
	infoView.PopulateRooms([]model.Room{roomOne, roomTwo})
	infoView.PopulateGroups([]model.Group{group})

	expectedInfoViewDevices := []DeviceUserInfoView{
		{
			ID:      "device-1",
			Name:    "My lamp",
			Aliases: []string{},
			RoomID:  "my-room-1",
			Groups: []GroupUserInfoView{{
				ID:   group.ID,
				Name: group.Name,
				Type: group.Type}},
			Type:          model.LightDeviceType,
			OriginalType:  model.LightDeviceType,
			IconURL:       model.LightDeviceType.IconURL(model.RawIconFormat),
			AnalyticsType: "Осветительный прибор",
			Properties:    []PropertyUserInfoView{},
			Capabilities: []CapabilityUserInfoView{
				{
					Type:          model.OnOffCapabilityType,
					Instance:      string(model.OnOnOffCapabilityInstance),
					AnalyticsName: "включение/выключение",
					Retrievable:   onOffCapability.Retrievable(),
					Parameters:    onOffCapability.Parameters(),
					State:         onOffCapability.State(),
				},
				{
					Type:          model.ColorSettingCapabilityType,
					Instance:      ColorCapabilityInstance,
					AnalyticsName: "изменение цвета",
					Retrievable:   colorCapabilityOne.Retrievable(),
					Parameters:    colorCapabilityOne.Parameters(),
					State:         colorCapabilityOne.State(),
				},
			},
		},
		{
			ID:      "device-2",
			Name:    "My lamp 2",
			Aliases: []string{},
			RoomID:  "my-room-2",
			Groups: []GroupUserInfoView{{
				ID:   group.ID,
				Name: group.Name,
				Type: group.Type}},
			Type:          model.LightDeviceType,
			IconURL:       model.LightDeviceType.IconURL(model.RawIconFormat),
			AnalyticsType: "Осветительный прибор",
			Properties:    []PropertyUserInfoView{},
			Capabilities: []CapabilityUserInfoView{
				{
					Type:          model.OnOffCapabilityType,
					Instance:      string(model.OnOnOffCapabilityInstance),
					AnalyticsName: "включение/выключение",
					Retrievable:   onOffCapability.Retrievable(),
					Parameters:    onOffCapability.Parameters(),
					State:         onOffCapability.State(),
				},
				{
					Type:          model.ColorSettingCapabilityType,
					Instance:      ColorCapabilityInstance,
					AnalyticsName: "изменение цвета",
					Retrievable:   colorCapabilityTwo.Retrievable(),
					Parameters:    colorCapabilityTwo.Parameters(),
					State:         colorCapabilityTwo.State(),
				},
				{
					Type:          model.ColorSettingCapabilityType,
					Instance:      string(model.TemperatureKCapabilityInstance),
					AnalyticsName: "изменение цветовой температуры",
					Retrievable:   colorCapabilityTwo.Retrievable(),
					Parameters:    colorCapabilityTwo.Parameters(),
					State:         colorCapabilityTwo.State(),
				},
			},
		},
		{
			ID:            "device-3",
			Name:          "My thermostat 1",
			Aliases:       []string{},
			Type:          model.OtherDeviceType,
			IconURL:       model.OtherDeviceType.IconURL(model.RawIconFormat),
			AnalyticsType: "Умное устройство",
			Groups:        []GroupUserInfoView{},
			Properties: []PropertyUserInfoView{
				{
					Type:          propertyCO2.Type(),
					Instance:      propertyCO2.Instance(),
					Retrievable:   propertyCO2.Retrievable(),
					Parameters:    propertyCO2.Parameters(),
					State:         propertyCO2.State(),
					AnalyticsName: "уровень CO2",
				},
				{
					Type:          propertyTimer.Type(),
					Instance:      propertyTimer.Instance(),
					Retrievable:   propertyTimer.Retrievable(),
					Parameters:    propertyTimer.Parameters(),
					State:         propertyTimer.State(),
					AnalyticsName: "таймер",
				},
			},
			Capabilities: []CapabilityUserInfoView{
				{
					Type:          model.ModeCapabilityType,
					Instance:      string(model.ThermostatModeInstance),
					Values:        []string{string(model.AutoMode), string(model.DryMode), string(model.EcoMode)},
					AnalyticsName: "изменение режима термостата",
					Retrievable:   modeCapabilityOne.Retrievable(),
					Parameters:    modeCapabilityOne.Parameters(),
					State:         modeCapabilityOne.State(),
				},
			},
		},
		{
			ID:            "device-4",
			ExternalID:    "external-device-4",
			Name:          "My thermostat 2",
			Aliases:       []string{},
			Type:          model.OtherDeviceType,
			IconURL:       model.OtherDeviceType.IconURL(model.RawIconFormat),
			AnalyticsType: "Умное устройство",
			Groups:        []GroupUserInfoView{},
			Properties:    []PropertyUserInfoView{},
			Capabilities: []CapabilityUserInfoView{
				{
					Type:          model.ModeCapabilityType,
					Instance:      string(model.ThermostatModeInstance),
					Values:        []string{string(model.AutoMode), string(model.HeatMode), string(model.CoolMode)},
					AnalyticsName: "изменение режима термостата",
					Retrievable:   modeCapabilityTwo.Retrievable(),
					Parameters:    modeCapabilityTwo.Parameters(),
					State:         modeCapabilityTwo.State(),
				},
				{
					Type:          model.CustomButtonCapabilityType,
					Instance:      "custom_button_1",
					AnalyticsName: "обученная пользователем кнопка",
					InstanceNames: []string{"Май литтл кнопка"},
					Retrievable:   customButton.Retrievable(),
					Parameters:    customButton.Parameters(),
					State:         customButton.State(),
				},
			},
		},
		{
			ID:            "device-5",
			ExternalID:    "virtual-device",
			Name:          "My virtual speaker",
			Aliases:       []string{},
			Type:          model.YandexStationDeviceType,
			IconURL:       model.YandexStationDeviceType.IconURL(model.RawIconFormat),
			AnalyticsType: "Умное устройство",
			Groups:        []GroupUserInfoView{},
			Properties:    []PropertyUserInfoView{},
			Capabilities:  []CapabilityUserInfoView{},
			QuasarInfo: &QuasarInfo{
				DeviceID: "virtual-device",
				Platform: string(model.KnownQuasarPlatforms[model.YandexStationDeviceType]),
			},
		},
	}

	colors := model.ColorPalette.ToSortedSlice()
	expectedInfoView := UserInfoResponse{
		Payload: UserInfoView{
			Devices: expectedInfoViewDevices,
			Rooms: []RoomUserInfoView{
				{
					ID:   roomOne.ID,
					Name: roomOne.Name,
				},
				{
					ID:   roomTwo.ID,
					Name: roomTwo.Name,
				},
			},
			Groups: []GroupUserInfoView{{
				ID:   group.ID,
				Name: group.Name,
				Type: group.Type}},
			Colors: make([]ColorUserInfoView, 0, len(colors)),
		},
	}
	invalidPayloadColors := make([]ColorUserInfoView, 0, len(colors))
	for _, color := range colors {
		cuiv := ColorUserInfoView{ID: color.ID, Name: color.Name}
		expectedInfoView.Payload.Colors = append(expectedInfoView.Payload.Colors, cuiv)
		for _, colorAlias := range model.ColorIDToAdditionalAliases[color.ID] {
			cuivAlias := ColorUserInfoView{ID: color.ID, Name: colorAlias}
			expectedInfoView.Payload.Colors = append(expectedInfoView.Payload.Colors, cuivAlias)
		}
		invalidPayloadColors = append(invalidPayloadColors, cuiv)
	}
	assert.NotEqual(t, len(invalidPayloadColors), len(expectedInfoView.Payload.Colors))
	assert.NotEqual(t, 0, len(expectedInfoView.Payload.Colors))
	assert.ElementsMatch(t, expectedInfoView.Payload.Devices, infoView.Devices)
	assert.ElementsMatch(t, expectedInfoView.Payload.Scenarios, infoView.Scenarios)
	assert.ElementsMatch(t, expectedInfoView.Payload.Rooms, infoView.Rooms)
	assert.ElementsMatch(t, expectedInfoView.Payload.Groups, infoView.Groups)
	assert.ElementsMatch(t, expectedInfoView.Payload.Colors, infoView.Colors)
}
