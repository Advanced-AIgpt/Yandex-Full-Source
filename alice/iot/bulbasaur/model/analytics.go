package model

//overrides `DeviceType.GenerateDeviceName()` in order to be more toloka-friendly
var analyticsDeviceTypeNamesMap = map[DeviceType]string{
	LightDeviceType:      "Осветительный прибор",
	SwitchDeviceType:     "Выключатель или переключатель",
	HubDeviceType:        "Пульт для управления другими устройствами",
	CookingDeviceType:    "Кухонная техника",
	HumidifierDeviceType: "Увлажнитель воздуха",
	PurifierDeviceType:   "Очиститель воздуха",
}

func analyticsDeviceType(deviceType DeviceType) string {
	return deviceType.String()
}

func analyticsDeviceTypeName(deviceType DeviceType) string {
	v, ok := analyticsDeviceTypeNamesMap[deviceType]
	if !ok {
		v = deviceType.GenerateDeviceName()
	}
	if len(v) == 0 {
		return "Умное устройство"
	}
	return v
}

func analyticsGroupTypeName(deviceType DeviceType) string {
	if deviceType == SmartSpeakerDeviceType {
		return "Мультирум"
	}
	v, ok := analyticsDeviceTypeNamesMap[deviceType]
	if !ok {
		v = deviceType.GenerateDeviceName()
	}
	return v
}

var analyticsCapabilityNamesMap = map[analyticsCapabilityKey]string{
	{Type: OnOffCapabilityType, Instance: string(OnOnOffCapabilityInstance)}: "включение/выключение",

	{Type: ColorSettingCapabilityType, Instance: string(RgbColorCapabilityInstance)}:     "изменение цвета",
	{Type: ColorSettingCapabilityType, Instance: string(HsvColorCapabilityInstance)}:     "изменение цвета",
	{Type: ColorSettingCapabilityType, Instance: string(TemperatureKCapabilityInstance)}: "изменение цветовой температуры",
	{Type: ColorSettingCapabilityType, Instance: string(SceneCapabilityInstance)}:        "изменение режима освещения",

	{Type: RangeCapabilityType, Instance: string(BrightnessRangeInstance)}:  "изменение яркости",
	{Type: RangeCapabilityType, Instance: string(TemperatureRangeInstance)}: "изменение температуры",
	{Type: RangeCapabilityType, Instance: string(VolumeRangeInstance)}:      "изменение громкости звука",
	{Type: RangeCapabilityType, Instance: string(ChannelRangeInstance)}:     "изменение канала",
	{Type: RangeCapabilityType, Instance: string(HumidityRangeInstance)}:    "изменение влажности",
	{Type: RangeCapabilityType, Instance: string(OpenRangeInstance)}:        "изменение открытия",

	{Type: ToggleCapabilityType, Instance: string(MuteToggleCapabilityInstance)}:           "включение/выключение беззвучного режима",
	{Type: ToggleCapabilityType, Instance: string(BacklightToggleCapabilityInstance)}:      "включение/выключение подсветки",
	{Type: ToggleCapabilityType, Instance: string(ControlsLockedToggleCapabilityInstance)}: "включение/выключение блокировки управления",
	{Type: ToggleCapabilityType, Instance: string(IonizationToggleCapabilityInstance)}:     "включение/выключение ионизации",
	{Type: ToggleCapabilityType, Instance: string(OscillationToggleCapabilityInstance)}:    "включение/выключение вращения",
	{Type: ToggleCapabilityType, Instance: string(KeepWarmToggleCapabilityInstance)}:       "включение/выключение поддержания тепла",
	{Type: ToggleCapabilityType, Instance: string(PauseToggleCapabilityInstance)}:          "включение/выключение паузы",
	{Type: ToggleCapabilityType, Instance: string(TrunkToggleCapabilityInstance)}:          "открытие/закрытие багажника",
	{Type: ToggleCapabilityType, Instance: string(CentralLockCapabilityInstance)}:          "открытие/закрытие центрального замка",

	{Type: ModeCapabilityType, Instance: string(FanSpeedModeInstance)}:    "изменение скорости вентилятора",
	{Type: ModeCapabilityType, Instance: string(ThermostatModeInstance)}:  "изменение режима термостата",
	{Type: ModeCapabilityType, Instance: string(SwingModeInstance)}:       "изменение направления воздуха",
	{Type: ModeCapabilityType, Instance: string(WorkSpeedModeInstance)}:   "изменение скорости работы",
	{Type: ModeCapabilityType, Instance: string(CleanUpModeInstance)}:     "изменение режима уборки",
	{Type: ModeCapabilityType, Instance: string(ProgramModeInstance)}:     "изменение программы работы",
	{Type: ModeCapabilityType, Instance: string(InputSourceModeInstance)}: "изменение источника сигнала",
	{Type: ModeCapabilityType, Instance: string(CoffeeModeInstance)}:      "изменение вида кофе",
	{Type: ModeCapabilityType, Instance: string(HeatModeInstance)}:        "изменение режима нагрева",
	{Type: ModeCapabilityType, Instance: string(DishwashingModeInstance)}: "изменение режима мытья посуды",
	{Type: ModeCapabilityType, Instance: string(TeaModeInstance)}:         "изменение режима приготовления чая",

	{Type: QuasarServerActionCapabilityType, Instance: string(PhraseActionCapabilityInstance)}: "фраза для произнесения вслух",
	{Type: QuasarServerActionCapabilityType, Instance: string(TextActionCapabilityInstance)}:   "команда для выполнения",

	{Type: QuasarCapabilityType, Instance: string(WeatherCapabilityInstance)}:        "прогноз погоды",
	{Type: QuasarCapabilityType, Instance: string(VolumeCapabilityInstance)}:         "управление громкостью",
	{Type: QuasarCapabilityType, Instance: string(MusicPlayCapabilityInstance)}:      "включение музыки",
	{Type: QuasarCapabilityType, Instance: string(NewsCapabilityInstance)}:           "сводка/лента новостей",
	{Type: QuasarCapabilityType, Instance: string(SoundPlayCapabilityInstance)}:      "проигрывание звука",
	{Type: QuasarCapabilityType, Instance: string(StopEverythingCapabilityInstance)}: "остановка всего",
	{Type: QuasarCapabilityType, Instance: string(TTSCapabilityInstance)}:            "текст, который произнесет колонка",
	{Type: QuasarCapabilityType, Instance: string(AliceShowCapabilityInstance)}:      "шоу Алисы",

	{Type: VideoStreamCapabilityType, Instance: string(GetStreamCapabilityInstance)}: "получение видеопотока",
}

