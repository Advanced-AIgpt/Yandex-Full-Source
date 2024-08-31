package uniproxy

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

const ColorCapabilityInstance = "color" // magic var to incapsulate HSV or RGB

//overrides `deviceType.GenerateDeviceName()` in order to be more toloka-friendly
var analyticsTypesMap = map[model.DeviceType]string{
	model.LightDeviceType:      "Осветительный прибор",
	model.SwitchDeviceType:     "Выключатель или переключатель",
	model.HubDeviceType:        "Пульт для управления другими устройствами",
	model.CookingDeviceType:    "Кухонная техника",
	model.HumidifierDeviceType: "Увлажнитель воздуха",
	model.PurifierDeviceType:   "Очиститель воздуха",
	model.CameraDeviceType:     "Камера видеонаблюдения",
}

func getAnalyticsType(deviceType model.DeviceType) string {
	v, ok := analyticsTypesMap[deviceType]
	if !ok {
		v = deviceType.GenerateDeviceName()
	}
	if len(v) == 0 {
		return "Умное устройство"
	}
	return v
}

var analyticsNamesMap = map[CapabilityUserInfoKey]string{
	{Type: model.OnOffCapabilityType, Instance: string(model.OnOnOffCapabilityInstance)}: "включение/выключение",

	{Type: model.ColorSettingCapabilityType, Instance: ColorCapabilityInstance}:                      "изменение цвета", // backward compatibility
	{Type: model.ColorSettingCapabilityType, Instance: string(model.RgbColorCapabilityInstance)}:     "изменение цвета",
	{Type: model.ColorSettingCapabilityType, Instance: string(model.HsvColorCapabilityInstance)}:     "изменение цвета",
	{Type: model.ColorSettingCapabilityType, Instance: string(model.TemperatureKCapabilityInstance)}: "изменение цветовой температуры",

	{Type: model.RangeCapabilityType, Instance: string(model.BrightnessRangeInstance)}:  "изменение яркости",
	{Type: model.RangeCapabilityType, Instance: string(model.TemperatureRangeInstance)}: "изменение температуры",
	{Type: model.RangeCapabilityType, Instance: string(model.VolumeRangeInstance)}:      "изменение громкости звука",
	{Type: model.RangeCapabilityType, Instance: string(model.ChannelRangeInstance)}:     "изменение канала",
	{Type: model.RangeCapabilityType, Instance: string(model.HumidityRangeInstance)}:    "изменение влажности",
	{Type: model.RangeCapabilityType, Instance: string(model.OpenRangeInstance)}:        "изменение открытия",

	{Type: model.ToggleCapabilityType, Instance: string(model.MuteToggleCapabilityInstance)}:           "включение/выключение беззвучного режима",
	{Type: model.ToggleCapabilityType, Instance: string(model.BacklightToggleCapabilityInstance)}:      "включение/выключение подсветки",
	{Type: model.ToggleCapabilityType, Instance: string(model.ControlsLockedToggleCapabilityInstance)}: "включение/выключение блокировки управления",
	{Type: model.ToggleCapabilityType, Instance: string(model.IonizationToggleCapabilityInstance)}:     "включение/выключение ионизации",
	{Type: model.ToggleCapabilityType, Instance: string(model.OscillationToggleCapabilityInstance)}:    "включение/выключение вращения",
	{Type: model.ToggleCapabilityType, Instance: string(model.KeepWarmToggleCapabilityInstance)}:       "включение/выключение поддержания тепла",
	{Type: model.ToggleCapabilityType, Instance: string(model.PauseToggleCapabilityInstance)}:          "включение/выключение паузы",
	{Type: model.ToggleCapabilityType, Instance: string(model.TrunkToggleCapabilityInstance)}:          "открытие/закрытие багажника",
	{Type: model.ToggleCapabilityType, Instance: string(model.CentralLockCapabilityInstance)}:          "открытие/закрытие центрального замка",

	{Type: model.ModeCapabilityType, Instance: string(model.FanSpeedModeInstance)}:    "изменение скорости вентилятора",
	{Type: model.ModeCapabilityType, Instance: string(model.ThermostatModeInstance)}:  "изменение режима термостата",
	{Type: model.ModeCapabilityType, Instance: string(model.SwingModeInstance)}:       "изменение направления воздуха",
	{Type: model.ModeCapabilityType, Instance: string(model.WorkSpeedModeInstance)}:   "изменение скорости работы",
	{Type: model.ModeCapabilityType, Instance: string(model.CleanUpModeInstance)}:     "изменение режима уборки",
	{Type: model.ModeCapabilityType, Instance: string(model.ProgramModeInstance)}:     "изменение программы работы",
	{Type: model.ModeCapabilityType, Instance: string(model.InputSourceModeInstance)}: "изменение источника сигнала",
	{Type: model.ModeCapabilityType, Instance: string(model.CoffeeModeInstance)}:      "изменение вида кофе",
	{Type: model.ModeCapabilityType, Instance: string(model.HeatModeInstance)}:        "изменение режима нагрева",
	{Type: model.ModeCapabilityType, Instance: string(model.DishwashingModeInstance)}: "изменение режима мытья посуды",
	{Type: model.ModeCapabilityType, Instance: string(model.TeaModeInstance)}:         "изменение режима приготовления чая",

	{Type: model.CustomButtonCapabilityType}: "обученная пользователем кнопка",

	{Type: model.QuasarServerActionCapabilityType, Instance: string(model.PhraseActionCapabilityInstance)}: "фраза для произношения вслух",
	{Type: model.QuasarServerActionCapabilityType, Instance: string(model.TextActionCapabilityInstance)}:   "команда для выполнения",

	{Type: model.QuasarCapabilityType, Instance: string(model.WeatherCapabilityInstance)}:        "прогноз погоды",
	{Type: model.QuasarCapabilityType, Instance: string(model.VolumeCapabilityInstance)}:         "управление громкостью",
	{Type: model.QuasarCapabilityType, Instance: string(model.MusicPlayCapabilityInstance)}:      "включение музыки",
	{Type: model.QuasarCapabilityType, Instance: string(model.NewsCapabilityInstance)}:           "сводка/лента новостей",
	{Type: model.QuasarCapabilityType, Instance: string(model.SoundPlayCapabilityInstance)}:      "проигрывание звука",
	{Type: model.QuasarCapabilityType, Instance: string(model.StopEverythingCapabilityInstance)}: "остановка всего",
	{Type: model.QuasarCapabilityType, Instance: string(model.TTSCapabilityInstance)}:            "текст, который произнесет колонка",
	{Type: model.QuasarCapabilityType, Instance: string(model.AliceShowCapabilityInstance)}:      "шоу Алисы",

	{Type: model.VideoStreamCapabilityType, Instance: string(model.GetStreamCapabilityInstance)}: "получение видеопотока с камеры",
}

