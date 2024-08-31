package main

import (
	"fmt"
	"strconv"

	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

var (
	externalIDPrefix = "IOT-1102" // ticket number or any other combination to search
)

func externalIDGenerator() string {
	externalID, _ := uuid.NewV4()
	if len(externalIDPrefix) > 0 {
		return fmt.Sprintf("%s-%s", externalIDPrefix, externalID.String())
	}
	return externalID.String()
}

func generateSocket(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	amperage := model.MakePropertyByType(model.FloatPropertyType)
	amperage.SetRetrievable(true)
	amperage.SetParameters(model.FloatPropertyParameters{
		Instance: model.AmperagePropertyInstance,
		Unit:     model.UnitAmpere,
	})
	amperage.SetState(model.FloatPropertyState{
		Instance: model.AmperagePropertyInstance,
		Value:    5,
	})

	voltage := model.MakePropertyByType(model.FloatPropertyType)
	voltage.SetRetrievable(true)
	voltage.SetParameters(model.FloatPropertyParameters{
		Instance: model.VoltagePropertyInstance,
		Unit:     model.UnitVolt,
	})
	voltage.SetState(model.FloatPropertyState{
		Instance: model.VoltagePropertyInstance,
		Value:    10,
	})

	power := model.MakePropertyByType(model.FloatPropertyType)
	power.SetRetrievable(true)
	power.SetParameters(model.FloatPropertyParameters{
		Instance: model.PowerPropertyInstance,
		Unit:     model.UnitWatt,
	})
	power.SetState(model.FloatPropertyState{
		Instance: model.PowerPropertyInstance,
		Value:    50,
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.SocketDeviceType,
		OriginalType: model.SocketDeviceType,
		Capabilities: []model.ICapability{
			onOff,
		},
		Properties: []model.IProperty{
			voltage,
			amperage,
			power,
		},
	}
}

func generateAirQualityStation(name, externalName, externalID string, opts ...Option) model.Device {
	pm1 := model.MakePropertyByType(model.FloatPropertyType)
	pm1.SetRetrievable(true)
	pm1.SetReportable(true)
	pm1.SetParameters(model.FloatPropertyParameters{
		Instance: model.PM1DensityPropertyInstance,
		Unit:     model.UnitDensityMcgM3,
	})
	pm1.SetState(model.FloatPropertyState{
		Instance: model.PM1DensityPropertyInstance,
		Value:    25,
	})

	pm25 := model.MakePropertyByType(model.FloatPropertyType)
	pm25.SetRetrievable(true)
	pm25.SetReportable(true)
	pm25.SetParameters(model.FloatPropertyParameters{
		Instance: model.PM2p5DensityPropertyInstance,
		Unit:     model.UnitDensityMcgM3,
	})
	pm25.SetState(model.FloatPropertyState{
		Instance: model.PM2p5DensityPropertyInstance,
		Value:    31,
	})

	pm10 := model.MakePropertyByType(model.FloatPropertyType)
	pm10.SetRetrievable(true)
	pm10.SetReportable(true)
	pm10.SetParameters(model.FloatPropertyParameters{
		Instance: model.PM10DensityPropertyInstance,
		Unit:     model.UnitDensityMcgM3,
	})
	pm10.SetState(model.FloatPropertyState{
		Instance: model.PM10DensityPropertyInstance,
		Value:    49,
	})

	tvoc := model.MakePropertyByType(model.FloatPropertyType)
	tvoc.SetRetrievable(true)
	tvoc.SetReportable(true)
	tvoc.SetParameters(model.FloatPropertyParameters{
		Instance: model.TvocPropertyInstance,
		Unit:     model.UnitDensityMcgM3,
	})
	tvoc.SetState(model.FloatPropertyState{
		Instance: model.TvocPropertyInstance,
		Value:    42,
	})

	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.SensorDeviceType,
		OriginalType: model.SensorDeviceType,
		Capabilities: []model.ICapability{},
		Properties: []model.IProperty{
			pm1, pm25, pm10, tvoc,
		},
	}
}

func generateLamp(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})
	lampBrightness := model.MakeCapabilityByType(model.RangeCapabilityType)
	lampBrightness.SetRetrievable(true)
	lampBrightness.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.BrightnessRangeInstance,
		Unit:         model.UnitPercent,
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})
	lampBrightness.SetState(model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    1,
	})

	scenes := make(model.ColorScenes, 0, len(model.KnownColorScenes))
	for _, sc := range model.KnownColorScenes {
		scenes = append(scenes, sc)
	}

	lampColor := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	lampColor.SetRetrievable(true)
	lampColor.SetParameters(model.ColorSettingCapabilityParameters{
		ColorModel: model.CM(model.HsvModelType),
		TemperatureK: &model.TemperatureKParameters{
			Min: 2000,
			Max: 9000,
		},
		ColorSceneParameters: &model.ColorSceneParameters{
			Scenes: scenes,
		},
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		case noColorModelOption:
			parameters := lampColor.Parameters().(model.ColorSettingCapabilityParameters)
			parameters.ColorModel = nil
			lampColor.SetParameters(parameters)
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.LightDeviceType,
		OriginalType: model.LightDeviceType,
		Capabilities: []model.ICapability{
			onOff,
			lampBrightness,
			lampColor,
		},
		Properties: []model.IProperty{},
	}
}