func analyticsCapabilityType(capabilityType CapabilityType) string {
	return capabilityType.String()
}

func analyticsCapabilityName(key analyticsCapabilityKey) string {
	return analyticsCapabilityNamesMap[key]
}

var analyticsPropertyNamesMap = map[analyticsPropertyKey]string{
	{Type: FloatPropertyType, Instance: string(HumidityPropertyInstance)}:           "влажность",
	{Type: FloatPropertyType, Instance: string(TemperaturePropertyInstance)}:        "температура",
	{Type: FloatPropertyType, Instance: string(CO2LevelPropertyInstance)}:           "уровень CO2",
	{Type: FloatPropertyType, Instance: string(WaterLevelPropertyInstance)}:         "уровень воды",
	{Type: FloatPropertyType, Instance: string(AmperagePropertyInstance)}:           "ток",
	{Type: FloatPropertyType, Instance: string(VoltagePropertyInstance)}:            "напряжение",
	{Type: FloatPropertyType, Instance: string(PowerPropertyInstance)}:              "мощность",
	{Type: FloatPropertyType, Instance: string(PM1DensityPropertyInstance)}:         "уровень частиц PM1",
	{Type: FloatPropertyType, Instance: string(PM2p5DensityPropertyInstance)}:       "уровень частиц PM2.5",
	{Type: FloatPropertyType, Instance: string(PM10DensityPropertyInstance)}:        "уровень частиц PM10",
	{Type: FloatPropertyType, Instance: string(TvocPropertyInstance)}:               "уровень органических веществ",
	{Type: FloatPropertyType, Instance: string(PressurePropertyInstance)}:           "давление",
	{Type: FloatPropertyType, Instance: string(BatteryLevelPropertyInstance)}:       "уровень заряда",
	{Type: FloatPropertyType, Instance: string(TimerPropertyInstance)}:              "таймер",
	{Type: FloatPropertyType, Instance: string(GasConcentrationPropertyInstance)}:   "концентрация газа",
	{Type: FloatPropertyType, Instance: string(SmokeConcentrationPropertyInstance)}: "концентрация дыма",

	{Type: EventPropertyType, Instance: string(OpenPropertyInstance)}:      "датчик открытия двери/окна",
	{Type: EventPropertyType, Instance: string(MotionPropertyInstance)}:    "датчик движения",
	{Type: EventPropertyType, Instance: string(VibrationPropertyInstance)}: "датчик вибрации",
	{Type: EventPropertyType, Instance: string(ButtonPropertyInstance)}:    "кнопка",
	{Type: EventPropertyType, Instance: string(SmokePropertyInstance)}:     "датчик дыма",
	{Type: EventPropertyType, Instance: string(GasPropertyInstance)}:       "датчик газа",
	{Type: EventPropertyType, Instance: string(WaterLeakPropertyInstance)}: "датчик протечки",
}

func analyticsPropertyType(propertyType PropertyType) string {
	return propertyType.String()
}

func analyticsPropertyName(key analyticsPropertyKey) string {
	return analyticsPropertyNamesMap[key]
}

type analyticsCapabilityKey struct {
	Type     CapabilityType
	Instance string
}

type analyticsPropertyKey struct {
	Type     PropertyType
	Instance string
}