func getAnalyticsName(key CapabilityUserInfoKey) string {
	if key.Type == model.CustomButtonCapabilityType {
		key.Instance = ""
	}
	return analyticsNamesMap[key]
}

var propertyAnalyticsNamesMap = map[PropertyUserInfoKey]string{
	{Type: model.FloatPropertyType, Instance: string(model.HumidityPropertyInstance)}:           "влажность",
	{Type: model.FloatPropertyType, Instance: string(model.TemperaturePropertyInstance)}:        "температура",
	{Type: model.FloatPropertyType, Instance: string(model.CO2LevelPropertyInstance)}:           "уровень CO2",
	{Type: model.FloatPropertyType, Instance: string(model.WaterLevelPropertyInstance)}:         "уровень воды",
	{Type: model.FloatPropertyType, Instance: string(model.AmperagePropertyInstance)}:           "ток",
	{Type: model.FloatPropertyType, Instance: string(model.VoltagePropertyInstance)}:            "напряжение",
	{Type: model.FloatPropertyType, Instance: string(model.PowerPropertyInstance)}:              "мощность",
	{Type: model.FloatPropertyType, Instance: string(model.PM1DensityPropertyInstance)}:         "уровень частиц PM1",
	{Type: model.FloatPropertyType, Instance: string(model.PM2p5DensityPropertyInstance)}:       "уровень частиц PM2.5",
	{Type: model.FloatPropertyType, Instance: string(model.PM10DensityPropertyInstance)}:        "уровень частиц PM10",
	{Type: model.FloatPropertyType, Instance: string(model.TvocPropertyInstance)}:               "уровень органических веществ",
	{Type: model.FloatPropertyType, Instance: string(model.PressurePropertyInstance)}:           "давление",
	{Type: model.FloatPropertyType, Instance: string(model.BatteryLevelPropertyInstance)}:       "уровень заряда",
	{Type: model.FloatPropertyType, Instance: string(model.TimerPropertyInstance)}:              "таймер",
	{Type: model.FloatPropertyType, Instance: string(model.IlluminationPropertyInstance)}:       "освещенность",
	{Type: model.FloatPropertyType, Instance: string(model.GasConcentrationPropertyInstance)}:   "концентрация газа",
	{Type: model.FloatPropertyType, Instance: string(model.SmokeConcentrationPropertyInstance)}: "концентрация дыма",

	{Type: model.EventPropertyType, Instance: string(model.OpenPropertyInstance)}:      "датчик открытия двери/окна",
	{Type: model.EventPropertyType, Instance: string(model.MotionPropertyInstance)}:    "датчик движения",
	{Type: model.EventPropertyType, Instance: string(model.VibrationPropertyInstance)}: "датчик вибрации",
	{Type: model.EventPropertyType, Instance: string(model.ButtonPropertyInstance)}:    "кнопка",
	{Type: model.EventPropertyType, Instance: string(model.SmokePropertyInstance)}:     "датчик дыма",
	{Type: model.EventPropertyType, Instance: string(model.GasPropertyInstance)}:       "датчик газа",
	{Type: model.EventPropertyType, Instance: string(model.WaterLeakPropertyInstance)}: "датчик протечки",
}

func getPropertyAnalyticsName(key PropertyUserInfoKey) string {
	return propertyAnalyticsNamesMap[key]
}