func generateConditioner(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	temperature := model.MakeCapabilityByType(model.RangeCapabilityType)
	temperature.SetRetrievable(true)
	temperature.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.TemperatureRangeInstance,
		Unit:         model.UnitTemperatureCelsius,
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       1,
			Max:       30,
			Precision: 1,
		},
	})

	fanSpeed := model.MakeCapabilityByType(model.ModeCapabilityType)
	fanSpeed.SetRetrievable(true)
	fanSpeed.SetParameters(model.ModeCapabilityParameters{
		Instance: model.FanSpeedModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.LowMode],
			model.KnownModes[model.MediumMode],
			model.KnownModes[model.HighMode],
		},
	})

	thermostat := model.MakeCapabilityByType(model.ModeCapabilityType)
	thermostat.SetRetrievable(true)
	thermostat.SetParameters(model.ModeCapabilityParameters{
		Instance: model.ThermostatModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.AutoMode],
			model.KnownModes[model.CoolMode],
		},
	})

	capabilitiesToDelete := make(map[string]struct{})
	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})

			// thermostat
		case unretrievableThermostatOption:
			thermostat.SetRetrievable(false)
		case thermostatModeOption:
			parameters := thermostat.Parameters().(model.ModeCapabilityParameters)
			parameters.Modes = v.Modes
			thermostat.SetParameters(parameters)

			// fanspeed
		case unretrievableFanSpeedOption:
			fanSpeed.SetRetrievable(false)
		case fanSpeedModeOption:
			parameters := fanSpeed.Parameters().(model.ModeCapabilityParameters)
			parameters.Modes = v.Modes
			fanSpeed.SetParameters(parameters)

			//temperature
		case unretrievableTemperatureRangeOption:
			temperature.SetRetrievable(false)
		case noTemperatureRangeOption:
			capabilitiesToDelete[temperature.Key()] = struct{}{}
		case temperatureRangeBoundsOption:
			parameters := temperature.Parameters().(model.RangeCapabilityParameters)
			parameters.Range.Min = v.min
			parameters.Range.Max = v.max
			temperature.SetParameters(parameters)
		}
	}

	prefilteredCapabilities := []model.ICapability{
		onOff,
		temperature,
		fanSpeed,
		thermostat,
	}
	filteredCapabilities := make([]model.ICapability, 0, len(prefilteredCapabilities))
	for _, capability := range prefilteredCapabilities {
		if _, shouldBeDeleted := capabilitiesToDelete[capability.Key()]; shouldBeDeleted {
			continue
		}
		filteredCapabilities = append(filteredCapabilities, capability)
	}

	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.AcDeviceType,
		OriginalType: model.AcDeviceType,
		Capabilities: filteredCapabilities,
		Properties:   []model.IProperty{},
	}
}

func generateThermostat(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	thermostatTemperature := model.MakeCapabilityByType(model.RangeCapabilityType)
	thermostatTemperature.SetRetrievable(true)
	thermostatTemperature.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.TemperatureRangeInstance,
		Unit:         model.UnitTemperatureCelsius,
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       1,
			Max:       30,
			Precision: 1,
		},
	})

	fanSpeedMode := model.MakeCapabilityByType(model.ModeCapabilityType)
	fanSpeedMode.SetRetrievable(true)
	fanSpeedMode.SetParameters(model.ModeCapabilityParameters{
		Instance: model.FanSpeedModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.LowMode],
			model.KnownModes[model.MediumMode],
			model.KnownModes[model.HighMode],
		},
	})

	thermostatMode := model.MakeCapabilityByType(model.ModeCapabilityType)
	thermostatMode.SetRetrievable(true)
	thermostatMode.SetParameters(model.ModeCapabilityParameters{
		Instance: model.ThermostatModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.AutoMode],
			model.KnownModes[model.CoolMode],
		},
	})

	capabilitiesToDelete := map[string]struct{}{}
	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})

			// thermostat
		case unretrievableThermostatOption:
			thermostatMode.SetRetrievable(false)
		case thermostatModeOption:
			parameters := thermostatMode.Parameters().(model.ModeCapabilityParameters)
			parameters.Modes = v.Modes
			thermostatMode.SetParameters(parameters)

			// fanspeed
		case unretrievableFanSpeedOption:
			fanSpeedMode.SetRetrievable(false)
		case fanSpeedModeOption:
			parameters := fanSpeedMode.Parameters().(model.ModeCapabilityParameters)
			parameters.Modes = v.Modes
			fanSpeedMode.SetParameters(parameters)

			//temperature
		case unretrievableTemperatureRangeOption:
			thermostatTemperature.SetRetrievable(false)
		case noTemperatureRangeOption:
			capabilitiesToDelete[thermostatTemperature.Key()] = struct{}{}
		case temperatureRangeBoundsOption:
			parameters := thermostatTemperature.Parameters().(model.RangeCapabilityParameters)
			parameters.Range.Min = v.min
			parameters.Range.Max = v.max
			thermostatTemperature.SetParameters(parameters)
		}
	}

	prefilteredCapabilities := []model.ICapability{
		onOff,
		thermostatTemperature,
		fanSpeedMode,
		thermostatMode,
	}
	filteredCapabilities := make([]model.ICapability, 0, len(prefilteredCapabilities))
	for _, capability := range prefilteredCapabilities {
		if _, shouldBeDeleted := capabilitiesToDelete[capability.Key()]; shouldBeDeleted {
			continue
		}
		filteredCapabilities = append(filteredCapabilities, capability)
	}

	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.ThermostatDeviceType,
		OriginalType: model.ThermostatDeviceType,
		Capabilities: filteredCapabilities,
		Properties:   []model.IProperty{},
	}
}

