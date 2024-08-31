package xtestdata

import (
	"context"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func GenerateDevice() *model.Device {
	// todo: start using xtestdata instead of data
	device := data.GenerateDevice()
	device.Name = device.ExternalName       // data does not set name, but after inserting name is set to externalName
	device.DeviceInfo = &model.DeviceInfo{} // data does not set deviceInfo, but after inserting it appears with nil values
	return &device
}

func GenerateHumidifier(id, externalID, skillID string) *model.Device {
	return &model.Device{
		ID:           id,
		Name:         "увлажнитель",
		ExternalID:   externalID,
		ExternalName: "humidifier",
		SkillID:      skillID,
		Type:         model.HumidifierDeviceType,
		OriginalType: model.HumidifierDeviceType,
		Capabilities: model.Capabilities{
			OnOffCapability(false).
				WithRetrievable(false).
				WithReportable(false),
		},
		Properties: model.Properties{},
		DeviceInfo: &model.DeviceInfo{},
		Status:     model.UnknownDeviceStatus,
	}
}

func GenerateAC(id, externalID, skillID string) *model.Device {
	return &model.Device{
		ID:           id,
		Name:         "кондиционер",
		ExternalID:   externalID,
		ExternalName: "ac",
		SkillID:      skillID,
		Type:         model.AcDeviceType,
		OriginalType: model.AcDeviceType,
		Capabilities: model.Capabilities{
			OnOffCapability(false).
				WithRetrievable(false).
				WithReportable(false),
		},
		Properties: model.Properties{},
		DeviceInfo: &model.DeviceInfo{},
		Status:     model.UnknownDeviceStatus,
	}
}

func GenerateSwitchNoState(id, externalID, skillID string) *model.Device {
	return &model.Device{
		ID:           id,
		Name:         "выключатель",
		ExternalID:   externalID,
		ExternalName: "vikluchatel",
		SkillID:      skillID,
		Type:         model.SwitchDeviceType,
		OriginalType: model.SwitchDeviceType,
		Capabilities: model.Capabilities{
			OnOffCapability(false).
				WithRetrievable(false).
				WithReportable(false),
		},
		Properties: model.Properties{},
		DeviceInfo: &model.DeviceInfo{},
		Status:     model.UnknownDeviceStatus,
	}
}

func GenerateLamp(id, externalID, skillID string) *model.Device {
	return &model.Device{
		ID:           id,
		Name:         "Лампа",
		Aliases:      []string{},
		ExternalID:   externalID,
		ExternalName: "Лампа",
		SkillID:      skillID,
		Type:         model.LightDeviceType,
		OriginalType: model.LightDeviceType,
		Capabilities: model.Capabilities{OnOffCapability(false)},
		Properties:   model.Properties{},
		Status:       model.UnknownDeviceStatus,
		DeviceInfo:   &model.DeviceInfo{},
	}
}

func GenerateYandexIOLamp(id, externalID, parentEndpointID string) *model.Device {
	return GenerateLamp(id, externalID, model.YANDEXIO).WithCustomData(yandexiocd.CustomData{ParentEndpointID: parentEndpointID})
}

func GenerateYandexIOMotionSensor(id, externalID, parentEndpointID string) *model.Device {
	return &model.Device{
		ID:           id,
		Name:         "Датчик движения",
		Aliases:      []string{},
		ExternalID:   externalID,
		ExternalName: "Датчик движения",
		SkillID:      model.YANDEXIO,
		Type:         model.SensorDeviceType,
		OriginalType: model.SensorDeviceType,
		Capabilities: model.Capabilities{},
		Properties: model.Properties{
			EventProperty(model.MotionPropertyInstance, []model.EventValue{model.DetectedEvent}, model.DetectedEvent),
		},
		CustomData: yandexiocd.CustomData{ParentEndpointID: parentEndpointID},
		Status:     model.UnknownDeviceStatus,
		DeviceInfo: &model.DeviceInfo{},
	}
}

func GenerateYandexIOButtonSensor(id, externalID, parentEndpointID string) *model.Device {
	return &model.Device{
		ID:           id,
		Name:         "Кнопка",
		Aliases:      []string{},
		ExternalID:   externalID,
		ExternalName: "Кнопка",
		SkillID:      model.YANDEXIO,
		Type:         model.SwitchDeviceType,
		OriginalType: model.SwitchDeviceType,
		Capabilities: model.Capabilities{},
		Properties: model.Properties{
			EventProperty(model.ButtonPropertyInstance, []model.EventValue{model.ClickEvent}, model.ClickEvent),
		},
		CustomData: yandexiocd.CustomData{ParentEndpointID: parentEndpointID},
		Status:     model.UnknownDeviceStatus,
		DeviceInfo: &model.DeviceInfo{},
	}
}

func GenerateTuyaSocket(id, externalID string) *model.Device {
	return &model.Device{
		ID:           id,
		Name:         "Розетка",
		Aliases:      []string{},
		ExternalID:   externalID,
		ExternalName: "Розетка",
		SkillID:      model.TUYA,
		Type:         model.SocketDeviceType,
		OriginalType: model.SocketDeviceType,
		Capabilities: model.Capabilities{OnOffCapability(true)},
		Properties: model.Properties{
			FloatProperty(model.AmperagePropertyInstance, model.UnitAmpere, 1),
			FloatProperty(model.VoltagePropertyInstance, model.UnitVolt, 220),
			FloatProperty(model.PowerPropertyInstance, model.UnitWatt, 220),
		},
		Status:     model.UnknownDeviceStatus,
		DeviceInfo: &model.DeviceInfo{},
	}
}

func GenerateEmptySensor(id, externalID string, skillID string) *model.Device {
	return &model.Device{
		ID:           id,
		Name:         "Датчик положения коленвала",
		ExternalID:   externalID,
		ExternalName: "Kneeval position datchik",
		SkillID:      skillID,
		Type:         model.SensorDeviceType,
		OriginalType: model.SensorDeviceType,
		Capabilities: model.Capabilities{},
		Properties:   model.Properties{},
		Status:       model.UnknownDeviceStatus,
		DeviceInfo:   &model.DeviceInfo{},
	}
}

func GenerateXiaomiClimateSensor(id, externalID string) *model.Device {
	return &model.Device{
		ID:           id,
		Name:         "Датчик климата",
		Aliases:      []string{},
		ExternalID:   externalID,
		ExternalName: "Датчик климата",
		SkillID:      model.XiaomiSkill,
		Type:         model.SensorDeviceType,
		OriginalType: model.SensorDeviceType,
		Capabilities: model.Capabilities{},
		Properties: model.Properties{
			FloatProperty(model.TemperaturePropertyInstance, model.UnitTemperatureCelsius, 26),
			FloatProperty(model.HumidityPropertyInstance, model.UnitPercent, 35),
		},
		Status:     model.UnknownDeviceStatus,
		DeviceInfo: &model.DeviceInfo{},
	}
}

func GenerateXiaomiMotionSensor(id, externalID string) *model.Device {
	return &model.Device{
		ID:           id,
		Name:         "Датчик движения",
		Aliases:      []string{},
		ExternalID:   externalID,
		ExternalName: "Датчик движения",
		SkillID:      model.XiaomiSkill,
		Type:         model.SensorDeviceType,
		OriginalType: model.SensorDeviceType,
		Capabilities: model.Capabilities{},
		Properties: model.Properties{
			EventProperty(model.MotionPropertyInstance, []model.EventValue{model.DetectedEvent, model.NotDetectedEvent}, model.NotDetectedEvent),
		},
		Status:     model.UnknownDeviceStatus,
		DeviceInfo: &model.DeviceInfo{},
	}
}

func GenerateXiaomiOpeningSensor(id, externalID string) *model.Device {
	return &model.Device{
		ID:           id,
		Name:         "Датчик открытия",
		Aliases:      []string{},
		ExternalID:   externalID,
		ExternalName: "Датчик открытия",
		SkillID:      model.XiaomiSkill,
		Type:         model.SensorDeviceType,
		OriginalType: model.SensorDeviceType,
		Capabilities: model.Capabilities{},
		Properties: model.Properties{
			EventProperty(model.OpenPropertyInstance, []model.EventValue{model.OpenedEvent, model.ClosedEvent}, model.ClosedEvent),
		},
		Status:     model.UnknownDeviceStatus,
		DeviceInfo: &model.DeviceInfo{},
	}
}

func GenerateSmartSpeaker(id, externalID, quasarID string, deviceType model.DeviceType) *model.Device {
	speaker := &model.Device{
		ID:           id,
		Name:         "Колонка",
		ExternalID:   externalID,
		ExternalName: "Колонка",
		SkillID:      model.QUASAR,
		Type:         deviceType,
		OriginalType: deviceType,
		Capabilities: model.GenerateQuasarCapabilities(context.Background(), deviceType),
		Properties:   model.Properties{},
		CustomData: quasar.CustomData{
			DeviceID: quasarID,
			Platform: string(model.KnownQuasarPlatforms[deviceType]),
		},
		Status:     model.UnknownDeviceStatus,
		DeviceInfo: &model.DeviceInfo{},
	}
	return speaker
}

func GenerateMidiSpeaker(id, externalID, deviceID string) *model.Device {
	speaker := GenerateSmartSpeaker(id, externalID, deviceID, model.YandexStationMidiDeviceType)
	speaker.Name = "Яндекс Станция 2"
	speaker.ExternalName = "Яндекс Станция 2"
	speaker.Capabilities = append(speaker.Capabilities, model.MakeCapabilityByType(model.ColorSettingCapabilityType).
		WithRetrievable(true).
		WithReportable(true).
		WithParameters(
			model.ColorSettingCapabilityParameters{
				ColorSceneParameters: &model.ColorSceneParameters{
					Scenes: []model.ColorScene{
						model.KnownColorScenes[model.ColorSceneIDNight],
					},
				},
			},
		),
	)
	return speaker
}

func CreateStereopair(devices model.Devices) *model.Stereopair {
	return &model.Stereopair{
		ID:      uuid.Must(uuid.NewV4()).String(),
		Name:    "Стереопара",
		Devices: devices,
		Created: timestamp.Now(),
		Config: model.StereopairConfig{Devices: []model.StereopairDeviceConfig{
			{
				ID:      devices[0].ID,
				Channel: model.LeftChannel,
				Role:    model.LeaderRole,
			},
			{
				ID:      devices[1].ID,
				Channel: model.RightChannel,
				Role:    model.FollowerRole,
			},
		}},
	}
}

func WrapDevices(devices ...*model.Device) model.Devices {
	result := make(model.Devices, 0, len(devices))
	for _, device := range devices {
		result = append(result, *device)
	}
	return result
}