func generateTV(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	tvVolume := model.MakeCapabilityByType(model.RangeCapabilityType)
	tvVolume.SetRetrievable(true)
	tvVolume.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.VolumeRangeInstance,
		Unit:         "",
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       0,
			Max:       100,
			Precision: 1,
		},
	})
	tvVolume.SetState(model.RangeCapabilityState{
		Instance: model.VolumeRangeInstance,
		Value:    1,
	})

	tvChannel := model.MakeCapabilityByType(model.RangeCapabilityType)
	tvChannel.SetRetrievable(true)
	tvChannel.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.ChannelRangeInstance,
		Unit:         "",
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})
	tvChannel.SetState(model.RangeCapabilityState{
		Instance: model.ChannelRangeInstance,
		Value:    1,
	})

	tvMute := model.MakeCapabilityByType(model.ToggleCapabilityType)
	tvMute.SetRetrievable(true)
	tvMute.SetParameters(model.ToggleCapabilityParameters{Instance: model.MuteToggleCapabilityInstance})

	tvPause := model.MakeCapabilityByType(model.ToggleCapabilityType)
	tvPause.SetRetrievable(true)
	tvPause.SetParameters(model.ToggleCapabilityParameters{Instance: model.PauseToggleCapabilityInstance})

	tvInputSource := model.MakeCapabilityByType(model.ModeCapabilityType)
	tvInputSource.SetRetrievable(true)
	tvInputSource.SetParameters(model.ModeCapabilityParameters{
		Instance: model.InputSourceModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.OneMode],
			model.KnownModes[model.TwoMode],
			model.KnownModes[model.ThreeMode],
		},
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		case unretrievableChannelRangeOption:
			tvChannel.SetRetrievable(false)
		case channelRangeBoundsOption:
			parameters := tvChannel.Parameters().(model.RangeCapabilityParameters)
			parameters.Range.Min = v.min
			parameters.Range.Max = v.max
			tvChannel.SetParameters(parameters)
		case unretrievableInputSourceOption:
			tvInputSource.SetRetrievable(false)
		case inputSourceModeOption:
			parameters := tvInputSource.Parameters().(model.ModeCapabilityParameters)
			parameters.Modes = v.Modes
			tvInputSource.SetParameters(parameters)
		case unretrievableMuteToggleOption:
			tvMute.SetRetrievable(false)
		case unretrievablePauseToggleOption:
			tvPause.SetRetrievable(false)
		case unretrievableVolumeRangeOption:
			tvVolume.SetRetrievable(false)
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.TvDeviceDeviceType,
		OriginalType: model.TvDeviceDeviceType,
		Capabilities: []model.ICapability{
			onOff,
			tvVolume,
			tvChannel,
			tvMute,
			tvPause,
			tvInputSource,
		},
		Properties: []model.IProperty{},
	}
}

func generateVacuumCleaner(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	workSpeedMode := model.MakeCapabilityByType(model.ModeCapabilityType)
	workSpeedMode.SetRetrievable(true)
	workSpeedMode.SetParameters(model.ModeCapabilityParameters{
		Instance: model.WorkSpeedModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.QuietMode],
			model.KnownModes[model.NormalMode],
			model.KnownModes[model.FastMode],
			model.KnownModes[model.TurboMode],
		},
	})

	pauseToggle := model.MakeCapabilityByType(model.ToggleCapabilityType)
	pauseToggle.SetRetrievable(true)
	pauseToggle.SetParameters(model.ToggleCapabilityParameters{Instance: model.PauseToggleCapabilityInstance})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}

	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.VacuumCleanerDeviceType,
		OriginalType: model.VacuumCleanerDeviceType,
		Capabilities: []model.ICapability{
			onOff,
			workSpeedMode,
			pauseToggle,
		},
		Properties: []model.IProperty{},
	}
}

func generateCurtain(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	openRange := model.MakeCapabilityByType(model.RangeCapabilityType)
	openRange.SetRetrievable(true)
	openRange.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.OpenRangeInstance,
		Unit:         model.UnitPercent,
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       0,
			Max:       100,
			Precision: 1,
		},
	})
	openRange.SetState(model.RangeCapabilityState{
		Instance: model.OpenRangeInstance,
		Value:    0,
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.CurtainDeviceType,
		OriginalType: model.CurtainDeviceType,
		Capabilities: []model.ICapability{
			onOff, openRange,
		},
		Properties: []model.IProperty{},
	}
}

func generateHumidifier(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	humidityRange := model.MakeCapabilityByType(model.RangeCapabilityType)
	humidityRange.SetRetrievable(true)
	humidityRange.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.HumidityRangeInstance,
		Unit:         model.UnitPercent,
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       10,
			Max:       70,
			Precision: 1,
		},
	})
	humidityRange.SetState(model.RangeCapabilityState{
		Instance: model.HumidityRangeInstance,
		Relative: nil,
		Value:    50,
	})

	waterLevel := model.MakePropertyByType(model.FloatPropertyType)
	waterLevel.SetRetrievable(true)
	waterLevel.SetParameters(model.FloatPropertyParameters{
		Instance: model.WaterLevelPropertyInstance,
		Unit:     model.UnitPercent,
	})
	waterLevel.SetState(model.FloatPropertyState{
		Instance: model.WaterLevelPropertyInstance,
		Value:    100,
	})

	co2Level := model.MakePropertyByType(model.FloatPropertyType)
	co2Level.SetRetrievable(true)
	co2Level.SetParameters(model.FloatPropertyParameters{
		Instance: model.CO2LevelPropertyInstance,
		Unit:     model.UnitPPM,
	})
	co2Level.SetState(model.FloatPropertyState{
		Instance: model.CO2LevelPropertyInstance,
		Value:    700,
	})

	relativeHumidity := model.MakePropertyByType(model.FloatPropertyType)
	relativeHumidity.SetRetrievable(true)
	relativeHumidity.SetReportable(true)
	relativeHumidity.SetParameters(model.FloatPropertyParameters{
		Instance: model.HumidityPropertyInstance,
		Unit:     model.UnitPercent,
	})
	relativeHumidity.SetState(model.FloatPropertyState{
		Instance: model.HumidityPropertyInstance,
		Value:    40,
	})

	temperature := model.MakePropertyByType(model.FloatPropertyType)
	temperature.SetRetrievable(true)
	temperature.SetReportable(true)
	temperature.SetParameters(model.FloatPropertyParameters{
		Instance: model.TemperaturePropertyInstance,
		Unit:     model.UnitTemperatureCelsius,
	})
	temperature.SetState(model.FloatPropertyState{
		Instance: model.TemperaturePropertyInstance,
		Value:    20,
	})

	amperage := model.MakePropertyByType(model.FloatPropertyType)
	amperage.SetRetrievable(true)
	amperage.SetReportable(true)
	amperage.SetParameters(model.FloatPropertyParameters{
		Instance: model.AmperagePropertyInstance,
		Unit:     model.UnitAmpere,
	})
	amperage.SetState(model.FloatPropertyState{
		Instance: model.AmperagePropertyInstance,
		Value:    5,
	})

	voltage := model.MakePropertyByType(model.FloatPropertyType)
	voltage.SetRetrievable(true)
	voltage.SetReportable(true)
	voltage.SetParameters(model.FloatPropertyParameters{
		Instance: model.VoltagePropertyInstance,
		Unit:     model.UnitVolt,
	})
	voltage.SetState(model.FloatPropertyState{
		Instance: model.VoltagePropertyInstance,
		Value:    10,
	})

	power := model.MakePropertyByType(model.FloatPropertyType)
	power.SetRetrievable(true)
	power.SetReportable(true)
	power.SetParameters(model.FloatPropertyParameters{
		Instance: model.PowerPropertyInstance,
		Unit:     model.UnitWatt,
	})
	power.SetState(model.FloatPropertyState{
		Instance: model.PowerPropertyInstance,
		Value:    50,
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		case unreportableTemperaturePropertyOption:
			temperature.SetReportable(false)
		case unreportableHumidityPropertyOption:
			relativeHumidity.SetReportable(false)
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.HumidifierDeviceType,
		OriginalType: model.HumidifierDeviceType,
		Capabilities: []model.ICapability{
			onOff,
			humidityRange,
		},
		Properties: []model.IProperty{
			waterLevel,
			co2Level,
			relativeHumidity,
			temperature,
			amperage,
			voltage,
			power,
		},
	}
}

func generatePurifier(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	co2Level := model.MakePropertyByType(model.FloatPropertyType)
	co2Level.SetRetrievable(true)
	co2Level.SetReportable(true)
	co2Level.SetParameters(model.FloatPropertyParameters{
		Instance: model.CO2LevelPropertyInstance,
		Unit:     model.UnitPPM,
	})
	co2Level.SetState(model.FloatPropertyState{
		Instance: model.CO2LevelPropertyInstance,
		Value:    700,
	})

	pm1 := model.MakePropertyByType(model.FloatPropertyType)
	pm1.SetRetrievable(true)
	pm1.SetReportable(true)
	pm1.SetParameters(model.FloatPropertyParameters{
		Instance: model.PM1DensityPropertyInstance,
		Unit:     model.UnitDensityMcgM3,
	})

	pm25 := model.MakePropertyByType(model.FloatPropertyType)
	pm25.SetRetrievable(true)
	pm25.SetReportable(true)
	pm25.SetParameters(model.FloatPropertyParameters{
		Instance: model.PM2p5DensityPropertyInstance,
		Unit:     model.UnitDensityMcgM3,
	})
	pm10 := model.MakePropertyByType(model.FloatPropertyType)
	pm10.SetRetrievable(true)
	pm10.SetReportable(true)
	pm10.SetParameters(model.FloatPropertyParameters{
		Instance: model.PM10DensityPropertyInstance,
		Unit:     model.UnitDensityMcgM3,
	})

	tvoc := model.MakePropertyByType(model.FloatPropertyType)
	tvoc.SetRetrievable(true)
	tvoc.SetReportable(true)
	tvoc.SetParameters(model.FloatPropertyParameters{
		Instance: model.TvocPropertyInstance,
		Unit:     model.UnitDensityMcgM3,
	})

	temperature := model.MakePropertyByType(model.FloatPropertyType)
	temperature.SetRetrievable(true)
	temperature.SetReportable(true)
	temperature.SetParameters(model.FloatPropertyParameters{
		Instance: model.TemperaturePropertyInstance,
		Unit:     model.UnitTemperatureCelsius,
	})
	temperature.SetState(model.FloatPropertyState{
		Instance: model.TemperaturePropertyInstance,
		Value:    20,
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		case unreportableTemperaturePropertyOption:
			temperature.SetReportable(false)
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.PurifierDeviceType,
		OriginalType: model.PurifierDeviceType,
		Capabilities: []model.ICapability{
			onOff,
		},
		Properties: []model.IProperty{
			co2Level,
			temperature,
			pm1,
			pm25,
			pm10,
			tvoc,
		},
	}
}

func generateCar(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	centralLock := model.MakeCapabilityByType(model.ToggleCapabilityType)
	centralLock.SetRetrievable(true)
	centralLock.SetParameters(model.ToggleCapabilityParameters{Instance: model.CentralLockCapabilityInstance})
	centralLock.SetState(model.ToggleCapabilityState{
		Instance: model.CentralLockCapabilityInstance,
		Value:    true,
	})

	trunk := model.MakeCapabilityByType(model.ToggleCapabilityType)
	trunk.SetRetrievable(true)
	trunk.SetParameters(model.ToggleCapabilityParameters{Instance: model.TrunkToggleCapabilityInstance})
	trunk.SetState(model.ToggleCapabilityState{
		Instance: model.TrunkToggleCapabilityInstance,
		Value:    true,
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.RemoteCarDeviceType,
		OriginalType: model.RemoteCarDeviceType,
		Capabilities: []model.ICapability{
			onOff, centralLock, trunk,
		},
		Properties: []model.IProperty{},
	}
}

func generateReceiver(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	tvVolume := model.MakeCapabilityByType(model.RangeCapabilityType)
	tvVolume.SetRetrievable(true)
	tvVolume.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.VolumeRangeInstance,
		Unit:         "",
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})
	tvVolume.SetState(model.RangeCapabilityState{
		Instance: model.VolumeRangeInstance,
		Value:    1,
	})

	tvChannel := model.MakeCapabilityByType(model.RangeCapabilityType)
	tvChannel.SetRetrievable(true)
	tvChannel.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.ChannelRangeInstance,
		Unit:         "",
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})
	tvChannel.SetState(model.RangeCapabilityState{
		Instance: model.ChannelRangeInstance,
		Value:    1,
	})

	inputSourceMode := model.MakeCapabilityByType(model.ModeCapabilityType)
	inputSourceMode.SetRetrievable(true)
	inputSourceMode.SetParameters(model.ModeCapabilityParameters{
		Instance: model.InputSourceModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.OneMode],
			model.KnownModes[model.TwoMode],
			model.KnownModes[model.ThreeMode],
			model.KnownModes[model.FourMode],
			model.KnownModes[model.FiveMode],
		},
	})
	inputSourceMode.SetState(model.ModeCapabilityState{
		Instance: model.InputSourceModeInstance,
		Value:    model.OneMode,
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.ReceiverDeviceType,
		OriginalType: model.ReceiverDeviceType,
		Capabilities: []model.ICapability{
			onOff,
			tvVolume,
			tvChannel,
			inputSourceMode,
		},
		Properties: []model.IProperty{},
	}
}

func generateTVBox(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	tvVolume := model.MakeCapabilityByType(model.RangeCapabilityType)
	tvVolume.SetRetrievable(true)
	tvVolume.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.VolumeRangeInstance,
		Unit:         "",
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})
	tvVolume.SetState(model.RangeCapabilityState{
		Instance: model.VolumeRangeInstance,
		Value:    1,
	})

	tvChannel := model.MakeCapabilityByType(model.RangeCapabilityType)
	tvChannel.SetRetrievable(true)
	tvChannel.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.ChannelRangeInstance,
		Unit:         "",
		RandomAccess: true,
		Looped:       false,
		Range: &model.Range{
			Min:       1,
			Max:       100,
			Precision: 1,
		},
	})
	tvChannel.SetState(model.RangeCapabilityState{
		Instance: model.ChannelRangeInstance,
		Value:    1,
	})

	inputSourceMode := model.MakeCapabilityByType(model.ModeCapabilityType)
	inputSourceMode.SetRetrievable(true)
	inputSourceMode.SetParameters(model.ModeCapabilityParameters{
		Instance: model.InputSourceModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.OneMode],
			model.KnownModes[model.TwoMode],
			model.KnownModes[model.ThreeMode],
			model.KnownModes[model.FourMode],
			model.KnownModes[model.FiveMode],
		},
	})
	inputSourceMode.SetState(model.ModeCapabilityState{
		Instance: model.InputSourceModeInstance,
		Value:    model.OneMode,
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.TvBoxDeviceType,
		OriginalType: model.TvBoxDeviceType,
		Capabilities: []model.ICapability{
			onOff,
			tvVolume,
			tvChannel,
			inputSourceMode,
		},
		Properties: []model.IProperty{},
	}
}

func generateCustomDevice(name, externalName, externalID string, opts ...Option) model.Device {
	capabilities := make([]model.ICapability, 0)
	for i, opt := range opts {
		switch v := opt.(type) {
		case customButtonOption:
			customButton := model.MakeCapabilityByType(model.CustomButtonCapabilityType)
			customButton.SetRetrievable(true)
			customButton.SetParameters(
				model.CustomButtonCapabilityParameters{
					Instance:      model.CustomButtonCapabilityInstance(strconv.Itoa(i)),
					InstanceNames: []string{v.name},
				},
			)
			capabilities = append(capabilities, customButton)
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.OtherDeviceType,
		OriginalType: model.OtherDeviceType,
		Capabilities: capabilities,
		Properties:   []model.IProperty{},
	}
}

func generateSpeaker(speakerType model.DeviceType, speakerID string) model.Device {
	name := "Умное Устройство"
	if v, ok := speakerName[speakerType]; ok {
		name = v
	}
	platform := "unknown_platform"
	if v, ok := model.KnownQuasarPlatforms[speakerType]; ok {
		platform = string(v)
	}

	capabilities := make([]model.ICapability, 0)
	if speakerType.IsSmartSpeaker() {
		phraseCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		phraseCap.SetRetrievable(false)
		phraseCap.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})
		capabilities = append(capabilities, phraseCap)

		textActionCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		textActionCap.SetRetrievable(false)
		textActionCap.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance})
		capabilities = append(capabilities, textActionCap)
	}

	return model.Device{
		Name:         name,
		ExternalName: name,
		ExternalID:   fmt.Sprintf("%s.%s", speakerID, platform),
		SkillID:      model.VIRTUAL,
		Type:         speakerType,
		OriginalType: speakerType,
		Capabilities: capabilities,
		Properties:   []model.IProperty{},
		CustomData: quasar.CustomData{
			DeviceID: speakerID,
			Platform: platform,
		},
	}
}

func generateDishwasher(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	programMode := model.MakeCapabilityByType(model.ModeCapabilityType)
	programMode.SetRetrievable(true)
	programMode.SetParameters(model.ModeCapabilityParameters{
		Instance: model.ProgramModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.QuietMode],
			model.KnownModes[model.NormalMode],
			model.KnownModes[model.EcoMode],
			model.KnownModes[model.AutoMode],
		},
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.DishwasherDeviceType,
		OriginalType: model.DishwasherDeviceType,
		Capabilities: []model.ICapability{
			onOff,
			programMode,
		},
		Properties: []model.IProperty{},
	}
}

func generateCoffeeMaker(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	coffeeMode := model.MakeCapabilityByType(model.ModeCapabilityType)
	coffeeMode.SetRetrievable(true)
	coffeeMode.SetParameters(model.ModeCapabilityParameters{
		Instance: model.CoffeeModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.LatteMode],
			model.KnownModes[model.CappuccinoMode],
			model.KnownModes[model.EspressoMode],
			model.KnownModes[model.DoubleEspressoMode],
			model.KnownModes[model.AmericanoMode],
		},
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}
	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.CoffeeMakerDeviceType,
		OriginalType: model.CoffeeMakerDeviceType,
		Capabilities: []model.ICapability{
			onOff,
			coffeeMode,
		},
		Properties: []model.IProperty{},
	}
}

func generateMulticooker(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	programMode := model.MakeCapabilityByType(model.ModeCapabilityType)
	programMode.SetRetrievable(true)
	programMode.SetParameters(model.ModeCapabilityParameters{
		Instance: model.ProgramModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.VacuumMode],
			model.KnownModes[model.BoilingMode],
			model.KnownModes[model.BakingMode],
			model.KnownModes[model.DessertMode],
			model.KnownModes[model.BabyFoodMode],
			model.KnownModes[model.FowlMode],
			model.KnownModes[model.FryingMode],
			model.KnownModes[model.YogurtMode],
			model.KnownModes[model.CerealsMode],
			model.KnownModes[model.MacaroniMode],
			model.KnownModes[model.MilkPorridgeMode],
			model.KnownModes[model.MulticookerMode],
			model.KnownModes[model.SteamMode],
			model.KnownModes[model.PastaMode],
			model.KnownModes[model.PizzaMode],
			model.KnownModes[model.PilafMode],
			model.KnownModes[model.SauceMode],
			model.KnownModes[model.SoupMode],
			model.KnownModes[model.StewingMode],
			model.KnownModes[model.SlowCookMode],
			model.KnownModes[model.DeepFryerMode],
			model.KnownModes[model.BreadMode],
			model.KnownModes[model.AspicMode],
			model.KnownModes[model.CheesecakeMode],
		},
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}

	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.MulticookerDeviceType,
		OriginalType: model.MulticookerDeviceType,
		Capabilities: []model.ICapability{
			onOff,
			programMode,
		},
		Properties: []model.IProperty{},
	}
}

func generateKettle(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})
	temperature := model.MakeCapabilityByType(model.RangeCapabilityType)
	temperature.SetRetrievable(true)
	temperature.SetParameters(model.RangeCapabilityParameters{
		Instance:     model.TemperatureRangeInstance,
		Unit:         model.UnitTemperatureCelsius,
		RandomAccess: true,
		Looped:       false,
		Range:        &model.Range{Min: 1, Max: 100, Precision: 1},
	})
	keepWarm := model.MakeCapabilityByType(model.ToggleCapabilityType)
	keepWarm.SetRetrievable(true)
	keepWarm.SetParameters(model.ToggleCapabilityParameters{Instance: model.KeepWarmToggleCapabilityInstance})
	kettleCapabilities := []model.ICapability{onOff, temperature, keepWarm}
	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		case teaModesOption:
			teaMode := model.MakeCapabilityByType(model.ModeCapabilityType)
			teaMode.SetRetrievable(true)
			teaMode.SetParameters(model.ModeCapabilityParameters{Instance: model.TeaModeInstance, Modes: v.teaModes})
			kettleCapabilities = append(kettleCapabilities, teaMode)
		}
	}

	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.KettleDeviceType,
		OriginalType: model.KettleDeviceType,
		Capabilities: kettleCapabilities,
		Properties:   []model.IProperty{},
	}
}

func generateWashingMachine(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	programMode := model.MakeCapabilityByType(model.ModeCapabilityType)
	programMode.SetRetrievable(true)
	programMode.SetParameters(model.ModeCapabilityParameters{
		Instance: model.ProgramModeInstance,
		Modes: []model.Mode{
			model.KnownModes[model.AutoMode],
			model.KnownModes[model.WoolMode],
			model.KnownModes[model.OneMode],
		},
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}

	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.WashingMachineDeviceType,
		OriginalType: model.WashingMachineDeviceType,
		Capabilities: []model.ICapability{
			onOff,
			programMode,
		},
		Properties: []model.IProperty{},
	}
}

func generatePetFeeder(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(false)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}

	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.PetFeederDeviceType,
		OriginalType: model.PetFeederDeviceType,
		Capabilities: []model.ICapability{
			onOff,
		},
		Properties: []model.IProperty{},
	}
}

func generateEventSensor(name, externalName, externalID string, opts ...Option) model.Device {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(false)
	onOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})

	eventPropertyMaker := func(instance model.PropertyInstance, possibleEvents []model.EventValue) model.IProperty {
		events := make(model.Events, 0, len(possibleEvents))
		for _, eventValue := range possibleEvents {
			events = append(events, model.KnownEvents[model.EventKey{Instance: instance, Value: eventValue}])
		}
		res := model.MakePropertyByType(model.EventPropertyType)
		res.SetRetrievable(true)
		res.SetReportable(true)
		res.SetParameters(model.EventPropertyParameters{
			Instance: instance,
			Events:   events,
		})
		res.SetState(model.EventPropertyState{
			Instance: instance,
			Value:    possibleEvents[0],
		})
		return res
	}
	vibration := eventPropertyMaker(model.VibrationPropertyInstance,
		[]model.EventValue{
			model.TiltEvent,
			model.FallEvent,
			model.VibrationEvent,
		})
	openProperty := eventPropertyMaker(model.OpenPropertyInstance,
		[]model.EventValue{
			model.OpenedEvent,
			model.ClosedEvent,
		})
	buttonProperty := eventPropertyMaker(model.ButtonPropertyInstance,
		[]model.EventValue{
			model.ClickEvent,
			model.DoubleClickEvent,
			model.LongPressEvent,
		})
	motionProperty := eventPropertyMaker(model.MotionPropertyInstance,
		[]model.EventValue{
			model.DetectedEvent,
			model.NotDetectedEvent,
		})
	smokeProperty := eventPropertyMaker(model.SmokePropertyInstance,
		[]model.EventValue{
			model.HighEvent,
			model.DetectedEvent,
			model.NotDetectedEvent,
		})
	gasProperty := eventPropertyMaker(model.GasPropertyInstance,
		[]model.EventValue{
			model.DetectedEvent,
			model.NotDetectedEvent,
			model.HighEvent,
		})
	batteryLevelProperty := eventPropertyMaker(model.BatteryLevelPropertyInstance,
		[]model.EventValue{
			model.LowEvent,
			model.NormalEvent,
		})
	waterLevelProperty := eventPropertyMaker(model.WaterLevelPropertyInstance,
		[]model.EventValue{
			model.LowEvent,
			model.NormalEvent,
		})
	waterLeakProperty := eventPropertyMaker(model.WaterLeakPropertyInstance,
		[]model.EventValue{
			model.DryEvent,
			model.LeakEvent,
		})

	amperage := model.MakePropertyByType(model.FloatPropertyType)
	amperage.SetRetrievable(true)
	amperage.SetReportable(true)
	amperage.SetParameters(model.FloatPropertyParameters{
		Instance: model.AmperagePropertyInstance,
		Unit:     model.UnitAmpere,
	})
	amperage.SetState(model.FloatPropertyState{
		Instance: model.AmperagePropertyInstance,
		Value:    5,
	})

	voltage := model.MakePropertyByType(model.FloatPropertyType)
	voltage.SetRetrievable(true)
	voltage.SetReportable(true)
	voltage.SetParameters(model.FloatPropertyParameters{
		Instance: model.VoltagePropertyInstance,
		Unit:     model.UnitVolt,
	})
	voltage.SetState(model.FloatPropertyState{
		Instance: model.VoltagePropertyInstance,
		Value:    10,
	})

	power := model.MakePropertyByType(model.FloatPropertyType)
	power.SetRetrievable(true)
	power.SetReportable(true)
	power.SetParameters(model.FloatPropertyParameters{
		Instance: model.PowerPropertyInstance,
		Unit:     model.UnitWatt,
	})
	power.SetState(model.FloatPropertyState{
		Instance: model.PowerPropertyInstance,
		Value:    50,
	})

	for _, opt := range opts {
		switch v := opt.(type) {
		case unretrievableOnOffOption:
			onOff.SetRetrievable(false)
		case splitOnOffOption:
			onOff.SetParameters(model.OnOffCapabilityParameters{Split: v.Split})
		}
	}

	return model.Device{
		Name:         name,
		ExternalID:   externalID,
		ExternalName: externalName,
		SkillID:      model.VIRTUAL,
		Type:         model.SensorDeviceType,
		OriginalType: model.SensorDeviceType,
		Capabilities: []model.ICapability{
			onOff,
		},
		Properties: []model.IProperty{
			vibration,
			openProperty,
			buttonProperty,
			motionProperty,
			smokeProperty,
			gasProperty,
			batteryLevelProperty,
			waterLevelProperty,
			waterLeakProperty,
			amperage,
			voltage,
			power,
		},
	}
}
