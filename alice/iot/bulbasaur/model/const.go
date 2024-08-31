package model

import (
	"regexp"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/common"
	devicepb "a.yandex-team.ru/alice/protos/data/device"
)

// Ё - любимая буквама Хозяина, он требует выделять ее отдельно
// она не входит в а-я :(
var nameRegex = regexp.MustCompile(`^(([а-яёА-ЯЁ]+)|(\d+))$`)
var quasarNameRegex = regexp.MustCompile(`^(([а-яёА-ЯЁ0-9\-]+)|(\d+)|([a-zA-Z0-9\-]+))$`)
var scenarioNameRegex = regexp.MustCompile(`^(([а-яёА-ЯЁ\-,!]+)|(\d+))$`)
var quasarServerActionRegex = regexp.MustCompile(`^[а-яёА-ЯЁa-zA-Z\-,!.:=?\d]+$`)
var onlyRusLettersRegex = regexp.MustCompile(`[а-яёА-ЯЁ]`)
var rusLatinLettersRegex = regexp.MustCompile(`[а-яёА-ЯЁ]|[a-zA-Z]`)

var KnownDeviceTypes []string
var KnownQuasarDeviceTypes []string
var KnownRemoteCarDeviceTypes []string
var KnownDeviceStatuses []string
var KnownCapabilityTypes CapabilityTypes
var KnownPropertyTypes []string
var KnownColorModels []string
var KnownScenarioTriggers []string
var KnownScenarioArtificialTriggers []string
var KnownScenarioIcons []string
var KnownRangeInstanceNames map[RangeCapabilityInstance]string
var KnownToggleInstanceNames map[ToggleCapabilityInstance]string
var KnownPropertyInstanceNames map[PropertyInstance]string
var KnownPropertyVoiceInstanceNames map[PropertyInstance]string
var KnownPropertyInstances []string
var KnownFloatPropertyInstances []string
var KnownEventPropertyInstances []string
var KnownModes = make(map[ModeValue]Mode)
var KnownModeInstancesNames = make(map[ModeCapabilityInstance]string)
var KnownRemoteCarToggleInstances []string
var KnownQuasarServerActionInstances []string
var KnownQuasarCapabilityInstances []string
var KnownQuasarCapabilityTypes CapabilityTypes
var KnownUnits = make(map[Unit]struct{})
var IntToNumericModeValue map[int]ModeValue
var KnownDeviceIconsURL map[DeviceType]string
var KnownScenarioIconsURL map[ScenarioIcon]string
var KnownColorScenes map[ColorSceneID]ColorScene
var MultiroomSpeakers map[DeviceType]bool
var StereopairAvailablePairs map[DeviceType]DeviceTypes // map[<leader device type>] []{<Allowed follower device type>,...}
var CallableSpeakers map[DeviceType]bool
var KnownQuasarPlatforms map[DeviceType]QuasarPlatform
var KnownScenarioStepTypes []string
var KnownScenarioLaunchDeviceActionStatuses []string
var MultistepScenarioSpeakers map[DeviceType]bool
var KnownSpeakerNewsTopics []string
var KnownSpeakerSounds map[SpeakerSoundID]SpeakerSound
var RequireXTokenSpeakers map[DeviceType]bool
var VoiceprintSpeakers map[DeviceType]bool
var VoiceprintViaSoundSpeakers map[DeviceType]bool

var KnownEvents = make(map[EventKey]Event)

var MultiDeltaRangeInstances RangeCapabilityInstances
var CapabilityRelativeDeltaBuckets = make(map[RangeCapabilityInstance]float64)
var NonSupportingIntervalActionsDeviceTypes []string
var NonSupportingIntervalActionsCapabilities []string

var KnownInternalProviders []string
var KnownFavoriteTypes []string

var KnownYandexmidiColorScenes []string

// Map for checking device type is switchable
// Value - list of DeviceType converted to string []string(DeviceType) to allow using tools.Contains(string, []string)
var DeviceSwitchTypeMap = make(map[DeviceType][]string)

var EventPropertyInstanceToActivationEventValues = make(map[PropertyInstance]EventValues)

var tandemDisplayCompatibleSpeakerTypes map[DeviceType]DeviceTypes
var tandemSpeakerCompatibleDisplayTypes map[DeviceType]DeviceTypes

var deviceTypeDefaultNameMap = make(map[DeviceType]string)

var modesSortingMap = make(map[ModeValue]int)
var capabilitySortingMap = make(map[CapabilityType]int)
var propertySortingMap = make(map[PropertyType]int)

// while these are not const, as we need to modify them in tests in order not to suffer
// we all need to know, that any other modifications of these variables is prohibited
// and are punished by death
var ConstDeviceLimit uint64 = 301
var ConstUserNetworksLimit uint64 = 10
var ConstSharingUsersLimit = 4

const DeviceNameAliasesLimit = 4 // it means the number of additional names, i.e. total names limit is 1 + DeviceNameAliasesLimit
const GroupNameAliasesLimit = 5

var protoToDeviceTypeMap = map[protos.DeviceType]DeviceType{} //filled by init()
var protoToCapabilityTypeMap = map[protos.CapabilityType]CapabilityType{}
var protoToPropertyTypeMap = map[protos.PropertyType]PropertyType{}
var protoToColorModelTypeMap = map[protos.ColorModelType]ColorModelType{}

const (
	DeviceNameLength       int = 25
	QuasarDeviceNameLength int = 50

	GroupNameLength     int = 40
	RoomNameLength      int = 20
	HouseholdNameLength int = 20
)

const (
	DeleteReasonUnlink string = "unlink"
	DeleteReasonMobile string = "mobile"
)

type AliceResponseReactionType string

const (
	SoundAliceResponseReactionType AliceResponseReactionType = "sound"
	NLGAliceResponseReactionType   AliceResponseReactionType = "nlg"
)

type RequestSource string // note: this should be merged into Origin, discussion https://a.yandex-team.ru/review/2203258/details#comment-2996329

const (
	SearchApplicationSource RequestSource = "mobile"
	WidgetSource            RequestSource = "widget"
	ExternalAPISource       RequestSource = "api"
	SteelixSource           RequestSource = "steelix"
	TimeMachineSource       RequestSource = "time_machine"
	AliceSource             RequestSource = "alice"
)

var (
	KnownNeuterPropertyInstanceNames map[string]bool
	KnownFemalePropertyInstanceNames map[string]bool
	KnownMalePropertyInstanceNames   map[string]bool
)

func init() {
	KnownDeviceTypes = []string{
		string(LightDeviceType), string(LightCeilingDeviceType), string(LightLampDeviceType), string(LightStripDeviceType),
		string(SocketDeviceType), string(SwitchDeviceType), string(HubDeviceType),
		string(CookingDeviceType), string(KettleDeviceType), string(ThermostatDeviceType), string(AcDeviceType),
		string(MediaDeviceDeviceType), string(TvDeviceDeviceType), string(ReceiverDeviceType), string(TvBoxDeviceType),
		string(OtherDeviceType), string(OpenableDeviceType), string(CurtainDeviceType), string(PurifierDeviceType),
		string(HumidifierDeviceType), string(CoffeeMakerDeviceType), string(VacuumCleanerDeviceType), string(WashingMachineDeviceType),
		string(DishwasherDeviceType), string(MulticookerDeviceType), string(RefrigeratorDeviceType), string(FanDeviceType),
		string(IronDeviceType), string(SensorDeviceType), string(PetFeederDeviceType), string(CameraDeviceType),
	}
	KnownQuasarDeviceTypes = []string{
		string(SmartSpeakerDeviceType), string(YandexStationDeviceType), string(YandexStation2DeviceType), string(YandexStationMiniDeviceType),
		string(YandexStationMini2DeviceType), string(YandexStationMini2NoClockDeviceType), string(DexpSmartBoxDeviceType),
		string(IrbisADeviceType), string(ElariSmartBeatDeviceType), string(LGXBoomDeviceType), string(JetSmartMusicDeviceType),
		string(PrestigioSmartMateDeviceType), string(DigmaDiHomeDeviceType), string(JBLLinkPortableDeviceType),
		string(JBLLinkMusicDeviceType), string(YandexModuleDeviceType), string(YandexModule2DeviceType),
		string(YandexStationMicroDeviceType), string(YandexStationMidiDeviceType),
		string(YandexStationCentaurDeviceType), string(YandexStationChironDeviceType), string(YandexStationPholDeviceType),
	}

	KnownRemoteCarDeviceTypes = []string{
		string(RemoteCarDeviceType),
	}

	KnownDeviceTypes = append(KnownDeviceTypes, KnownQuasarDeviceTypes...)
	KnownDeviceTypes = append(KnownDeviceTypes, KnownRemoteCarDeviceTypes...)

	KnownDeviceStatuses = []string{
		string(OnlineDeviceStatus),
		string(OfflineDeviceStatus),
		string(UnknownDeviceStatus),
		string(NotFoundDeviceStatus),
	}

	KnownCapabilityTypes = CapabilityTypes{
		OnOffCapabilityType, ColorSettingCapabilityType, ModeCapabilityType,
		RangeCapabilityType, ToggleCapabilityType, CustomButtonCapabilityType,
		VideoStreamCapabilityType,
	}
	KnownQuasarCapabilityTypes = CapabilityTypes{
		QuasarServerActionCapabilityType, QuasarCapabilityType,
	}
	KnownCapabilityTypes = append(KnownCapabilityTypes, KnownQuasarCapabilityTypes...)

	KnownPropertyTypes = []string{
		string(FloatPropertyType),
		string(EventPropertyType),
	}
	KnownColorModels = []string{string(HsvModelType), string(RgbModelType)}
	KnownScenarioTriggers = []string{string(VoiceScenarioTriggerType), string(TimerScenarioTriggerType), string(PropertyScenarioTriggerType), string(TimetableScenarioTriggerType)}
	KnownScenarioArtificialTriggers = []string{string(AppScenarioTriggerType), string(APIScenarioTriggerType)}
	KnownScenarioIcons = []string{
		string(ScenarioIconMorning), string(ScenarioIconDay), string(ScenarioIconEvening), string(ScenarioIconNight),
		string(ScenarioIconGame), string(ScenarioIconSport), string(ScenarioIconCleaning), string(ScenarioIconHome), string(ScenarioIconSofa),
		string(ScenarioIconWork), string(ScenarioIconFlowers), string(ScenarioIconDrink), string(ScenarioIconAlarm), string(ScenarioIconParty),
		string(ScenarioIconRomantic), string(ScenarioIconCooking), string(ScenarioIconBall), string(ScenarioIconTree),
		string(ScenarioIconPresent), string(ScenarioIconToy), string(ScenarioIconSock), string(ScenarioIconStar),
		string(ScenarioIconSnowflake), string(ScenarioIconLamp), string(ScenarioIconTV), string(ScenarioIconStation),
		string(ScenarioIconToggle), string(ScenarioIconSocket), string(ScenarioIconMusic), string(ScenarioIconLamps),
		string(ScenarioIconHumidity), string(ScenarioIconSecurity), string(ScenarioIconDoor), string(ScenarioIconDaynight),
		string(ScenarioIconWaterleak), string(ScenarioIconCo2), string(ScenarioIconPuppy), string(ScenarioIconCastle),
		string(ScenarioIconWindow), string(ScenarioIconAc), string(ScenarioIconMotion), string(ScenarioIconGas),
		string(ScenarioIconTemperature), string(ScenarioIconHeater),
	}
	KnownRangeInstanceNames = map[RangeCapabilityInstance]string{
		VolumeRangeInstance:      "громкость",
		BrightnessRangeInstance:  "яркость",
		TemperatureRangeInstance: "температура",
		ChannelRangeInstance:     "канал",
		HumidityRangeInstance:    "влажность",
		OpenRangeInstance:        "открытие",
	}
	KnownToggleInstanceNames = map[ToggleCapabilityInstance]string{
		MuteToggleCapabilityInstance:           "без звука",
		BacklightToggleCapabilityInstance:      "подсветка",
		ControlsLockedToggleCapabilityInstance: "блокировка управления",
		IonizationToggleCapabilityInstance:     "ионизация",
		OscillationToggleCapabilityInstance:    "вращение",
		KeepWarmToggleCapabilityInstance:       "поддержание тепла",
		PauseToggleCapabilityInstance:          "пауза",
		TrunkToggleCapabilityInstance:          "багажник",
		CentralLockCapabilityInstance:          "центральный замок",
	}
	KnownPropertyInstanceNames = map[PropertyInstance]string{
		HumidityPropertyInstance:           "влажность",
		TemperaturePropertyInstance:        "температура",
		CO2LevelPropertyInstance:           "уровень углекислого газа",
		WaterLevelPropertyInstance:         "уровень воды",
		AmperagePropertyInstance:           "потребление тока",
		VoltagePropertyInstance:            "текущее напряжение",
		PowerPropertyInstance:              "потребляемая мощность",
		PM1DensityPropertyInstance:         "уровень частиц PM1",
		PM2p5DensityPropertyInstance:       "уровень частиц PM2.5",
		PM10DensityPropertyInstance:        "уровень частиц PM10",
		TvocPropertyInstance:               "уровень органических веществ",
		PressurePropertyInstance:           "давление",
		BatteryLevelPropertyInstance:       "уровень заряда",
		TimerPropertyInstance:              "таймер",
		IlluminationPropertyInstance:       "освещенность",
		VibrationPropertyInstance:          "вибрация",
		OpenPropertyInstance:               "дверь/окно",
		ButtonPropertyInstance:             "кнопка",
		MotionPropertyInstance:             "движение",
		SmokePropertyInstance:              "дым",
		SmokeConcentrationPropertyInstance: "концентрация дыма",
		GasPropertyInstance:                "газ",
		GasConcentrationPropertyInstance:   "концентрация газа",
		WaterLeakPropertyInstance:          "протечка воды",

		// IMPORTANT!
		// when adding new instances and instance names, you should specify its AGR gender
		// in KnownFemalePropertyInstanceNames, KnownMalePropertyInstanceNames, or KnownNeuterPropertyInstanceNames
	}
	KnownPropertyVoiceInstanceNames = make(map[PropertyInstance]string, len(KnownPropertyInstanceNames))
	for instanceName, value := range KnownPropertyInstanceNames {
		KnownPropertyVoiceInstanceNames[instanceName] = value
	}
	KnownPropertyVoiceInstanceNames[PM2p5DensityPropertyInstance] = "уровень частиц PM 2 с половиной"

	KnownFemalePropertyInstanceNames = map[string]bool{
		string(HumidityPropertyInstance):           true,
		string(TemperaturePropertyInstance):        true,
		string(IlluminationPropertyInstance):       true,
		string(GasConcentrationPropertyInstance):   true,
		string(SmokeConcentrationPropertyInstance): true,
	}
	KnownMalePropertyInstanceNames = map[string]bool{
		string(CO2LevelPropertyInstance):     true,
		string(WaterLevelPropertyInstance):   true,
		string(PM1DensityPropertyInstance):   true,
		string(PM2p5DensityPropertyInstance): true,
		string(PM10DensityPropertyInstance):  true,
		string(TvocPropertyInstance):         true,
		string(BatteryLevelPropertyInstance): true,
		string(TimerPropertyInstance):        true,
	}
	KnownNeuterPropertyInstanceNames = map[string]bool{
		string(AmperagePropertyInstance): true,
		string(VoltagePropertyInstance):  true,
		string(PowerPropertyInstance):    true,
		string(PressurePropertyInstance): true,
	}

	KnownEventPropertyInstances = []string{
		string(VibrationPropertyInstance),
		string(OpenPropertyInstance),
		string(ButtonPropertyInstance),
		string(MotionPropertyInstance),
		string(SmokePropertyInstance),
		string(GasPropertyInstance),
		string(BatteryLevelPropertyInstance),
		string(WaterLevelPropertyInstance),
		string(WaterLeakPropertyInstance),
	}
	KnownFloatPropertyInstances = []string{
		string(HumidityPropertyInstance),
		string(TemperaturePropertyInstance),
		string(CO2LevelPropertyInstance),
		string(WaterLevelPropertyInstance),
		string(AmperagePropertyInstance),
		string(VoltagePropertyInstance),
		string(PowerPropertyInstance),
		string(PM1DensityPropertyInstance),
		string(PM2p5DensityPropertyInstance),
		string(PM10DensityPropertyInstance),
		string(TvocPropertyInstance),
		string(PressurePropertyInstance),
		string(BatteryLevelPropertyInstance),
		string(TimerPropertyInstance),
		string(IlluminationPropertyInstance),
		string(GasConcentrationPropertyInstance),
		string(SmokeConcentrationPropertyInstance),
	}
	KnownPropertyInstances = append(KnownPropertyInstances, KnownEventPropertyInstances...)
	KnownPropertyInstances = append(KnownPropertyInstances, KnownFloatPropertyInstances...)

	KnownRemoteCarToggleInstances = []string{
		string(TrunkToggleCapabilityInstance),
		string(CentralLockCapabilityInstance),
	}
	KnownQuasarServerActionInstances = []string{
		string(PhraseActionCapabilityInstance),
		string(TextActionCapabilityInstance),
	}

	KnownQuasarCapabilityInstances = []string{
		string(WeatherCapabilityInstance),
		string(VolumeCapabilityInstance),
		string(MusicPlayCapabilityInstance),
		string(NewsCapabilityInstance),
		string(SoundPlayCapabilityInstance),
		string(StopEverythingCapabilityInstance),
		string(TTSCapabilityInstance),
		string(AliceShowCapabilityInstance),
	}

	KnownModes = map[ModeValue]Mode{
		TurboMode:          {Value: TurboMode, Name: tools.AOS("Турбо")},
		FastMode:           {Value: FastMode, Name: tools.AOS("Быстрый")},
		SlowMode:           {Value: SlowMode, Name: tools.AOS("Медленный")},
		ExpressMode:        {Value: ExpressMode, Name: tools.AOS("Экспресс")},
		QuietMode:          {Value: QuietMode, Name: tools.AOS("Тихий")},
		NormalMode:         {Value: NormalMode, Name: tools.AOS("Нормальный")},
		PreHeatMode:        {Value: PreHeatMode, Name: tools.AOS("Подогрев")},
		MaxMode:            {Value: MaxMode, Name: tools.AOS("Максимальный")},
		MinMode:            {Value: MinMode, Name: tools.AOS("Минимальный")},
		HorizontalMode:     {Value: HorizontalMode, Name: tools.AOS("Горизонтальный")},
		VerticalMode:       {Value: VerticalMode, Name: tools.AOS("Вертикальный")},
		StationaryMode:     {Value: StationaryMode, Name: tools.AOS("Статичный")},
		LatteMode:          {Value: LatteMode, Name: tools.AOS("Латте")},
		CappuccinoMode:     {Value: CappuccinoMode, Name: tools.AOS("Капучино")},
		EspressoMode:       {Value: EspressoMode, Name: tools.AOS("Эспрессо")},
		DoubleEspressoMode: {Value: DoubleEspressoMode, Name: tools.AOS("Двойной Эспрессо")},
		AmericanoMode:      {Value: AmericanoMode, Name: tools.AOS("Американо")},
		WindFreeMode:       {Value: WindFreeMode, Name: tools.AOS("WindFree")},

		OneMode:   {Value: OneMode, Name: tools.AOS("Один")},
		TwoMode:   {Value: TwoMode, Name: tools.AOS("Два")},
		ThreeMode: {Value: ThreeMode, Name: tools.AOS("Три")},
		FourMode:  {Value: FourMode, Name: tools.AOS("Четыре")},
		FiveMode:  {Value: FiveMode, Name: tools.AOS("Пять")},
		SixMode:   {Value: SixMode, Name: tools.AOS("Шесть")},
		SevenMode: {Value: SevenMode, Name: tools.AOS("Семь")},
		EightMode: {Value: EightMode, Name: tools.AOS("Восемь")},
		NineMode:  {Value: NineMode, Name: tools.AOS("Девять")},
		TenMode:   {Value: TenMode, Name: tools.AOS("Десять")},
		// washing machine modes
		WoolMode: {Value: WoolMode, Name: tools.AOS("Шерсть")},
		// old ac work modes
		HeatMode:    {Value: HeatMode, Name: tools.AOS("Нагрев")},
		CoolMode:    {Value: CoolMode, Name: tools.AOS("Охлаждение")},
		AutoMode:    {Value: AutoMode, Name: tools.AOS("Авто")},
		EcoMode:     {Value: EcoMode, Name: tools.AOS("Эко")},
		DryMode:     {Value: DryMode, Name: tools.AOS("Осушение")},
		FanOnlyMode: {Value: FanOnlyMode, Name: tools.AOS("Вентиляция")},
		// old fan modes names except auto because of collision
		LowMode:    {Value: LowMode, Name: tools.AOS("Низкая")},
		MediumMode: {Value: MediumMode, Name: tools.AOS("Средняя")},
		HighMode:   {Value: HighMode, Name: tools.AOS("Высокая")},

		// multicooker modes
		VacuumMode:       {Value: VacuumMode, Name: tools.AOS("Вакуум")},
		BoilingMode:      {Value: BoilingMode, Name: tools.AOS("Варка")},
		BakingMode:       {Value: BakingMode, Name: tools.AOS("Выпечка")},
		DessertMode:      {Value: DessertMode, Name: tools.AOS("Десерты")},
		BabyFoodMode:     {Value: BabyFoodMode, Name: tools.AOS("Детское питание")},
		FowlMode:         {Value: FowlMode, Name: tools.AOS("Дичь")},
		FryingMode:       {Value: FryingMode, Name: tools.AOS("Жарка")},
		YogurtMode:       {Value: YogurtMode, Name: tools.AOS("Йогурт")},
		CerealsMode:      {Value: CerealsMode, Name: tools.AOS("Крупы")},
		MacaroniMode:     {Value: MacaroniMode, Name: tools.AOS("Макароны")},
		MilkPorridgeMode: {Value: MilkPorridgeMode, Name: tools.AOS("Молочная каша")},
		MulticookerMode:  {Value: MulticookerMode, Name: tools.AOS("Мультиповар")},
		SteamMode:        {Value: SteamMode, Name: tools.AOS("Пар")},
		PastaMode:        {Value: PastaMode, Name: tools.AOS("Паста")},
		PizzaMode:        {Value: PizzaMode, Name: tools.AOS("Пицца")},
		PilafMode:        {Value: PilafMode, Name: tools.AOS("Плов")},
		SauceMode:        {Value: SauceMode, Name: tools.AOS("Соус")},
		SoupMode:         {Value: SoupMode, Name: tools.AOS("Суп")},
		StewingMode:      {Value: StewingMode, Name: tools.AOS("Тушение")},
		SlowCookMode:     {Value: SlowCookMode, Name: tools.AOS("Томление")},
		DeepFryerMode:    {Value: DeepFryerMode, Name: tools.AOS("Фритюр")},
		BreadMode:        {Value: BreadMode, Name: tools.AOS("Хлеб")},
		AspicMode:        {Value: AspicMode, Name: tools.AOS("Холодец")},
		CheesecakeMode:   {Value: CheesecakeMode, Name: tools.AOS("Чизкейк")},

		// dishwashing modes
		Auto45Mode:    {Value: Auto45Mode, Name: tools.AOS("Авто 45")},
		Auto60Mode:    {Value: Auto60Mode, Name: tools.AOS("Авто 60")},
		Auto75Mode:    {Value: Auto75Mode, Name: tools.AOS("Авто 75")},
		Fast45Mode:    {Value: Fast45Mode, Name: tools.AOS("Быстро 45")},
		Fast60Mode:    {Value: Fast60Mode, Name: tools.AOS("Быстро 60")},
		Fast75Mode:    {Value: Fast75Mode, Name: tools.AOS("Быстро 75")},
		PreRinseMode:  {Value: PreRinseMode, Name: tools.AOS("Ополаскивание")},
		IntensiveMode: {Value: IntensiveMode, Name: tools.AOS("Интенсивная")},
		GlassMode:     {Value: GlassMode, Name: tools.AOS("Стекло")},

		// tea modes
		BlackTeaMode:  {Value: BlackTeaMode, Name: tools.AOS("Черный чай")},
		GreenTeaMode:  {Value: GreenTeaMode, Name: tools.AOS("Зеленый чай")},
		PuerhTeaMode:  {Value: PuerhTeaMode, Name: tools.AOS("Чай пуэр")},
		WhiteTeaMode:  {Value: WhiteTeaMode, Name: tools.AOS("Белый чай")},
		OolongTeaMode: {Value: OolongTeaMode, Name: tools.AOS("Чай улун")},
		RedTeaMode:    {Value: RedTeaMode, Name: tools.AOS("Красный чай")},
		HerbalTeaMode: {Value: HerbalTeaMode, Name: tools.AOS("Травяной чай")},
		FlowerTeaMode: {Value: FlowerTeaMode, Name: tools.AOS("Цветочный чай")},
	}
	KnownModeInstancesNames = map[ModeCapabilityInstance]string{
		ThermostatModeInstance:  "термостат",
		FanSpeedModeInstance:    "скорость вентиляции",
		WorkSpeedModeInstance:   "скорость работы",
		CleanUpModeInstance:     "уборка",
		ProgramModeInstance:     "программа",
		InputSourceModeInstance: "источник сигнала",
		CoffeeModeInstance:      "кофе",
		SwingModeInstance:       "направление воздуха",
		HeatModeInstance:        "нагрев",
		DishwashingModeInstance: "мойка посуды",
		TeaModeInstance:         "чай",
	}
	DeviceSwitchTypeMap[SocketDeviceType] = []string{string(SocketDeviceType), string(LightDeviceType)}
	DeviceSwitchTypeMap[SwitchDeviceType] = []string{string(SwitchDeviceType), string(LightDeviceType)}

	KnownUnits = map[Unit]struct{}{
		UnitPercent:            {},
		UnitTemperatureCelsius: {},
		UnitTemperatureKelvin:  {},
		UnitPPM:                {},
		UnitAmpere:             {},
		UnitVolt:               {},
		UnitWatt:               {},
		UnitDensityMcgM3:       {},
		UnitPressureAtm:        {},
		UnitPressurePascal:     {},
		UnitPressureBar:        {},
		UnitPressureMmHg:       {},
		UnitTimeSeconds:        {},
		UnitIlluminationLux:    {},
	}
	MultiDeltaRangeInstances = []RangeCapabilityInstance{VolumeRangeInstance, TemperatureRangeInstance, BrightnessRangeInstance, OpenRangeInstance}

	CapabilityRelativeDeltaBuckets = map[RangeCapabilityInstance]float64{
		VolumeRangeInstance:      33.0,
		TemperatureRangeInstance: 33.0,
	}

	// some priorities for ModesSorting
	modesSortingMap = map[ModeValue]int{
		CoolMode:    -7,
		HeatMode:    -6,
		FanOnlyMode: -5,
		DryMode:     -4,
		AutoMode:    -3,
		LowMode:     -2,
		MediumMode:  -1,
		HighMode:    0,
		OneMode:     1,
		TwoMode:     2,
		ThreeMode:   3,
		FourMode:    4,
		FiveMode:    5,
		SixMode:     6,
		SevenMode:   7,
		EightMode:   8,
		NineMode:    9,
		TenMode:     10,
	}

	// some priorities for CapabilitySorting
	capabilitySortingMap = map[CapabilityType]int{
		OnOffCapabilityType:              1,
		ColorSettingCapabilityType:       2,
		RangeCapabilityType:              3,
		ModeCapabilityType:               4,
		ToggleCapabilityType:             5,
		CustomButtonCapabilityType:       6,
		QuasarServerActionCapabilityType: 7,
		QuasarCapabilityType:             8,
		VideoStreamCapabilityType:        9,
	}

	// some priorities for PropertySorting
	propertySortingMap = map[PropertyType]int{
		EventPropertyType: 1,
		FloatPropertyType: 2,
	}

	IntToNumericModeValue = map[int]ModeValue{
		1:  OneMode,
		2:  TwoMode,
		3:  ThreeMode,
		4:  FourMode,
		5:  FiveMode,
		6:  SixMode,
		7:  SevenMode,
		8:  EightMode,
		9:  NineMode,
		10: TenMode,
	}

	NonSupportingIntervalActionsDeviceTypes = []string{
		string(PetFeederDeviceType),
		string(AcDeviceType),
	}

	NonSupportingIntervalActionsCapabilities = []string{
		string(VideoStreamCapabilityType),
	}

	KnownInternalProviders = []string{QUASAR, REMOTECAR, YANDEXIO}
	KnownFavoriteTypes = []string{string(DeviceFavoriteType), string(DevicePropertyFavoriteType), string(ScenarioFavoriteType), string(GroupFavoriteType)}

	KnownYandexmidiColorScenes = []string{string(ColorSceneIDLavaLamp), string(ColorSceneIDCandle), string(ColorSceneIDNight), string(ColorSceneIDInactive)}

	KnownScenarioLaunchDeviceActionStatuses = []string{string(DoneScenarioLaunchDeviceActionStatus), string(ErrorScenarioLaunchDeviceActionStatus)}

	deviceTypeDefaultNameMap = map[DeviceType]string{
		AcDeviceType:             "Кондиционер",
		CoffeeMakerDeviceType:    "Кофеварка",
		CookingDeviceType:        "Кухонный прибор",
		CurtainDeviceType:        "Шторы",
		DishwasherDeviceType:     "Посудомоечная машина",
		FanDeviceType:            "Вентилятор",
		HubDeviceType:            "Хаб",
		HumidifierDeviceType:     "Увлажнитель",
		IronDeviceType:           "Утюг",
		KettleDeviceType:         "Чайник",
		LightDeviceType:          "Осветительный прибор",
		LightCeilingDeviceType:   "Люстра",
		LightLampDeviceType:      "Лампа",
		LightStripDeviceType:     "Лента",
		MediaDeviceDeviceType:    "Медиаустройство",
		MulticookerDeviceType:    "Мультиварка",
		OpenableDeviceType:       "Открывающееся устройство",
		SmartSpeakerDeviceType:   "Умная колонка",
		OtherDeviceType:          "Умное устройство",
		PetFeederDeviceType:      "Кормушка",
		PurifierDeviceType:       "Очиститель",
		ReceiverDeviceType:       "Ресивер",
		RefrigeratorDeviceType:   "Холодильник",
		SocketDeviceType:         "Розетка",
		SwitchDeviceType:         "Выключатель",
		ThermostatDeviceType:     "Термостат",
		TvBoxDeviceType:          "ТВ приставка",
		TvDeviceDeviceType:       "Телевизор",
		VacuumCleanerDeviceType:  "Пылесос",
		WashingMachineDeviceType: "Стиральная машина",
		RemoteCarDeviceType:      "Автомобиль",
		SensorDeviceType:         "Датчик",
		CameraDeviceType:         "Камера видеонаблюдения",
	}

	KnownScenarioIconsURL = map[ScenarioIcon]string{
		ScenarioIconMorning:     "https://avatars.mds.yandex.net/get-iot/icons-scenarios-morning.svg",
		ScenarioIconDay:         "https://avatars.mds.yandex.net/get-iot/icons-scenarios-day.svg",
		ScenarioIconEvening:     "https://avatars.mds.yandex.net/get-iot/icons-scenarios-evening.svg",
		ScenarioIconNight:       "https://avatars.mds.yandex.net/get-iot/icons-scenarios-night.svg",
		ScenarioIconGame:        "https://avatars.mds.yandex.net/get-iot/icons-scenarios-game.svg",
		ScenarioIconSport:       "https://avatars.mds.yandex.net/get-iot/icons-scenarios-sport.svg",
		ScenarioIconCleaning:    "https://avatars.mds.yandex.net/get-iot/icons-scenarios-cleaning.svg",
		ScenarioIconHome:        "https://avatars.mds.yandex.net/get-iot/icons-scenarios-home.svg",
		ScenarioIconSofa:        "https://avatars.mds.yandex.net/get-iot/icons-scenarios-sofa.svg",
		ScenarioIconWork:        "https://avatars.mds.yandex.net/get-iot/icons-scenarios-work.svg",
		ScenarioIconFlowers:     "https://avatars.mds.yandex.net/get-iot/icons-scenarios-flowers.svg",
		ScenarioIconDrink:       "https://avatars.mds.yandex.net/get-iot/icons-scenarios-drink.svg",
		ScenarioIconAlarm:       "https://avatars.mds.yandex.net/get-iot/icons-scenarios-alarm.svg",
		ScenarioIconParty:       "https://avatars.mds.yandex.net/get-iot/icons-scenarios-party.svg",
		ScenarioIconRomantic:    "https://avatars.mds.yandex.net/get-iot/icons-scenarios-romantic.svg",
		ScenarioIconCooking:     "https://avatars.mds.yandex.net/get-iot/icons-scenarios-cooking.svg",
		ScenarioIconPresent:     "https://avatars.mds.yandex.net/get-iot/icons-scenarios-present.svg",
		ScenarioIconStar:        "https://avatars.mds.yandex.net/get-iot/icons-scenarios-star.svg",
		ScenarioIconTree:        "https://avatars.mds.yandex.net/get-iot/icons-scenarios-tree.svg",
		ScenarioIconSnowflake:   "https://avatars.mds.yandex.net/get-iot/icons-scenarios-snowflake.svg",
		ScenarioIconToy:         "https://avatars.mds.yandex.net/get-iot/icons-scenarios-toy.svg",
		ScenarioIconBall:        "https://avatars.mds.yandex.net/get-iot/icons-scenarios-ball.svg",
		ScenarioIconSock:        "https://avatars.mds.yandex.net/get-iot/icons-scenarios-sock.svg",
		ScenarioIconLamp:        "https://avatars.mds.yandex.net/get-iot/icons-scenarios-lamp.svg",
		ScenarioIconTV:          "https://avatars.mds.yandex.net/get-iot/icons-scenarios-tv.svg",
		ScenarioIconStation:     "https://avatars.mds.yandex.net/get-iot/icons-scenarios-station.svg",
		ScenarioIconToggle:      "https://avatars.mds.yandex.net/get-iot/icons-scenarios-toggle.svg",
		ScenarioIconSocket:      "https://avatars.mds.yandex.net/get-iot/icons-scenarios-socket.svg",
		ScenarioIconMusic:       "https://avatars.mds.yandex.net/get-iot/icons-scenarios-music.svg",
		ScenarioIconLamps:       "https://avatars.mds.yandex.net/get-iot/icons-scenarios-lamps.svg",
		ScenarioIconHumidity:    "https://avatars.mds.yandex.net/get-iot/icons-scenarios-humidity.svg",
		ScenarioIconSecurity:    "https://avatars.mds.yandex.net/get-iot/icons-scenarios-security.svg",
		ScenarioIconDoor:        "https://avatars.mds.yandex.net/get-iot/icons-scenarios-door.svg",
		ScenarioIconDaynight:    "https://avatars.mds.yandex.net/get-iot/icons-scenarios-daynight.svg",
		ScenarioIconWaterleak:   "https://avatars.mds.yandex.net/get-iot/icons-scenarios-waterleak.svg",
		ScenarioIconCo2:         "https://avatars.mds.yandex.net/get-iot/icons-scenarios-co2.svg",
		ScenarioIconPuppy:       "https://avatars.mds.yandex.net/get-iot/icons-scenarios-puppy.svg",
		ScenarioIconCastle:      "https://avatars.mds.yandex.net/get-iot/icons-scenarios-castle.svg",
		ScenarioIconWindow:      "https://avatars.mds.yandex.net/get-iot/icons-scenarios-window.svg",
		ScenarioIconAc:          "https://avatars.mds.yandex.net/get-iot/icons-scenarios-ac.svg",
		ScenarioIconMotion:      "https://avatars.mds.yandex.net/get-iot/icons-scenarios-motion.svg",
		ScenarioIconGas:         "https://avatars.mds.yandex.net/get-iot/icons-scenarios-gas.svg",
		ScenarioIconTemperature: "https://avatars.mds.yandex.net/get-iot/icons-scenarios-temperature.svg",
		ScenarioIconHeater:      "https://avatars.mds.yandex.net/get-iot/icons-scenarios-heater.svg",
	}

	KnownDeviceIconsURL = map[DeviceType]string{
		// svg
		LightDeviceType:          "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.light.svg",
		LightCeilingDeviceType:   "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.light.ceiling.svg",
		LightLampDeviceType:      "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.light.lamp.svg",
		LightStripDeviceType:     "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.light.strip.svg",
		SocketDeviceType:         "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.socket.svg",
		SwitchDeviceType:         "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.switch.svg",
		HubDeviceType:            "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.hub.svg",
		PurifierDeviceType:       "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.purifier.svg",
		HumidifierDeviceType:     "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.humidifier.svg",
		VacuumCleanerDeviceType:  "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.vacuum_cleaner.svg",
		CookingDeviceType:        "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.cooking.svg",
		KettleDeviceType:         "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.cooking.kettle.svg",
		CoffeeMakerDeviceType:    "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.cooking.coffee_maker.svg",
		MulticookerDeviceType:    "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.cooking.multicooker.svg",
		ThermostatDeviceType:     "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.thermostat.svg",
		AcDeviceType:             "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.thermostat.ac.svg",
		MediaDeviceDeviceType:    "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.media_device.svg",
		TvDeviceDeviceType:       "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.media_device.tv.svg",
		ReceiverDeviceType:       "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.media_device.receiver.svg",
		TvBoxDeviceType:          "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.media_device.tv_box.svg",
		WashingMachineDeviceType: "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.washing_machine.svg",
		DishwasherDeviceType:     "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.dishwasher.svg",
		RefrigeratorDeviceType:   "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.refrigerator.svg",
		FanDeviceType:            "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.fan.svg",
		IronDeviceType:           "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.iron.svg",
		OpenableDeviceType:       "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.openable.svg",
		CurtainDeviceType:        "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.openable.curtain.svg",
		SmartSpeakerDeviceType:   "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.svg",
		SensorDeviceType:         "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.sensor.svg",
		PetFeederDeviceType:      "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.other.svg",
		CameraDeviceType:         "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.camera.svg",
		OtherDeviceType:          "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.other.svg",

		// do not have special icon
		JetSmartMusicDeviceType:        "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.svg",
		DigmaDiHomeDeviceType:          "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.svg",
		YandexStationCentaurDeviceType: "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.svg",
		YandexStationChironDeviceType:  "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.svg",
		YandexStationPholDeviceType:    "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.svg",
		RemoteCarDeviceType:            "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.other.svg",
		// png
		YandexStationDeviceType:             "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station.png",
		YandexStation2DeviceType:            "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station_2.png",
		YandexStationMiniDeviceType:         "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station.mini.png",
		YandexStationMini2DeviceType:        "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station.mini_2_updated.png",
		YandexStationMini2NoClockDeviceType: "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station.mini_2_no_clock.png",
		YandexStationMidiDeviceType:         "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station.midi.png",
		DexpSmartBoxDeviceType:              "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.dexp.smartbox.png",
		IrbisADeviceType:                    "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.irbis.a.png",
		ElariSmartBeatDeviceType:            "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.elari.smartbeat.png",
		LGXBoomDeviceType:                   "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.lg.xboom_wk7y.png",
		PrestigioSmartMateDeviceType:        "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.prestigio.smartmate.png",
		JBLLinkPortableDeviceType:           "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.jbl.link_portable.png",
		JBLLinkMusicDeviceType:              "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.jbl.link_music.png",
		YandexModuleDeviceType:              "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.media_device.dongle.yandex.module.png",
		YandexModule2DeviceType:             "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.media_device.dongle.yandex.module_2.png",
		YandexStationMicroDeviceType:        "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station.micro.png",
		// other
	}

	KnownColorScenes = map[ColorSceneID]ColorScene{
		ColorSceneIDReading:  {ID: ColorSceneIDReading, Name: "Чтение"},
		ColorSceneIDNight:    {ID: ColorSceneIDNight, Name: "Ночь"},
		ColorSceneIDRest:     {ID: ColorSceneIDRest, Name: "Отдых"},
		ColorSceneIDMovie:    {ID: ColorSceneIDMovie, Name: "Кино"},
		ColorSceneIDDinner:   {ID: ColorSceneIDDinner, Name: "Ужин"},
		ColorSceneIDRomance:  {ID: ColorSceneIDRomance, Name: "Романтика"},
		ColorSceneIDAlarm:    {ID: ColorSceneIDAlarm, Name: "Тревога"},
		ColorSceneIDParty:    {ID: ColorSceneIDParty, Name: "Вечеринка"},
		ColorSceneIDFantasy:  {ID: ColorSceneIDFantasy, Name: "Фантазия"},
		ColorSceneIDCandle:   {ID: ColorSceneIDCandle, Name: "Свеча"},
		ColorSceneIDGarland:  {ID: ColorSceneIDGarland, Name: "Гирлянда"},
		ColorSceneIDSunset:   {ID: ColorSceneIDSunset, Name: "Закат"},
		ColorSceneIDSunrise:  {ID: ColorSceneIDSunrise, Name: "Рассвет"},
		ColorSceneIDSiren:    {ID: ColorSceneIDSiren, Name: "Сирена"},
		ColorSceneIDAlice:    {ID: ColorSceneIDAlice, Name: "Алиса"},
		ColorSceneIDOcean:    {ID: ColorSceneIDOcean, Name: "Океан"},
		ColorSceneIDNeon:     {ID: ColorSceneIDNeon, Name: "Неон"},
		ColorSceneIDJungle:   {ID: ColorSceneIDJungle, Name: "Джунгли"},
		ColorSceneIDLavaLamp: {ID: ColorSceneIDLavaLamp, Name: "Лава лампа"},
		ColorSceneIDInactive: {ID: ColorSceneIDInactive, Name: "Неактивный"},
	}

	MultiroomSpeakers = map[DeviceType]bool{
		YandexStationMiniDeviceType:         true,
		YandexStationMini2DeviceType:        true,
		YandexStationMini2NoClockDeviceType: true,
		YandexStationDeviceType:             true,
		YandexStation2DeviceType:            true,
		JBLLinkMusicDeviceType:              true,
		JBLLinkPortableDeviceType:           true,
		YandexStationMicroDeviceType:        true,
		YandexStationMidiDeviceType:         true,
	}

	StereopairAvailablePairs = map[DeviceType]DeviceTypes{
		YandexStationDeviceType:             {YandexStationDeviceType},
		YandexStation2DeviceType:            {YandexStation2DeviceType},
		YandexStationMiniDeviceType:         {YandexStationMiniDeviceType},
		YandexStationMini2DeviceType:        {YandexStationMini2DeviceType, YandexStationMini2NoClockDeviceType},
		YandexStationMini2NoClockDeviceType: {YandexStationMini2DeviceType, YandexStationMini2NoClockDeviceType},
		YandexStationMicroDeviceType:        {YandexStationMicroDeviceType},
		YandexStationCentaurDeviceType:      {YandexStationCentaurDeviceType},
		YandexStationChironDeviceType:       {YandexStationChironDeviceType},
		YandexStationPholDeviceType:         {YandexStationPholDeviceType},
		YandexStationMidiDeviceType:         {YandexStationMidiDeviceType},
	}

	CallableSpeakers = map[DeviceType]bool{
		YandexStationDeviceType:             true,
		YandexStation2DeviceType:            true,
		YandexStationMiniDeviceType:         true,
		YandexStationMini2DeviceType:        true,
		YandexStationMini2NoClockDeviceType: true,
		YandexStationMicroDeviceType:        true,
		JBLLinkPortableDeviceType:           true,
		JBLLinkMusicDeviceType:              true,
	}

	MultistepScenarioSpeakers = map[DeviceType]bool{
		YandexStationMiniDeviceType:         true,
		YandexStationMini2DeviceType:        true,
		YandexStationMini2NoClockDeviceType: true,
		YandexStationDeviceType:             true,
		YandexStation2DeviceType:            true,
		YandexStationMicroDeviceType:        true,
		YandexStationMidiDeviceType:         true,
	}

	KnownQuasarPlatforms = map[DeviceType]QuasarPlatform{
		YandexStationDeviceType:             YandexStationQuasarPlatform,
		YandexStation2DeviceType:            YandexStation2QuasarPlatform,
		YandexStationMiniDeviceType:         YandexStationMiniQuasarPlatform,
		YandexStationMini2DeviceType:        YandexStationMini2QuasarPlatform,
		YandexStationMini2NoClockDeviceType: YandexStationMini2QuasarPlatform,
		YandexStationCentaurDeviceType:      YandexStationCentaurQuasarPlatform,
		YandexStationChironDeviceType:       YandexStationChironQuasarPlatform,
		YandexStationPholDeviceType:         YandexStationPholQuasarPlatform,
		YandexModuleDeviceType:              YandexModuleQuasarPlatform,
		YandexModule2DeviceType:             YandexModule2QuasarPlatform,
		DexpSmartBoxDeviceType:              DexpSmartBoxQuasarPlatform,
		IrbisADeviceType:                    IrbisAQuasarPlatform,
		LGXBoomDeviceType:                   LGXBoomQuasarPlatform,
		ElariSmartBeatDeviceType:            ElariSmartBeatQuasarPlatform,
		JetSmartMusicDeviceType:             JetSmartMusicQuasarPlatform,
		PrestigioSmartMateDeviceType:        PrestigioSmartMateQuasarPlatform,
		DigmaDiHomeDeviceType:               DigmaDiHomeQuasarPlatform,
		JBLLinkMusicDeviceType:              JBLLinkMusicQuasarPlatform,
		JBLLinkPortableDeviceType:           JBLLinkPortableQuasarPlatform,
		YandexStationMicroDeviceType:        YandexStationMicroQuasarPlatform,
		YandexStationMidiDeviceType:         YandexStationMidiQuasarPlatform,
		SmartSpeakerDeviceType:              UnknownQuasarPlatform,
	}

	RequireXTokenSpeakers = map[DeviceType]bool{
		YandexStationDeviceType:  true,
		YandexStation2DeviceType: true,
	}

	VoiceprintSpeakers = map[DeviceType]bool{
		YandexStationMiniDeviceType:         true,
		YandexStationMini2DeviceType:        true,
		YandexStationMini2NoClockDeviceType: true,
		YandexStationCentaurDeviceType:      true,
		YandexStationChironDeviceType:       true,
		YandexStationPholDeviceType:         true,
		YandexStationDeviceType:             true,
		YandexStation2DeviceType:            true,
		YandexStationMicroDeviceType:        true,
		YandexStationMidiDeviceType:         true,
	}

	// some speakers (ex. mini1) cannot receive xcodes and tokens from directives
	// so we use sound (codename: pilyuk) to transfer some voiceprint auth info to them
	VoiceprintViaSoundSpeakers = map[DeviceType]bool{
		YandexStationMiniDeviceType: true,
	}

	KnownScenarioStepTypes = []string{string(ScenarioStepActionsType), string(ScenarioStepDelayType)}

	for k, v := range deviceTypeToProtoMap {
		protoToDeviceTypeMap[v] = k
	}

	for k, v := range capabilityTypeToProtoMap {
		protoToCapabilityTypeMap[v] = k
	}

	for k, v := range mmCapabilityTypeToProtoMap {
		mmProtoToCapabilityTypeMap[v] = k
	}

	for k, v := range propertyTypeToProtoMap {
		protoToPropertyTypeMap[v] = k
	}

	for k, v := range colorModelTypeToProtoMap {
		protoToColorModelTypeMap[v] = k
	}

	for k, v := range mmColorModelTypeToProtoMap {
		mmProtoToColorModelTypeMap[v] = k
	}

	EventPropertyInstanceToActivationEventValues = map[PropertyInstance]EventValues{
		VibrationPropertyInstance:    {TiltEvent, FallEvent, VibrationEvent},
		OpenPropertyInstance:         {OpenedEvent},
		ButtonPropertyInstance:       {ClickEvent, DoubleClickEvent, LongPressEvent},
		MotionPropertyInstance:       {DetectedEvent},
		SmokePropertyInstance:        {DetectedEvent, HighEvent},
		GasPropertyInstance:          {DetectedEvent, HighEvent},
		BatteryLevelPropertyInstance: {LowEvent},
		WaterLevelPropertyInstance:   {LowEvent},
		WaterLeakPropertyInstance:    {LeakEvent},
	}

	KnownEvents = map[EventKey]Event{
		{Instance: VibrationPropertyInstance, Value: TiltEvent}:      {Value: TiltEvent, Name: tools.AOS("наклон")},
		{Instance: VibrationPropertyInstance, Value: FallEvent}:      {Value: FallEvent, Name: tools.AOS("падение")},
		{Instance: VibrationPropertyInstance, Value: VibrationEvent}: {Value: VibrationEvent, Name: tools.AOS("вибрация")},
		{Instance: OpenPropertyInstance, Value: OpenedEvent}:         {Value: OpenedEvent, Name: tools.AOS("открыто")},
		{Instance: OpenPropertyInstance, Value: ClosedEvent}:         {Value: ClosedEvent, Name: tools.AOS("закрыто")},
		{Instance: ButtonPropertyInstance, Value: ClickEvent}:        {Value: ClickEvent, Name: tools.AOS("нажатие")},
		{Instance: ButtonPropertyInstance, Value: DoubleClickEvent}:  {Value: DoubleClickEvent, Name: tools.AOS("двойное нажатие")},
		{Instance: ButtonPropertyInstance, Value: LongPressEvent}:    {Value: LongPressEvent, Name: tools.AOS("долгое нажатие")},
		{Instance: MotionPropertyInstance, Value: DetectedEvent}:     {Value: DetectedEvent, Name: tools.AOS("движение")},
		{Instance: MotionPropertyInstance, Value: NotDetectedEvent}:  {Value: NotDetectedEvent, Name: tools.AOS("нет движения")},
		{Instance: SmokePropertyInstance, Value: DetectedEvent}:      {Value: DetectedEvent, Name: tools.AOS("обнаружен")},
		{Instance: SmokePropertyInstance, Value: NotDetectedEvent}:   {Value: NotDetectedEvent, Name: tools.AOS("не обнаружен")},
		{Instance: SmokePropertyInstance, Value: HighEvent}:          {Value: HighEvent, Name: tools.AOS("высокий уровень")},
		{Instance: GasPropertyInstance, Value: DetectedEvent}:        {Value: DetectedEvent, Name: tools.AOS("обнаружен")},
		{Instance: GasPropertyInstance, Value: NotDetectedEvent}:     {Value: NotDetectedEvent, Name: tools.AOS("не обнаружен")},
		{Instance: GasPropertyInstance, Value: HighEvent}:            {Value: HighEvent, Name: tools.AOS("высокий уровень")},
		{Instance: BatteryLevelPropertyInstance, Value: NormalEvent}: {Value: NormalEvent, Name: tools.AOS("обычный уровень")},
		{Instance: BatteryLevelPropertyInstance, Value: LowEvent}:    {Value: LowEvent, Name: tools.AOS("низкий уровень")},
		{Instance: WaterLevelPropertyInstance, Value: NormalEvent}:   {Value: NormalEvent, Name: tools.AOS("обычный уровень")},
		{Instance: WaterLevelPropertyInstance, Value: LowEvent}:      {Value: LowEvent, Name: tools.AOS("низкий уровень")},
		{Instance: WaterLeakPropertyInstance, Value: DryEvent}:       {Value: DryEvent, Name: tools.AOS("нет протечки")},
		{Instance: WaterLeakPropertyInstance, Value: LeakEvent}:      {Value: LeakEvent, Name: tools.AOS("протечка")},
	}

	KnownSpeakerNewsTopics = []string{
		string(PoliticsSpeakerNewsTopic),
		string(SocietySpeakerNewsTopic),
		string(BusinessSpeakerNewsTopic),
		string(WorldSpeakerNewsTopic),
		string(SportSpeakerNewsTopic),
		string(IncidentSpeakerNewsTopic),
		string(IndexSpeakerNewsTopic),
		string(CultureSpeakerNewsTopic),
		string(ComputersSpeakerNewsTopic),
		string(ScienceSpeakerNewsTopic),
		string(AutoSpeakerNewsTopic),
	}

	tandemDisplayCompatibleSpeakerTypes = map[DeviceType]DeviceTypes{
		YandexModuleDeviceType: {
			DexpSmartBoxDeviceType,
			IrbisADeviceType,
			ElariSmartBeatDeviceType,
			PrestigioSmartMateDeviceType,
			JBLLinkMusicDeviceType,
			JBLLinkPortableDeviceType,
			YandexStationDeviceType,
			YandexStationMiniDeviceType,
			YandexStationMini2DeviceType,
			YandexStationMini2NoClockDeviceType,
			LGXBoomDeviceType,
		},
		YandexModule2DeviceType: {
			YandexStationDeviceType,
			YandexStation2DeviceType,
			YandexStationMicroDeviceType,
			YandexStationMiniDeviceType,
			YandexStationMini2DeviceType,
			YandexStationMini2NoClockDeviceType,
			YandexStationMidiDeviceType,
		},
		TvDeviceDeviceType: {
			YandexStationDeviceType,
			YandexStation2DeviceType,
			YandexStationMicroDeviceType,
			YandexStationMiniDeviceType,
			YandexStationMini2DeviceType,
			YandexStationMini2NoClockDeviceType,
			YandexStationMidiDeviceType,
		},
	}

	tandemSpeakerCompatibleDisplayTypes = make(map[DeviceType]DeviceTypes)
	for displayType, compatibleSpeakerTypes := range tandemDisplayCompatibleSpeakerTypes {
		for _, speakerType := range compatibleSpeakerTypes {
			tandemSpeakerCompatibleDisplayTypes[speakerType] = append(tandemSpeakerCompatibleDisplayTypes[speakerType], displayType)
		}
	}

	KnownSpeakerSounds = make(map[SpeakerSoundID]SpeakerSound)
	for _, speakerSound := range knownSpeakerSoundsArray {
		KnownSpeakerSounds[speakerSound.ID] = speakerSound
	}

	deferredEventValueToDeferredEventKey = make(map[EventValue]deferredEventKey)
	for key, deferredEvents := range KnownDeferredEvents {
		for _, deferredEvent := range deferredEvents {
			deferredEventValueToDeferredEventKey[deferredEvent.Value] = key
		}
	}
}

const (
	LightDeviceType        DeviceType = "devices.types.light"
	LightCeilingDeviceType DeviceType = "devices.types.light.ceiling"
	LightLampDeviceType    DeviceType = "devices.types.light.lamp"
	LightStripDeviceType   DeviceType = "devices.types.light.strip"

	SocketDeviceType DeviceType = "devices.types.socket"
	SwitchDeviceType DeviceType = "devices.types.switch"
	HubDeviceType    DeviceType = "devices.types.hub"

	PurifierDeviceType      DeviceType = "devices.types.purifier"
	HumidifierDeviceType    DeviceType = "devices.types.humidifier"
	VacuumCleanerDeviceType DeviceType = "devices.types.vacuum_cleaner"

	CookingDeviceType     DeviceType = "devices.types.cooking"
	KettleDeviceType      DeviceType = "devices.types.cooking.kettle"
	CoffeeMakerDeviceType DeviceType = "devices.types.cooking.coffee_maker"
	MulticookerDeviceType DeviceType = "devices.types.cooking.multicooker"

	ThermostatDeviceType DeviceType = "devices.types.thermostat"
	AcDeviceType         DeviceType = "devices.types.thermostat.ac"

	MediaDeviceDeviceType DeviceType = "devices.types.media_device"
	TvDeviceDeviceType    DeviceType = "devices.types.media_device.tv"
	ReceiverDeviceType    DeviceType = "devices.types.media_device.receiver"
	TvBoxDeviceType       DeviceType = "devices.types.media_device.tv_box"

	WashingMachineDeviceType DeviceType = "devices.types.washing_machine"
	DishwasherDeviceType     DeviceType = "devices.types.dishwasher"
	RefrigeratorDeviceType   DeviceType = "devices.types.refrigerator"
	FanDeviceType            DeviceType = "devices.types.fan"
	IronDeviceType           DeviceType = "devices.types.iron"
	PetFeederDeviceType      DeviceType = "devices.types.pet_feeder"

	OpenableDeviceType DeviceType = "devices.types.openable"
	CurtainDeviceType  DeviceType = "devices.types.openable.curtain"

	SmartSpeakerDeviceType              DeviceType = "devices.types.smart_speaker"
	YandexStationDeviceType             DeviceType = "devices.types.smart_speaker.yandex.station"
	YandexStation2DeviceType            DeviceType = "devices.types.smart_speaker.yandex.station_2"
	YandexStationMiniDeviceType         DeviceType = "devices.types.smart_speaker.yandex.station.mini"
	YandexStationMini2DeviceType        DeviceType = "devices.types.smart_speaker.yandex.station.mini_2"
	YandexStationMini2NoClockDeviceType DeviceType = "devices.types.smart_speaker.yandex.station.mini_2_no_clock"
	YandexStationMicroDeviceType        DeviceType = "devices.types.smart_speaker.yandex.station.micro"
	YandexStationCentaurDeviceType      DeviceType = "devices.types.smart_speaker.yandex.station.centaur"
	YandexStationChironDeviceType       DeviceType = "devices.types.smart_speaker.yandex.station.chiron"
	YandexStationPholDeviceType         DeviceType = "devices.types.smart_speaker.yandex.station.phol"
	YandexStationMidiDeviceType         DeviceType = "devices.types.smart_speaker.yandex.station.midi"
	DexpSmartBoxDeviceType              DeviceType = "devices.types.smart_speaker.dexp.smartbox"
	IrbisADeviceType                    DeviceType = "devices.types.smart_speaker.irbis.a"
	ElariSmartBeatDeviceType            DeviceType = "devices.types.smart_speaker.elari.smartbeat"
	LGXBoomDeviceType                   DeviceType = "devices.types.smart_speaker.lg.xboom_wk7y"
	JetSmartMusicDeviceType             DeviceType = "devices.types.smart_speaker.jet.smartmusic"
	PrestigioSmartMateDeviceType        DeviceType = "devices.types.smart_speaker.prestigio.smartmate"
	DigmaDiHomeDeviceType               DeviceType = "devices.types.smart_speaker.digma.dihome"
	JBLLinkPortableDeviceType           DeviceType = "devices.types.smart_speaker.jbl.link_portable"
	JBLLinkMusicDeviceType              DeviceType = "devices.types.smart_speaker.jbl.link_music"

	YandexModuleDeviceType  DeviceType = "devices.types.media_device.dongle.yandex.module"
	YandexModule2DeviceType DeviceType = "devices.types.media_device.dongle.yandex.module_2"

	RemoteCarDeviceType DeviceType = "devices.types.remote_car"

	SensorDeviceType DeviceType = "devices.types.sensor"

	CameraDeviceType DeviceType = "devices.types.camera"

	OtherDeviceType DeviceType = "devices.types.other"
)

const (
	YandexStationQuasarPlatform        QuasarPlatform = "yandexstation"
	YandexStation2QuasarPlatform       QuasarPlatform = "yandexstation_2"
	YandexStationCentaurQuasarPlatform QuasarPlatform = "centaur"
	YandexStationChironQuasarPlatform  QuasarPlatform = "chiron"
	YandexStationPholQuasarPlatform    QuasarPlatform = "phol"
	YandexModuleQuasarPlatform         QuasarPlatform = "yandexmodule"
	DexpSmartBoxQuasarPlatform         QuasarPlatform = "lightcomm"
	IrbisAQuasarPlatform               QuasarPlatform = "linkplay_a98"
	LGXBoomQuasarPlatform              QuasarPlatform = "wk7y"
	ElariSmartBeatQuasarPlatform       QuasarPlatform = "elari_a98"
	YandexStationMiniQuasarPlatform    QuasarPlatform = "yandexmini"
	JetSmartMusicQuasarPlatform        QuasarPlatform = "jet_smart_music"
	PrestigioSmartMateQuasarPlatform   QuasarPlatform = "prestigio_smart_mate"
	DigmaDiHomeQuasarPlatform          QuasarPlatform = "digma_di_home"
	JBLLinkMusicQuasarPlatform         QuasarPlatform = "jbl_link_music"
	JBLLinkPortableQuasarPlatform      QuasarPlatform = "jbl_link_portable"
	YandexStationMini2QuasarPlatform   QuasarPlatform = "yandexmini_2"
	YandexStationMicroQuasarPlatform   QuasarPlatform = "yandexmicro"
	YandexStationMidiQuasarPlatform    QuasarPlatform = "yandexmidi"
	YandexModule2QuasarPlatform        QuasarPlatform = "yandexmodule_2"
	UnknownQuasarPlatform              QuasarPlatform = "unknown"
)

const (
	OnOffCapabilityType              CapabilityType = "devices.capabilities.on_off"
	ColorSettingCapabilityType       CapabilityType = "devices.capabilities.color_setting"
	ModeCapabilityType               CapabilityType = "devices.capabilities.mode"
	RangeCapabilityType              CapabilityType = "devices.capabilities.range"
	ToggleCapabilityType             CapabilityType = "devices.capabilities.toggle"
	CustomButtonCapabilityType       CapabilityType = "devices.capabilities.custom.button"
	QuasarServerActionCapabilityType CapabilityType = "devices.capabilities.quasar.server_action"
	QuasarCapabilityType             CapabilityType = "devices.capabilities.quasar"
	VideoStreamCapabilityType        CapabilityType = "devices.capabilities.video_stream"
)

var (
	FloatPropertyType PropertyType = "devices.properties.float"
	EventPropertyType PropertyType = "devices.properties.event"
)

const (
	OnOnOffCapabilityInstance OnOffCapabilityInstance = "on"
)

const (
	RgbColorCapabilityInstance     ColorSettingCapabilityInstance = "rgb"
	HsvColorCapabilityInstance     ColorSettingCapabilityInstance = "hsv"
	TemperatureKCapabilityInstance ColorSettingCapabilityInstance = "temperature_k"
	SceneCapabilityInstance        ColorSettingCapabilityInstance = "scene"
)

// artificial color capability instances
const (
	HypothesisColorCapabilityInstance      string = "color"
	HypothesisColorSceneCapabilityInstance string = "color_scene"
)

const (
	MuteToggleCapabilityInstance           ToggleCapabilityInstance = "mute"
	BacklightToggleCapabilityInstance      ToggleCapabilityInstance = "backlight"
	ControlsLockedToggleCapabilityInstance ToggleCapabilityInstance = "controls_locked"
	IonizationToggleCapabilityInstance     ToggleCapabilityInstance = "ionization"
	OscillationToggleCapabilityInstance    ToggleCapabilityInstance = "oscillation"
	KeepWarmToggleCapabilityInstance       ToggleCapabilityInstance = "keep_warm"
	PauseToggleCapabilityInstance          ToggleCapabilityInstance = "pause"

	// RemoteCar Toggles
	TrunkToggleCapabilityInstance ToggleCapabilityInstance = "trunk"
	CentralLockCapabilityInstance ToggleCapabilityInstance = "central_lock"
)

const (
	BrightnessRangeInstance  RangeCapabilityInstance = "brightness"
	TemperatureRangeInstance RangeCapabilityInstance = "temperature"
	VolumeRangeInstance      RangeCapabilityInstance = "volume"
	ChannelRangeInstance     RangeCapabilityInstance = "channel"
	HumidityRangeInstance    RangeCapabilityInstance = "humidity"
	OpenRangeInstance        RangeCapabilityInstance = "open"
)

const (
	ThermostatModeInstance  ModeCapabilityInstance = "thermostat"
	FanSpeedModeInstance    ModeCapabilityInstance = "fan_speed"
	WorkSpeedModeInstance   ModeCapabilityInstance = "work_speed"
	CleanUpModeInstance     ModeCapabilityInstance = "cleanup_mode"
	ProgramModeInstance     ModeCapabilityInstance = "program"
	InputSourceModeInstance ModeCapabilityInstance = "input_source"
	CoffeeModeInstance      ModeCapabilityInstance = "coffee_mode"
	SwingModeInstance       ModeCapabilityInstance = "swing"
	HeatModeInstance        ModeCapabilityInstance = "heat"
	DishwashingModeInstance ModeCapabilityInstance = "dishwashing"
	TeaModeInstance         ModeCapabilityInstance = "tea_mode"
)

const (
	PhraseActionCapabilityInstance QuasarServerActionCapabilityInstance = "phrase_action"
	TextActionCapabilityInstance   QuasarServerActionCapabilityInstance = "text_action"
)

const (
	WeatherCapabilityInstance        QuasarCapabilityInstance = "weather"
	VolumeCapabilityInstance         QuasarCapabilityInstance = "volume"
	MusicPlayCapabilityInstance      QuasarCapabilityInstance = "music_play"
	NewsCapabilityInstance           QuasarCapabilityInstance = "news"
	SoundPlayCapabilityInstance      QuasarCapabilityInstance = "sound_play"
	StopEverythingCapabilityInstance QuasarCapabilityInstance = "stop_everything"
	TTSCapabilityInstance            QuasarCapabilityInstance = "tts"
	AliceShowCapabilityInstance      QuasarCapabilityInstance = "alice_show"
)

const (
	GetStreamCapabilityInstance VideoStreamCapabilityInstance = "get_stream"
)

const (
	//TemperatureK string         = "temperature_k"
	HsvModelType ColorModelType = "hsv"
	RgbModelType ColorModelType = "rgb"
)

// float properties
const (
	HumidityPropertyInstance           PropertyInstance = "humidity"
	TemperaturePropertyInstance        PropertyInstance = "temperature"
	CO2LevelPropertyInstance           PropertyInstance = "co2_level"
	WaterLevelPropertyInstance         PropertyInstance = "water_level"
	AmperagePropertyInstance           PropertyInstance = "amperage"
	VoltagePropertyInstance            PropertyInstance = "voltage"
	PowerPropertyInstance              PropertyInstance = "power"
	PM1DensityPropertyInstance         PropertyInstance = "pm1_density"
	PM2p5DensityPropertyInstance       PropertyInstance = "pm2.5_density"
	PM10DensityPropertyInstance        PropertyInstance = "pm10_density"
	TvocPropertyInstance               PropertyInstance = "tvoc"
	PressurePropertyInstance           PropertyInstance = "pressure"
	BatteryLevelPropertyInstance       PropertyInstance = "battery_level"
	TimerPropertyInstance              PropertyInstance = "timer"
	IlluminationPropertyInstance       PropertyInstance = "illumination"
	GasConcentrationPropertyInstance   PropertyInstance = "gas_concentration"
	SmokeConcentrationPropertyInstance PropertyInstance = "smoke_concentration"
)

// event properties
const (
	OpenPropertyInstance      PropertyInstance = "open"
	MotionPropertyInstance    PropertyInstance = "motion"
	VibrationPropertyInstance PropertyInstance = "vibration"
	ButtonPropertyInstance    PropertyInstance = "button"
	SmokePropertyInstance     PropertyInstance = "smoke"
	GasPropertyInstance       PropertyInstance = "gas"
	WaterLeakPropertyInstance PropertyInstance = "water_leak"
)

const (
	NormalStatus  PropertyStatus = "normal"
	WarningStatus PropertyStatus = "warning"
	DangerStatus  PropertyStatus = "danger"
)

const (
	OnlineDeviceStatus   DeviceStatus = "online"
	OfflineDeviceStatus  DeviceStatus = "offline"
	NotFoundDeviceStatus DeviceStatus = "not_found"
	UnknownDeviceStatus  DeviceStatus = "unknown"
	SplitStatus          DeviceStatus = "split"
	// TODO split GroupState
)

const (
	// when new unit is added you also need to add it to KnownUnits map
	UnitPercent            Unit = "unit.percent"
	UnitTemperatureCelsius Unit = "unit.temperature.celsius"
	UnitTemperatureKelvin  Unit = "unit.temperature.kelvin"
	UnitPPM                Unit = "unit.ppm"
	UnitAmpere             Unit = "unit.ampere"
	UnitVolt               Unit = "unit.volt"
	UnitWatt               Unit = "unit.watt"
	UnitDensityMcgM3       Unit = "unit.density.mcg_m3"
	UnitPressureAtm        Unit = "unit.pressure.atm"
	UnitPressurePascal     Unit = "unit.pressure.pascal"
	UnitPressureBar        Unit = "unit.pressure.bar"
	UnitPressureMmHg       Unit = "unit.pressure.mmhg"
	UnitTimeSeconds        Unit = "unit.time.seconds"
	UnitIlluminationLux    Unit = "unit.illumination.lux"
)

func GetMMHGFromPascal(pascal float64) float64 {
	return pascal / 133
}

const (
	// Old Thermostat Modes
	HeatMode    ModeValue = "heat"
	CoolMode    ModeValue = "cool"
	AutoMode    ModeValue = "auto"
	EcoMode     ModeValue = "eco"
	DryMode     ModeValue = "dry"
	FanOnlyMode ModeValue = "fan_only"

	// Old Fan Speed Modes without auto
	LowMode    ModeValue = "low"
	MediumMode ModeValue = "medium"
	HighMode   ModeValue = "high"

	//Unlinked Modes
	TurboMode          ModeValue = "turbo"
	FastMode           ModeValue = "fast"
	SlowMode           ModeValue = "slow"
	ExpressMode        ModeValue = "express"
	QuietMode          ModeValue = "quiet"
	NormalMode         ModeValue = "normal"
	PreHeatMode        ModeValue = "preheat"
	MaxMode            ModeValue = "max"
	MinMode            ModeValue = "min"
	HorizontalMode     ModeValue = "horizontal"
	VerticalMode       ModeValue = "vertical"
	StationaryMode     ModeValue = "stationary"
	LatteMode          ModeValue = "latte"
	CappuccinoMode     ModeValue = "cappuccino"
	EspressoMode       ModeValue = "espresso"
	DoubleEspressoMode ModeValue = "double_espresso"
	AmericanoMode      ModeValue = "americano"
	WindFreeMode       ModeValue = "windfree" // special samsung mode

	OneMode   ModeValue = "one"
	TwoMode   ModeValue = "two"
	ThreeMode ModeValue = "three"
	FourMode  ModeValue = "four"
	FiveMode  ModeValue = "five"
	SixMode   ModeValue = "six"
	SevenMode ModeValue = "seven"
	EightMode ModeValue = "eight"
	NineMode  ModeValue = "nine"
	TenMode   ModeValue = "ten"

	// washing machine
	WoolMode ModeValue = "wool"

	// dishwashing
	Auto45Mode    ModeValue = "auto_45"
	Auto60Mode    ModeValue = "auto_60"
	Auto75Mode    ModeValue = "auto_75"
	Fast45Mode    ModeValue = "fast_45"
	Fast60Mode    ModeValue = "fast_60"
	Fast75Mode    ModeValue = "fast_75"
	PreRinseMode  ModeValue = "pre_rinse"
	IntensiveMode ModeValue = "intensive"
	GlassMode     ModeValue = "glass"

	// multicooker
	VacuumMode       ModeValue = "vacuum"
	BoilingMode      ModeValue = "boiling"
	BakingMode       ModeValue = "baking"
	DessertMode      ModeValue = "dessert"
	BabyFoodMode     ModeValue = "baby_food"
	FowlMode         ModeValue = "fowl"
	FryingMode       ModeValue = "frying"
	YogurtMode       ModeValue = "yogurt"
	CerealsMode      ModeValue = "cereals"
	MacaroniMode     ModeValue = "macaroni"
	MilkPorridgeMode ModeValue = "milk_porridge"
	MulticookerMode  ModeValue = "multicooker"
	SteamMode        ModeValue = "steam"
	PastaMode        ModeValue = "pasta"
	PizzaMode        ModeValue = "pizza"
	PilafMode        ModeValue = "pilaf"
	SauceMode        ModeValue = "sauce"
	SoupMode         ModeValue = "soup"
	StewingMode      ModeValue = "stewing"
	SlowCookMode     ModeValue = "slow_cook"
	DeepFryerMode    ModeValue = "deep_fryer"
	BreadMode        ModeValue = "bread"
	AspicMode        ModeValue = "aspic"
	CheesecakeMode   ModeValue = "cheesecake"

	// tea mode
	BlackTeaMode  ModeValue = "black_tea"
	GreenTeaMode  ModeValue = "green_tea"
	PuerhTeaMode  ModeValue = "puerh_tea"
	WhiteTeaMode  ModeValue = "white_tea"
	OolongTeaMode ModeValue = "oolong_tea"
	RedTeaMode    ModeValue = "red_tea"
	HerbalTeaMode ModeValue = "herbal_tea"
	FlowerTeaMode ModeValue = "flower_tea"
)

const (
	// vibration sensor
	TiltEvent      EventValue = "tilt"
	FallEvent      EventValue = "fall"
	VibrationEvent EventValue = "vibration"

	// open
	OpenedEvent EventValue = "opened"
	ClosedEvent EventValue = "closed"

	// button
	ClickEvent       EventValue = "click"
	DoubleClickEvent EventValue = "double_click"
	LongPressEvent   EventValue = "long_press"

	// motion/gas/smoke
	DetectedEvent    EventValue = "detected"
	NotDetectedEvent EventValue = "not_detected"

	// gas/smoke
	HighEvent EventValue = "high"

	// battery/water_level
	LowEvent    EventValue = "low"
	NormalEvent EventValue = "normal"

	// water_leak
	DryEvent  EventValue = "dry"
	LeakEvent EventValue = "leak"
)

const (
	TUYA      string = "T"
	QUASAR    string = "Q"
	REMOTECAR string = "RC" // todo: remove all REMOTECAR usages except capabilities
	YANDEXIO  string = "YANDEX_IO"
	VIRTUAL   string = "VIRTUAL"
	QUALITY   string = "QUALITY"
	UIQUALITY string = "eacb68b3-27dc-4d8d-bdbb-b4f6fb7babd2"

	XiaomiSkill     string = "ad26f8c2-fc31-4928-a653-d829fda7e6c2"
	AqaraSkill      string = "c927bb15-5ecb-472a-8895-c3740602d36a"
	PhilipsSkill    string = "4a8cbce2-61d3-4e58-9f7b-6b30371d265c"
	NewPhilipsSkill string = "9d1ee15f-5392-4252-8eca-18d0845f3b03"
	RedmondSkill    string = "d1ba0dfe-e7b5-4a1c-af18-2e35ea0adb3c"
	SamsungSkill    string = "3601a821-be82-4185-9dc1-bf47476a0c99"
	LGSkill         string = "457299b6-174e-4373-9064-9c7b8cab27d1"
	LegrandSkill    string = "4e5c1f77-4832-4f25-923f-935b1d9f32d6"
	RubetekSkill    string = "c43a5814-508b-44aa-afde-10442c10e7ba"
	ElariSkill      string = "43606352-ee3f-4ec8-a131-2c754551b4d2"
	DigmaSkill      string = "909af191-576d-4fda-95b8-cd0cf2d6dbbb"
	PerenioSkill    string = "997adf4b-22f9-4fa0-8305-850c18a787dc"
	SberSkill       string = "d953c7e4-d0a8-409a-a8dc-810b9c5e537c"
)

const (
	HumanReadableTuyaProviderName      string = "Яндекс"
	HumanReadableQuasarProviderName    string = "Яндекс"
	HumanReadableYandexIOProviderName  string = "Яндекс"
	HumanReadableRemoteCarProviderName string = "Яндекс"
	HumanReadableSberProviderName      string = "Сбер"
)

const (
	VoiceScenarioTriggerType     ScenarioTriggerType = "scenario.trigger.voice"
	TimerScenarioTriggerType     ScenarioTriggerType = "scenario.trigger.timer"
	TimetableScenarioTriggerType ScenarioTriggerType = "scenario.trigger.timetable"
	PropertyScenarioTriggerType  ScenarioTriggerType = "scenario.trigger.property"
	AppScenarioTriggerType       ScenarioTriggerType = "scenario.trigger.app"
	APIScenarioTriggerType       ScenarioTriggerType = "scenario.trigger.api"
)

const SlowConnection string = "Похоже, что ваше устройство привязано к региону \"Китай\", время реакции может составлять до нескольких секунд"

const (
	ScenarioIconMorning     ScenarioIcon = "morning"
	ScenarioIconDay         ScenarioIcon = "day"
	ScenarioIconEvening     ScenarioIcon = "evening"
	ScenarioIconNight       ScenarioIcon = "night"
	ScenarioIconGame        ScenarioIcon = "game"
	ScenarioIconSport       ScenarioIcon = "sport"
	ScenarioIconCleaning    ScenarioIcon = "cleaning"
	ScenarioIconHome        ScenarioIcon = "home"
	ScenarioIconSofa        ScenarioIcon = "sofa"
	ScenarioIconWork        ScenarioIcon = "work"
	ScenarioIconFlowers     ScenarioIcon = "flowers"
	ScenarioIconDrink       ScenarioIcon = "drink"
	ScenarioIconAlarm       ScenarioIcon = "alarm"
	ScenarioIconParty       ScenarioIcon = "party"
	ScenarioIconRomantic    ScenarioIcon = "romantic"
	ScenarioIconCooking     ScenarioIcon = "cooking"
	ScenarioIconPresent     ScenarioIcon = "present"
	ScenarioIconStar        ScenarioIcon = "star"
	ScenarioIconTree        ScenarioIcon = "tree"
	ScenarioIconSnowflake   ScenarioIcon = "snowflake"
	ScenarioIconToy         ScenarioIcon = "toy"
	ScenarioIconBall        ScenarioIcon = "ball"
	ScenarioIconSock        ScenarioIcon = "sock"
	ScenarioIconLamp        ScenarioIcon = "lamp"
	ScenarioIconTV          ScenarioIcon = "tv"
	ScenarioIconStation     ScenarioIcon = "station"
	ScenarioIconToggle      ScenarioIcon = "toggle"
	ScenarioIconSocket      ScenarioIcon = "socket"
	ScenarioIconMusic       ScenarioIcon = "music"
	ScenarioIconLamps       ScenarioIcon = "lamps"
	ScenarioIconHumidity    ScenarioIcon = "humidity"
	ScenarioIconSecurity    ScenarioIcon = "security"
	ScenarioIconDoor        ScenarioIcon = "door"
	ScenarioIconDaynight    ScenarioIcon = "daynight"
	ScenarioIconWaterleak   ScenarioIcon = "waterleak"
	ScenarioIconCo2         ScenarioIcon = "co2"
	ScenarioIconPuppy       ScenarioIcon = "puppy"
	ScenarioIconCastle      ScenarioIcon = "castle"
	ScenarioIconWindow      ScenarioIcon = "window"
	ScenarioIconAc          ScenarioIcon = "ac"
	ScenarioIconMotion      ScenarioIcon = "motion"
	ScenarioIconGas         ScenarioIcon = "gas"
	ScenarioIconTemperature ScenarioIcon = "temperature"
	ScenarioIconHeater      ScenarioIcon = "heater"
)

const (
	RenameToRussianErrorMessage                           string = "Пишите кириллицей, без пунктуации и спецсимволов. Между словами и числами ставьте пробелы"
	RenameToRussianAndLatinErrorMessage                   string = "Пишите кириллицей или латиницей, без пунктуации и спецсимволов. Между словами и числами ставьте пробелы"
	NameIsAlreadyTakenErrorMessage                        string = "Данное имя уже используется"
	NameLengthErrorMessage                                string = "Название должно быть не длиннее %d символов"
	NameMinLettersErrorMessage                            string = "Название должно содержать не менее двух букв"
	NameEmptyErrorMessage                                 string = "Введите название"
	DeviceHasNoRoomErrorMessage                           string = "Выберите комнату"
	DeviceInGroupTypeSwitchError                          string = "Удалите устройство из всех групп, чтобы сменить его тип"
	DeviceTypeUnacceptableSwitchMessage                   string = "Подобное изменение типа недопустимо для этого устройства"
	DeviceUnreachableErrorMessage                         string = "Устройство не отвечает. Проверьте, вдруг оно выключено или пропал интернет."
	DeviceBusyErrorMessage                                string = "Устройство уже работает. Подождите, пока оно закончит."
	DeviceInvalidActionErrorMessage                       string = "Это устройство так не умеет. Попробуйте что-нибудь другое."
	DeviceInvalidValueErrorMessage                        string = "Кажется, вы просите невозможного. Попробуйте что-нибудь другое."
	DeviceNotSupportedInCurrentModeMessage                string = "В этом режиме такая команда не работает. Измените режим и повторите команду."
	DiscoveryErrorMessage                                 string = "Не получилось обновить список устройств. Подождите немного и попробуйте ещё раз."
	DiscoveryAuthorizationErrorMessage                    string = "Не удается получить доступ к аккаунту в приложении производителя. Попробуйте заново связать аккаунты."
	ScenarioActionValueErrorMessage                       string = "Устройство не сможет обработать заданные действия. Измените их и попробуйте еще раз."
	VoiceTriggerPhraseAlreadyTakenErrorMessage            string = "Активационная фраза для сценария уже используется"
	TriggersLimitReachedErrorMessage                      string = "Вы уже добавили для этого сценария четыре фразы. Больше не получится."
	TimetableTriggersLimitReachedErrorMessage             string = "Вы уже настроили расписание для этого сценария"
	LocalScenarioSyncFailedErrorMessage                   string = "Невозможно сохранить изменения: проверьте, что колонка в сети, и попробуйте ещё раз."
	ScenarioTextServerActionNameErrorMessage              string = "Фраза для Алисы не может быть фразой запуска сценария"
	NoAddedDevicesErrorMessage                            string = "Сначала добавьте устройства."
	DoorOpenErrorMessage                                  string = "Не забудьте закрыть дверцу"
	LidOpenErrorMessage                                   string = "Не забудьте закрыть крышку"
	RemoteControlDisabledErrorMessage                     string = "Сначала нужно спросить разрешения у самого устройства: проверьте, на нём должна быть специальная кнопка"
	NotEnoughWaterErrorMessage                            string = "Попробуйте долить воды"
	LowChargeLevelErrorMessage                            string = "Устройство нужно зарядить"
	ContainerFullErrorMessage                             string = "Сначала нужно очистить контейнер"
	ContainerEmptyErrorMessage                            string = "В контейнер нужно что-нибудь положить"
	DripTrayFullErrorMessage                              string = "Нужно очистить поддон"
	DeviceStuckErrorMessage                               string = "Кажется, на пути препятствие, его нужно убрать"
	DeviceOffErrorMessage                                 string = "Сначала нужно включить устройство"
	FirmwareOutOfDateErrorMessage                         string = "Нужно обновить прошивку устройства, которым хотите управлять"
	NotEnoughDetergentErrorMessage                        string = "Добавьте моющее средство"
	AccountLinkingErrorErrorMessage                       string = "Попробуйте привязать устройство заново, а то оно отвязалось"
	HumanInvolvementNeededErrorMessage                    string = "Что-то не так с устройством: пожалуйста, осмотрите его"
	DeviceNameAliasesLimitReachedErrorMessage             string = "Слишком много имен для устройства"
	GroupNameAliasesLimitReachedErrorMessage              string = "Слишком много имен для группы"
	DeviceNameAliasesAlreadyExistsErrorMessage            string = "Такое имя у устройства уже есть. Придумайте другое."
	DeviceTypeAliasesUnsupportedErrorMessage              string = "Не могу добавить дополнительное имя к этому устройству"
	QuasarServerActionValueEmptyErrorMessage              string = "Введите команду Алисе"
	QuasarServerActionValueLengthErrorMessage             string = "Команда Алисе должна быть не длиннее %d символов"
	QuasarServerActionValueCharErrorMessage               string = `Команда Алисе может содержать только кириллицу, латиницу, цифры и спецсимволы: "-,!.:=?".`
	QuasarServerActionValueMinLettersErrorMessage         string = "Команда Алисе должна содержать не менее двух букв"
	HouseholdContainsDevicesDeletionErrorMessage          string = "Нельзя удалить дом, в котором есть устройства."
	LastHouseholdDeletionErrorMessage                     string = "Нельзя удалить последний имеющийся дом."
	ActiveHouseholdDeletionErrorMessage                   string = "Нельзя удалить текущий активный дом."
	HouseholdEmptyAddressErrorMessage                     string = "Введите адрес дома"
	HouseholdEmptyLocationErrorMessage                    string = "Введите координаты дома"
	HouseholdInvalidAddressErrorMessage                   string = "Введите существующий адрес"
	ScenarioStepsRepeatedDeviceErrorMessage               string = "Нельзя добавить одно и то же устройство в один шаг сценария несколько раз"
	ScenarioStepsAtLeastOneActionErrorMessage             string = "В сценарии должно быть хотя бы одно действие с устройствами"
	ScenarioStepsDelayLimitReachedErrorMessage            string = "Общая продолжительность задержек в сценарии не должна превышать сутки"
	ScenarioStepsConsecutiveDelaysErrorMessage            string = "В сценарий нельзя добавить несколько задержек подряд"
	ScenarioStepsDelayLastStepErrorMessage                string = "Задержка не может быть последним действием в сценарии"
	DeviceInternalErrorMessage                            string = "Случилось что-то непонятное. Подождите немного и попробуйте ещё раз."
	DeviceUnlinkedErrorMessage                            string = "Устройство отвязано от аккаунта"
	InvalidPropertyConditionErrorMessage                  string = "Кажется, вы ввели некорректное значение"
	UnknownErrorMessage                                   string = "Что-то пошло не так. Попробуйте позднее ещё раз."
	DisassembleStereopairBeforeDeviceDeletionErrorMessage string = "Перед удалением устройства нужно разобрать стереопару"
	DeviceNotFoundErrorMessage                            string = "Данное устройство вам не принадлежит."
	RoomNotFoundErrorMessage                              string = "Данная комната вам не принадлежит."
	GroupNotFoundErrorMessage                             string = "Данная группа вам не принадлежит."
	HouseholdNotFoundErrorMessage                         string = "Данный дом вам не принадлежит."
	SharedDeviceUsedInScenarioErrorMessage                string = "Нельзя добавлять в сценарии чужие устройства. Пожалуйста, удалите их из сценария и попробуйте еще раз."
	SharedHouseholdOwnerLeavingErrorMessage               string = "Вы не можете покинуть дом, который вам принадлежит."
	SharingLinkNeedlessAcceptanceErrorMessage             string = "Вы уже имеете доступ к этому дому."
	SharingLinkDoesNotExistErrorMessage                   string = "Истек срок действия ссылки."
	SharingInvitationDoesNotExistErrorMessage             string = "Истек срок действия приглашения."
	SharingInvitationDoesNotOwnedByUserErrorMessage       string = "Вы не являетесь адресатом этого приглашения."
	SharingUsersLimitReachedErrorMessage                  string = "Вы можете иметь доступ только в дома четырех пользователей единовременно."
	SharingDevicesLimitReachedErrorMessage                string = "Вы не можете управлять более чем 301 устройствами одновременно, включая устройства из чужих домов."
)

type ColorID string

func (id ColorID) String() string {
	return string(id)
}

const (
	ColorIDFieryWhite    ColorID = "fiery_white"
	ColorIDSoftWhite     ColorID = "soft_white"
	ColorIDWarmWhite     ColorID = "warm_white"
	ColorIDDaylight      ColorID = "daylight"
	ColorIDWhite         ColorID = "white"
	ColorIDColdWhite     ColorID = "cold_white"
	ColorIDMistyWhite    ColorID = "misty_white"
	ColorIDHeavenlyWhite ColorID = "heavenly_white"

	ColorIDRed       ColorID = "red"
	ColorIDCoral     ColorID = "coral"
	ColorIDOrange    ColorID = "orange"
	ColorIDYellow    ColorID = "yellow"
	ColorIDLime      ColorID = "lime"
	ColorIDGreen     ColorID = "green"
	ColorIDEmerald   ColorID = "emerald"
	ColorIDTurquoise ColorID = "turquoise"
	ColorIDCyan      ColorID = "cyan"
	ColorIDBlue      ColorID = "blue"
	ColorIDMoonlight ColorID = "moonlight"
	ColorIDLavender  ColorID = "lavender"
	ColorIDViolet    ColorID = "violet"
	ColorIDPurple    ColorID = "purple"
	ColorIDOrchid    ColorID = "orchid"
	ColorIDMauve     ColorID = "mauve"
	ColorIDRaspberry ColorID = "raspberry"
)

const (
	ColorSceneIDReading  ColorSceneID = "reading"
	ColorSceneIDNight    ColorSceneID = "night"
	ColorSceneIDRest     ColorSceneID = "rest"
	ColorSceneIDMovie    ColorSceneID = "movie"
	ColorSceneIDDinner   ColorSceneID = "dinner"
	ColorSceneIDRomance  ColorSceneID = "romance"
	ColorSceneIDAlarm    ColorSceneID = "alarm"
	ColorSceneIDParty    ColorSceneID = "party"
	ColorSceneIDFantasy  ColorSceneID = "fantasy"
	ColorSceneIDCandle   ColorSceneID = "candle"
	ColorSceneIDGarland  ColorSceneID = "garland"
	ColorSceneIDSunset   ColorSceneID = "sunset"
	ColorSceneIDSunrise  ColorSceneID = "sunrise"
	ColorSceneIDSiren    ColorSceneID = "siren"
	ColorSceneIDOcean    ColorSceneID = "ocean"
	ColorSceneIDNeon     ColorSceneID = "neon"
	ColorSceneIDAlice    ColorSceneID = "alice"
	ColorSceneIDJungle   ColorSceneID = "jungle"
	ColorSceneIDLavaLamp ColorSceneID = "lava_lamp"
	ColorSceneIDInactive ColorSceneID = "inactive"
)

const (
	Increase RelativityType = "increase"
	Decrease RelativityType = "decrease"
	Invert   RelativityType = "invert"
)

const (
	Min string = "min"
	Max string = "max"
)

// old map uses our protos
var capabilityTypeToProtoMap = map[CapabilityType]protos.CapabilityType{
	OnOffCapabilityType:              protos.CapabilityType_OnOffCapabilityType,
	ColorSettingCapabilityType:       protos.CapabilityType_ColorSettingCapabilityType,
	ModeCapabilityType:               protos.CapabilityType_ModeCapabilityType,
	RangeCapabilityType:              protos.CapabilityType_RangeCapabilityType,
	ToggleCapabilityType:             protos.CapabilityType_ToggleCapabilityType,
	CustomButtonCapabilityType:       protos.CapabilityType_CustomButtonCapabilityType,
	QuasarServerActionCapabilityType: protos.CapabilityType_QuasarServerActionCapabilityType,
	QuasarCapabilityType:             protos.CapabilityType_QuasarCapabilityType,
	VideoStreamCapabilityType:        protos.CapabilityType_VideoStreamCapabilityType,
}

// new chad map uses protos from megamind/protos/common
var mmCapabilityTypeToProtoMap = map[CapabilityType]common.TIoTUserInfo_TCapability_ECapabilityType{
	OnOffCapabilityType:              common.TIoTUserInfo_TCapability_OnOffCapabilityType,
	ColorSettingCapabilityType:       common.TIoTUserInfo_TCapability_ColorSettingCapabilityType,
	ModeCapabilityType:               common.TIoTUserInfo_TCapability_ModeCapabilityType,
	RangeCapabilityType:              common.TIoTUserInfo_TCapability_RangeCapabilityType,
	ToggleCapabilityType:             common.TIoTUserInfo_TCapability_ToggleCapabilityType,
	CustomButtonCapabilityType:       common.TIoTUserInfo_TCapability_CustomButtonCapabilityType,
	QuasarServerActionCapabilityType: common.TIoTUserInfo_TCapability_QuasarServerActionCapabilityType,
	QuasarCapabilityType:             common.TIoTUserInfo_TCapability_QuasarCapabilityType,
	VideoStreamCapabilityType:        common.TIoTUserInfo_TCapability_VideoStreamCapabilityType,
}

var mmProtoToCapabilityTypeMap = map[common.TIoTUserInfo_TCapability_ECapabilityType]CapabilityType{}

var propertyTypeToProtoMap = map[PropertyType]protos.PropertyType{
	FloatPropertyType: protos.PropertyType_FloatPropertyType,
	EventPropertyType: protos.PropertyType_EventPropertyType,
}

// old map uses our protos
var colorModelTypeToProtoMap = map[ColorModelType]protos.ColorModelType{
	HsvModelType: protos.ColorModelType_HsvColorModel,
	RgbModelType: protos.ColorModelType_RgbColorModel,
}

// new chad map uses protos from megamind/protos/common
var mmColorModelTypeToProtoMap = map[ColorModelType]common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType{
	HsvModelType: common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_HsvColorModel,
	RgbModelType: common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_RgbColorModel,
}

var mmProtoToColorModelTypeMap = map[common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType]ColorModelType{}

// old map uses our protos
var deviceTypeToProtoMap = map[DeviceType]protos.DeviceType{
	LightDeviceType:        protos.DeviceType_LightDeviceType,
	LightCeilingDeviceType: protos.DeviceType_LightCeilingDeviceType,
	LightLampDeviceType:    protos.DeviceType_LightLampDeviceType,
	LightStripDeviceType:   protos.DeviceType_LightStripDeviceType,

	SocketDeviceType: protos.DeviceType_SocketDeviceType,
	SwitchDeviceType: protos.DeviceType_SwitchDeviceType,
	HubDeviceType:    protos.DeviceType_HubDeviceType,

	PurifierDeviceType:      protos.DeviceType_PurifierDeviceType,
	HumidifierDeviceType:    protos.DeviceType_HumidifierDeviceType,
	VacuumCleanerDeviceType: protos.DeviceType_VacuumCleanerDeviceType,

	CookingDeviceType:     protos.DeviceType_CookingDeviceType,
	KettleDeviceType:      protos.DeviceType_KettleDeviceType,
	CoffeeMakerDeviceType: protos.DeviceType_CoffeeMakerDeviceType,
	MulticookerDeviceType: protos.DeviceType_MulticookerDeviceType,

	ThermostatDeviceType: protos.DeviceType_ThermostatDeviceType,
	AcDeviceType:         protos.DeviceType_AcDeviceType,

	MediaDeviceDeviceType: protos.DeviceType_MediaDeviceDeviceType,
	TvDeviceDeviceType:    protos.DeviceType_TvDeviceDeviceType,
	ReceiverDeviceType:    protos.DeviceType_ReceiverDeviceType,
	TvBoxDeviceType:       protos.DeviceType_TvBoxDeviceType,

	WashingMachineDeviceType: protos.DeviceType_WashingMachineDeviceType,
	DishwasherDeviceType:     protos.DeviceType_DishwasherDeviceType,
	RefrigeratorDeviceType:   protos.DeviceType_RefrigeratorDeviceType,
	FanDeviceType:            protos.DeviceType_FanDeviceType,
	IronDeviceType:           protos.DeviceType_IronDeviceType,
	PetFeederDeviceType:      protos.DeviceType_PetFeederDeviceType,

	CameraDeviceType: protos.DeviceType_CameraDeviceType,

	OpenableDeviceType: protos.DeviceType_OpenableDeviceType,
	CurtainDeviceType:  protos.DeviceType_CurtainDeviceType,

	SmartSpeakerDeviceType:              protos.DeviceType_SmartSpeakerDeviceType,
	YandexStationDeviceType:             protos.DeviceType_YandexStationDeviceType,
	YandexStation2DeviceType:            protos.DeviceType_YandexStation2DeviceType,
	YandexStationMiniDeviceType:         protos.DeviceType_YandexStationMiniDeviceType,
	YandexStationMini2DeviceType:        protos.DeviceType_YandexStationMini2DeviceType,
	YandexStationMini2NoClockDeviceType: protos.DeviceType_YandexStationMini2NoClockDeviceType,
	YandexStationMicroDeviceType:        protos.DeviceType_YandexStationMicroDeviceType,
	YandexStationMidiDeviceType:         protos.DeviceType_YandexStationMidiDeviceType,
	YandexStationCentaurDeviceType:      protos.DeviceType_YandexStationCentaurDeviceType,
	YandexStationChironDeviceType:       protos.DeviceType_YandexStationChironDeviceType,
	YandexStationPholDeviceType:         protos.DeviceType_YandexStationPholDeviceType,
	DexpSmartBoxDeviceType:              protos.DeviceType_DexpSmartBoxDeviceType,
	IrbisADeviceType:                    protos.DeviceType_IrbisADeviceType,
	ElariSmartBeatDeviceType:            protos.DeviceType_ElariSmartBeatDeviceType,
	LGXBoomDeviceType:                   protos.DeviceType_LGXBoomDeviceType,
	JetSmartMusicDeviceType:             protos.DeviceType_JetSmartMusicDeviceType,
	PrestigioSmartMateDeviceType:        protos.DeviceType_PrestigioSmartMateDeviceType,
	DigmaDiHomeDeviceType:               protos.DeviceType_DigmaDiHomeDeviceType,
	JBLLinkPortableDeviceType:           protos.DeviceType_JBLLinkPortableDeviceType,
	JBLLinkMusicDeviceType:              protos.DeviceType_JBLLinkMusicDeviceType,

	YandexModuleDeviceType:  protos.DeviceType_YandexModuleDeviceType,
	YandexModule2DeviceType: protos.DeviceType_YandexModule2DeviceType,

	RemoteCarDeviceType: protos.DeviceType_RemoteCarDeviceType,

	SensorDeviceType: protos.DeviceType_SensorDeviceType,

	OtherDeviceType: protos.DeviceType_OtherDeviceType,
}

// new chad map uses protos from megamind/protos/common
var mmDeviceTypeToProtoMap = map[DeviceType]devicepb.EUserDeviceType{
	// EUserDeviceType_UnknownDeviceType
	LightDeviceType:        devicepb.EUserDeviceType_LightDeviceType,
	LightCeilingDeviceType: devicepb.EUserDeviceType_LightCeilingDeviceType,
	LightLampDeviceType:    devicepb.EUserDeviceType_LightLampDeviceType,
	LightStripDeviceType:   devicepb.EUserDeviceType_LightStripDeviceType,

	SocketDeviceType: devicepb.EUserDeviceType_SocketDeviceType,
	SwitchDeviceType: devicepb.EUserDeviceType_SwitchDeviceType,
	HubDeviceType:    devicepb.EUserDeviceType_HubDeviceType,

	PurifierDeviceType:      devicepb.EUserDeviceType_PurifierDeviceType,
	HumidifierDeviceType:    devicepb.EUserDeviceType_HumidifierDeviceType,
	VacuumCleanerDeviceType: devicepb.EUserDeviceType_VacuumCleanerDeviceType,

	CookingDeviceType:     devicepb.EUserDeviceType_CookingDeviceType,
	KettleDeviceType:      devicepb.EUserDeviceType_KettleDeviceType,
	CoffeeMakerDeviceType: devicepb.EUserDeviceType_CoffeeMakerDeviceType,
	MulticookerDeviceType: devicepb.EUserDeviceType_MulticookerDeviceType,

	ThermostatDeviceType: devicepb.EUserDeviceType_ThermostatDeviceType,
	AcDeviceType:         devicepb.EUserDeviceType_AcDeviceType,

	MediaDeviceDeviceType: devicepb.EUserDeviceType_MediaDeviceDeviceType,
	TvDeviceDeviceType:    devicepb.EUserDeviceType_TvDeviceDeviceType,
	ReceiverDeviceType:    devicepb.EUserDeviceType_ReceiverDeviceType,
	TvBoxDeviceType:       devicepb.EUserDeviceType_TvBoxDeviceType,

	WashingMachineDeviceType: devicepb.EUserDeviceType_WashingMachineDeviceType,
	DishwasherDeviceType:     devicepb.EUserDeviceType_DishwasherDeviceType,
	RefrigeratorDeviceType:   devicepb.EUserDeviceType_RefrigeratorDeviceType,
	FanDeviceType:            devicepb.EUserDeviceType_FanDeviceType,
	IronDeviceType:           devicepb.EUserDeviceType_IronDeviceType,
	PetFeederDeviceType:      devicepb.EUserDeviceType_PetFeederDeviceType,

	CameraDeviceType: devicepb.EUserDeviceType_CameraDeviceType,

	OpenableDeviceType: devicepb.EUserDeviceType_OpenableDeviceType,
	CurtainDeviceType:  devicepb.EUserDeviceType_CurtainDeviceType,

	SmartSpeakerDeviceType:              devicepb.EUserDeviceType_SmartSpeakerDeviceType,
	YandexStationDeviceType:             devicepb.EUserDeviceType_YandexStationDeviceType,
	YandexStation2DeviceType:            devicepb.EUserDeviceType_YandexStation2DeviceType,
	YandexStationMiniDeviceType:         devicepb.EUserDeviceType_YandexStationMiniDeviceType,
	YandexStationMini2DeviceType:        devicepb.EUserDeviceType_YandexStationMini2DeviceType,
	YandexStationMini2NoClockDeviceType: devicepb.EUserDeviceType_YandexStationMini2NoClockDeviceType,
	YandexStationMicroDeviceType:        devicepb.EUserDeviceType_YandexStationMicroDeviceType,
	YandexStationMidiDeviceType:         devicepb.EUserDeviceType_YandexStationMidiDeviceType,
	YandexStationCentaurDeviceType:      devicepb.EUserDeviceType_YandexStationCentaurDeviceType,
	YandexStationChironDeviceType:       devicepb.EUserDeviceType_YandexStationChironDeviceType,
	YandexStationPholDeviceType:         devicepb.EUserDeviceType_YandexStationPholDeviceType,
	DexpSmartBoxDeviceType:              devicepb.EUserDeviceType_DexpSmartBoxDeviceType,
	IrbisADeviceType:                    devicepb.EUserDeviceType_IrbisADeviceType,
	ElariSmartBeatDeviceType:            devicepb.EUserDeviceType_ElariSmartBeatDeviceType,
	LGXBoomDeviceType:                   devicepb.EUserDeviceType_LGXBoomDeviceType,
	JetSmartMusicDeviceType:             devicepb.EUserDeviceType_JetSmartMusicDeviceType,
	PrestigioSmartMateDeviceType:        devicepb.EUserDeviceType_PrestigioSmartMateDeviceType,
	DigmaDiHomeDeviceType:               devicepb.EUserDeviceType_DigmaDiHomeDeviceType,
	JBLLinkPortableDeviceType:           devicepb.EUserDeviceType_JBLLinkPortableDeviceType,
	JBLLinkMusicDeviceType:              devicepb.EUserDeviceType_JBLLinkMusicDeviceType,

	YandexModuleDeviceType:  devicepb.EUserDeviceType_YandexModuleDeviceType,
	YandexModule2DeviceType: devicepb.EUserDeviceType_YandexModule2DeviceType,

	RemoteCarDeviceType: devicepb.EUserDeviceType_RemoteCarDeviceType,

	SensorDeviceType: devicepb.EUserDeviceType_SensorDeviceType,

	OtherDeviceType: devicepb.EUserDeviceType_OtherDeviceType,
}

var mmProtoToDeviceTypeMap = map[devicepb.EUserDeviceType]DeviceType{
	devicepb.EUserDeviceType_UnknownDeviceType:                   OtherDeviceType,
	devicepb.EUserDeviceType_LightDeviceType:                     LightDeviceType,
	devicepb.EUserDeviceType_LightCeilingDeviceType:              LightCeilingDeviceType,
	devicepb.EUserDeviceType_LightLampDeviceType:                 LightLampDeviceType,
	devicepb.EUserDeviceType_LightStripDeviceType:                LightStripDeviceType,
	devicepb.EUserDeviceType_SocketDeviceType:                    SocketDeviceType,
	devicepb.EUserDeviceType_SwitchDeviceType:                    SwitchDeviceType,
	devicepb.EUserDeviceType_HubDeviceType:                       HubDeviceType,
	devicepb.EUserDeviceType_PurifierDeviceType:                  PurifierDeviceType,
	devicepb.EUserDeviceType_HumidifierDeviceType:                HumidifierDeviceType,
	devicepb.EUserDeviceType_VacuumCleanerDeviceType:             VacuumCleanerDeviceType,
	devicepb.EUserDeviceType_CookingDeviceType:                   CookingDeviceType,
	devicepb.EUserDeviceType_KettleDeviceType:                    KettleDeviceType,
	devicepb.EUserDeviceType_CoffeeMakerDeviceType:               CoffeeMakerDeviceType,
	devicepb.EUserDeviceType_MulticookerDeviceType:               MulticookerDeviceType,
	devicepb.EUserDeviceType_ThermostatDeviceType:                ThermostatDeviceType,
	devicepb.EUserDeviceType_AcDeviceType:                        AcDeviceType,
	devicepb.EUserDeviceType_MediaDeviceDeviceType:               MediaDeviceDeviceType,
	devicepb.EUserDeviceType_TvDeviceDeviceType:                  TvDeviceDeviceType,
	devicepb.EUserDeviceType_ReceiverDeviceType:                  ReceiverDeviceType,
	devicepb.EUserDeviceType_TvBoxDeviceType:                     TvBoxDeviceType,
	devicepb.EUserDeviceType_WashingMachineDeviceType:            WashingMachineDeviceType,
	devicepb.EUserDeviceType_DishwasherDeviceType:                DishwasherDeviceType,
	devicepb.EUserDeviceType_RefrigeratorDeviceType:              RefrigeratorDeviceType,
	devicepb.EUserDeviceType_FanDeviceType:                       FanDeviceType,
	devicepb.EUserDeviceType_IronDeviceType:                      IronDeviceType,
	devicepb.EUserDeviceType_PetFeederDeviceType:                 PetFeederDeviceType,
	devicepb.EUserDeviceType_OpenableDeviceType:                  OpenableDeviceType,
	devicepb.EUserDeviceType_CurtainDeviceType:                   CurtainDeviceType,
	devicepb.EUserDeviceType_SmartSpeakerDeviceType:              SmartSpeakerDeviceType,
	devicepb.EUserDeviceType_YandexStationDeviceType:             YandexStationDeviceType,
	devicepb.EUserDeviceType_YandexStation2DeviceType:            YandexStation2DeviceType,
	devicepb.EUserDeviceType_YandexStationMiniDeviceType:         YandexStationMiniDeviceType,
	devicepb.EUserDeviceType_YandexStationMini2DeviceType:        YandexStationMini2DeviceType,
	devicepb.EUserDeviceType_YandexStationMini2NoClockDeviceType: YandexStationMini2NoClockDeviceType,
	devicepb.EUserDeviceType_YandexStationMicroDeviceType:        YandexStationMicroDeviceType,
	devicepb.EUserDeviceType_YandexStationMidiDeviceType:         YandexStationMidiDeviceType,
	devicepb.EUserDeviceType_YandexStationCentaurDeviceType:      YandexStationCentaurDeviceType,
	devicepb.EUserDeviceType_YandexStationChironDeviceType:       YandexStationChironDeviceType,
	devicepb.EUserDeviceType_YandexStationPholDeviceType:         YandexStationPholDeviceType,
	devicepb.EUserDeviceType_DexpSmartBoxDeviceType:              DexpSmartBoxDeviceType,
	devicepb.EUserDeviceType_IrbisADeviceType:                    IrbisADeviceType,
	devicepb.EUserDeviceType_ElariSmartBeatDeviceType:            ElariSmartBeatDeviceType,
	devicepb.EUserDeviceType_LGXBoomDeviceType:                   LGXBoomDeviceType,
	devicepb.EUserDeviceType_JetSmartMusicDeviceType:             JetSmartMusicDeviceType,
	devicepb.EUserDeviceType_PrestigioSmartMateDeviceType:        PrestigioSmartMateDeviceType,
	devicepb.EUserDeviceType_DigmaDiHomeDeviceType:               DigmaDiHomeDeviceType,
	devicepb.EUserDeviceType_JBLLinkPortableDeviceType:           JBLLinkPortableDeviceType,
	devicepb.EUserDeviceType_JBLLinkMusicDeviceType:              JBLLinkMusicDeviceType,
	devicepb.EUserDeviceType_YandexModuleDeviceType:              YandexModuleDeviceType,
	devicepb.EUserDeviceType_YandexModule2DeviceType:             YandexModule2DeviceType,
	devicepb.EUserDeviceType_RemoteCarDeviceType:                 RemoteCarDeviceType,
	devicepb.EUserDeviceType_SensorDeviceType:                    SensorDeviceType,
	devicepb.EUserDeviceType_CameraDeviceType:                    CameraDeviceType,
	devicepb.EUserDeviceType_OtherDeviceType:                     OtherDeviceType,
}

var mmDeviceStatusToProtoMap = map[DeviceStatus]common.TIoTUserInfo_TDevice_EDeviceState{
	UnknownDeviceStatus:  common.TIoTUserInfo_TDevice_UnknownDeviceState,
	OnlineDeviceStatus:   common.TIoTUserInfo_TDevice_OnlineDeviceState,
	OfflineDeviceStatus:  common.TIoTUserInfo_TDevice_OfflineDeviceState,
	NotFoundDeviceStatus: common.TIoTUserInfo_TDevice_NotFoundDeviceState,
	SplitStatus:          common.TIoTUserInfo_TDevice_SplitState,
}

var mmProtoToDeviceStatusMap = map[common.TIoTUserInfo_TDevice_EDeviceState]DeviceStatus{
	common.TIoTUserInfo_TDevice_UnknownDeviceState:  UnknownDeviceStatus,
	common.TIoTUserInfo_TDevice_OnlineDeviceState:   OnlineDeviceStatus,
	common.TIoTUserInfo_TDevice_OfflineDeviceState:  OfflineDeviceStatus,
	common.TIoTUserInfo_TDevice_NotFoundDeviceState: NotFoundDeviceStatus,
	common.TIoTUserInfo_TDevice_SplitState:          SplitStatus,
}

var mmMusicPlayObjectTypeToProtoMap = map[MusicPlayObjectType]common.TMusicPlayObjectTypeSlot_EValue{
	TrackMusicPlayObjectType:          common.TMusicPlayObjectTypeSlot_Track,
	AlbumMusicPlayObjectType:          common.TMusicPlayObjectTypeSlot_Album,
	ArtistMusicPlayObjectType:         common.TMusicPlayObjectTypeSlot_Artist,
	PlaylistMusicPlayObjectType:       common.TMusicPlayObjectTypeSlot_Playlist,
	RadioMusicPlayObjectType:          common.TMusicPlayObjectTypeSlot_Radio,
	GenerativeMusicPlayObjectType:     common.TMusicPlayObjectTypeSlot_Generative,
	PodcastMusicPlayObjectType:        common.TMusicPlayObjectTypeSlot_Album, // megamind supports podcasts as albums,
	PodcastEpisodeMusicPlayObjectType: common.TMusicPlayObjectTypeSlot_Track, // episodes as tracks
}

var RelativeFlagNonSupportingSkills = map[string]bool{
	TUYA:                                   true,
	QUASAR:                                 true,
	VIRTUAL:                                true,
	"357e40f9-640d-462e-8c39-8ecdade9b611": true,
	"3d1b38d7-ab99-44fe-a799-0daa65202358": true,
	"ad26f8c2-fc31-4928-a653-d829fda7e6c2": true, //xiaomi
	"4a8cbce2-61d3-4e58-9f7b-6b30371d265c": true,
	"14b3d60c-36d7-42e3-adaa-3bf7bbd9dab8": true,
	"c178cd89-5bdb-44f2-b3ae-9bb328a36f7b": true,
	"35e2897a-c583-495a-9e33-f5d6f0f4cb49": true,
	"f1dc4741-402d-40a9-86bc-480356b68f75": true,
	"cd65d53a-0106-4708-bc6d-2042cb1d46c3": true,
	"f90771df-9756-4c81-9495-50825e7eeff8": true,
	"ffdfeaae-536f-42e6-bb80-a9ef73f39240": true,
	"311bf19d-2873-4c52-9a69-d20bd9410cf2": true,
	"fe03b0ca-5a70-4210-96d5-a4f1c10d96a8": true,
	"be937798-ec53-457b-aa7e-e928c05768b0": true,
	"457299b6-174e-4373-9064-9c7b8cab27d1": true,
	"580f0b9c-3420-489d-b7c9-4bec87057d06": true,
	"24b419c6-135f-4d28-b7cb-8a8da2fc5f1f": true,
	"b91b16fc-3e95-4e42-9654-a4f685e5bc44": true,
	"4172ed0a-a4d3-4d57-ac42-93d9a5f28668": true,
	"3601a821-be82-4185-9dc1-bf47476a0c99": true,
	"966138ad-35e5-4118-b47c-7a9f5bf17674": true,
	"c927bb15-5ecb-472a-8895-c3740602d36a": true,
	"4e5c1f77-4832-4f25-923f-935b1d9f32d6": true,
	"0a36403d-0b70-4305-85d8-7da221d8f91c": true,
	"a7fc411f-0fe2-461e-915b-7ff11969c551": true,
	"2d7a8c3a-4326-49f4-b0ab-de5c81916614": true,
	"fc4fd3aa-a19b-4b55-9ea9-7e2289e9b67f": true,
	"4b7c9be0-3a7a-4591-80dc-f1cd37b49a9d": true,
	"a061fc56-af82-4d58-be45-5ab534881292": true,
	"c43a5814-508b-44aa-afde-10442c10e7ba": true,
	"997adf4b-22f9-4fa0-8305-850c18a787dc": true,
	"85eb5745-659c-4730-98d7-3a13dd6a6fb8": true,
	"541a365a-100e-45d6-84f2-6c8770e31bab": true,
	"5f7f874e-9978-4329-b521-a276c4f1bec6": true,
	"91ddede6-c5e4-4662-9e9c-bf60e184f907": true,
	"43606352-ee3f-4ec8-a131-2c754551b4d2": true,
	"023a3727-ade4-494d-a82e-6880ed2348b2": true,
	"13521c6a-bde1-4d62-9d67-10e0562c7543": true,
	"909af191-576d-4fda-95b8-cd0cf2d6dbbb": true,
	"99ef8285-8e88-47d6-ae85-179f0e9c5b35": true,
	"e6a3a900-96c9-42ea-9472-4211d69b6011": true,
}

var KnownBackgroundImages = map[BackgroundImageID]BackgroundImage{
	LivingRoomBackgroundImageID: {
		ID: LivingRoomBackgroundImageID,
	},
	BedroomBackgroundImageID: {
		ID: BedroomBackgroundImageID,
	},
	KitchenBackgroundImageID: {
		ID: KitchenBackgroundImageID,
	},
	ChildrenRoomBackgroundImageID: {
		ID: ChildrenRoomBackgroundImageID,
	},
	HallwayBackgroundImageID: {
		ID: HallwayBackgroundImageID,
	},
	BathroomBackgroundImageID: {
		ID: BathroomBackgroundImageID,
	},
	WorkspaceBackgroundImageID: {
		ID: WorkspaceBackgroundImageID,
	},
	CountryHouseBackgroundImageID: {
		ID: CountryHouseBackgroundImageID,
	},
	AllBackgroundImageID: {
		ID: AllBackgroundImageID,
	},
	BalconyBackgroundImageID: {
		ID: BalconyBackgroundImageID,
	},
	MyRoomBackgroundImageID: {
		ID: MyRoomBackgroundImageID,
	},
	ScenariosBackgroundImageID: {
		ID: ScenariosBackgroundImageID,
	},
	FavoriteBackgroundImageID: {
		ID: FavoriteBackgroundImageID,
	},
	Default1BackgroundImageID: {
		ID: Default1BackgroundImageID,
	},
	Default2BackgroundImageID: {
		ID: Default2BackgroundImageID,
	},
	Default3BackgroundImageID: {
		ID: Default3BackgroundImageID,
	},
	Default4BackgroundImageID: {
		ID: Default4BackgroundImageID,
	},
}

var knownSpeakerSoundsArray = []SpeakerSound{
	{ID: "chainsaw-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Бензопила"},
	{ID: "explosion-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Взрыв"},
	{ID: "water-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Вода №1 (льется)"},
	{ID: "water-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Вода №2 (бурлит)"},
	{ID: "water-3", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Вода (наливается в стакан)"},
	{ID: "switch-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Выключатель №1"},
	{ID: "switch-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Выключатель №2"},
	{ID: "gun-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Выстрел (дробовик)"},
	{ID: "ship-horn-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Гудок корабля №1"},
	{ID: "ship-horn-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Гудок корабля №2"},
	{ID: "door-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Дверь №1"},
	{ID: "door-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Дверь №2"},
	{ID: "glass-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Стекло (разбивается)"},
	{ID: "glass-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Звон бокалов"},
	{ID: "bell-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Колокол №1"},
	{ID: "bell-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Колокол №2"},
	{ID: "car-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Машина (заводится)"},
	{ID: "car-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Машина (не заводится)"},
	{ID: "sword-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Меч (парирование)"},
	{ID: "sword-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Меч (выходит из ножен)"},
	{ID: "sword-3", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Меч (поединок)"},
	{ID: "siren-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Сирена №1"},
	{ID: "siren-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Сирена №2"},
	{ID: "old-phone-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Старый телефон №1"},
	{ID: "old-phone-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Старый телефон №2"},
	{ID: "construction-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Строительство (пила и молоток)"},
	{ID: "construction-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Строительство (отбойный молоток)"},
	{ID: "phone-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Телефон №1 (звонок)"},
	{ID: "phone-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Телефон №2 (звонок)"},
	{ID: "phone-3", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Телефон №3 (набор номера)"},
	{ID: "phone-4", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Телефон №4 (гудок)"},
	{ID: "phone-5", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Телефон №5 (гудок)"},
	{ID: "toilet-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Унитаз"},
	{ID: "cuckoo-clock-1", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Часы с кукушкой №1"},
	{ID: "cuckoo-clock-2", CategoryID: ThingsSpeakerSoundCategoryID, Name: "Часы с кукушкой №2"},
	{ID: "wolf-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Волк"},
	{ID: "crow-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Ворона №1"},
	{ID: "crow-2", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Ворона №2"},
	{ID: "cow-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Корова №1"},
	{ID: "cow-2", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Корова №2"},
	{ID: "cow-3", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Корова №3"},
	{ID: "cat-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Кошка №1 (мяуканье)"},
	{ID: "cat-2", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Кошка №2 (мяуканье)"},
	{ID: "cat-3", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Кошка №3 (мяуканье)"},
	{ID: "cat-4", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Кошка №4 (мурчание)"},
	{ID: "cat-5", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Кошка №4 (шипение)"},
	{ID: "cuckoo-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Кукушка"},
	{ID: "chicken-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Курица"},
	{ID: "lion-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Лев №1"},
	{ID: "lion-2", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Лев №2"},
	{ID: "horse-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Лошадь №1 (ржание)"},
	{ID: "horse-2", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Лошадь №2 (фырканье)"},
	{ID: "horse-galloping-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Лошадь №3 (галоп)"},
	{ID: "horse-walking-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Лошадь №4 (шаг)"},
	{ID: "frog-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Лягушка"},
	{ID: "seagull-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Морской котик"},
	{ID: "monkey-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Обезьяна"},
	{ID: "sheep-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Овца №1"},
	{ID: "sheep-2", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Овца №2 (несколько)"},
	{ID: "rooster-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Петух"},
	{ID: "elephant-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Слон №1"},
	{ID: "elephant-2", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Слон №2"},
	{ID: "dog-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Собака №1 (лай)"},
	{ID: "dog-2", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Собака №2 (рык)"},
	{ID: "dog-3", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Собака №3 (скуление)"},
	{ID: "dog-4", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Собака №4 (лай)"},
	{ID: "dog-5", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Собака №5 (лай)"},
	{ID: "owl-1", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Сова №1"},
	{ID: "owl-2", CategoryID: AnimalsSpeakerSoundCategoryID, Name: "Сова №2"},
	{ID: "boot-1", CategoryID: GameSpeakerSoundCategoryID, Name: "Загрузка (8 бит)"},
	{ID: "8-bit-coin-1", CategoryID: GameSpeakerSoundCategoryID, Name: "Монета (8 бит) №1"},
	{ID: "8-bit-coin-2", CategoryID: GameSpeakerSoundCategoryID, Name: "Монета (8 бит) №2"},
	{ID: "loss-1", CategoryID: GameSpeakerSoundCategoryID, Name: "Неудача №1"},
	{ID: "loss-2", CategoryID: GameSpeakerSoundCategoryID, Name: "Неудача №2"},
	{ID: "loss-3", CategoryID: GameSpeakerSoundCategoryID, Name: "Неудача №3"},
	{ID: "ping-1", CategoryID: GameSpeakerSoundCategoryID, Name: "Оповещение (8 бит)"},
	{ID: "win-1", CategoryID: GameSpeakerSoundCategoryID, Name: "Победные фанфары №1"},
	{ID: "win-2", CategoryID: GameSpeakerSoundCategoryID, Name: "Победные фанфары №2"},
	{ID: "win-3", CategoryID: GameSpeakerSoundCategoryID, Name: "Победные фанфары №3"},
	{ID: "8-bit-flyby-1", CategoryID: GameSpeakerSoundCategoryID, Name: "Полет (8 бит)"},
	{ID: "8-bit-machine-gun-1", CategoryID: GameSpeakerSoundCategoryID, Name: "Пулемет (8 бит)"},
	{ID: "8-bit-phone-1", CategoryID: GameSpeakerSoundCategoryID, Name: "Телефон (8 бит)"},
	{ID: "powerup-1", CategoryID: GameSpeakerSoundCategoryID, Name: "Усиление (8 бит) №1"},
	{ID: "powerup-2", CategoryID: GameSpeakerSoundCategoryID, Name: "Усиление (8 бит) №2"},
	{ID: "cheer-1", CategoryID: HumanSpeakerSoundCategoryID, Name: "Аплодисменты"},
	{ID: "cheer-2", CategoryID: HumanSpeakerSoundCategoryID, Name: "Болельщики"},
	{ID: "kids-1", CategoryID: HumanSpeakerSoundCategoryID, Name: "Дети"},
	{ID: "walking-dead-1", CategoryID: HumanSpeakerSoundCategoryID, Name: "Зомби №1 (рык)"},
	{ID: "walking-dead-2", CategoryID: HumanSpeakerSoundCategoryID, Name: "Зомби №2 (стон)"},
	{ID: "walking-dead-3", CategoryID: HumanSpeakerSoundCategoryID, Name: "Зомби №3 (рык с криком)"},
	{ID: "cough-1", CategoryID: HumanSpeakerSoundCategoryID, Name: "Кашель №1"},
	{ID: "cough-2", CategoryID: HumanSpeakerSoundCategoryID, Name: "Кашель №2"},
	{ID: "laugh-1", CategoryID: HumanSpeakerSoundCategoryID, Name: "Смех №1"},
	{ID: "laugh-2", CategoryID: HumanSpeakerSoundCategoryID, Name: "Смех №2"},
	{ID: "laugh-3", CategoryID: HumanSpeakerSoundCategoryID, Name: "Смех №3 (злодейский)"},
	{ID: "laugh-4", CategoryID: HumanSpeakerSoundCategoryID, Name: "Смех №4 (злодейский)"},
	{ID: "laugh-5", CategoryID: HumanSpeakerSoundCategoryID, Name: "Смех №5 (детский)"},
	{ID: "crowd-1", CategoryID: HumanSpeakerSoundCategoryID, Name: "Толпа №1 (разговоры)"},
	{ID: "crowd-2", CategoryID: HumanSpeakerSoundCategoryID, Name: "Толпа №2 (удивление)"},
	{ID: "crowd-3", CategoryID: HumanSpeakerSoundCategoryID, Name: "Толпа №3 (аплодисменты)"},
	{ID: "crowd-4", CategoryID: HumanSpeakerSoundCategoryID, Name: "Толпа №4 (бурные аплодисменты)"},
	{ID: "crowd-5", CategoryID: HumanSpeakerSoundCategoryID, Name: "Толпа №5 (болельщики)"},
	{ID: "crowd-6", CategoryID: HumanSpeakerSoundCategoryID, Name: "Толпа №6 (одобрительные крики)"},
	{ID: "crowd-7", CategoryID: HumanSpeakerSoundCategoryID, Name: "Толпа №6 (недовольство)"},
	{ID: "sneeze-1", CategoryID: HumanSpeakerSoundCategoryID, Name: "Чихание №1"},
	{ID: "sneeze-2", CategoryID: HumanSpeakerSoundCategoryID, Name: "Чихание №2"},
	{ID: "walking-room-1", CategoryID: HumanSpeakerSoundCategoryID, Name: "Шаги в комнате"},
	{ID: "walking-snow-1", CategoryID: HumanSpeakerSoundCategoryID, Name: "Шаги на снегу"},
	{ID: "walking-leaves-1", CategoryID: HumanSpeakerSoundCategoryID, Name: "Шаги по листьям"},
	{ID: "harp-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Арфа"},
	{ID: "drums-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Барабанный проигрыш №1"},
	{ID: "drums-2", CategoryID: MusicSpeakerSoundCategoryID, Name: "Барабанный проигрыш №2"},
	{ID: "drums-3", CategoryID: MusicSpeakerSoundCategoryID, Name: "Барабанный проигрыш №3"},
	{ID: "drum-loop-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Бит №1 (быстро)"},
	{ID: "drum-loop-2", CategoryID: MusicSpeakerSoundCategoryID, Name: "Бит №2 (медленно)"},
	{ID: "tambourine-80bpm-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Бубен №1 (80 ударов в минуту)"},
	{ID: "tambourine-100bpm-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Бубен №2 (100 ударов в минуту)"},
	{ID: "tambourine-120bpm-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Бубен №3 (120 ударов в минуту)"},
	{ID: "bagpipes-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Волынка №1"},
	{ID: "bagpipes-2", CategoryID: MusicSpeakerSoundCategoryID, Name: "Волынка №2"},
	{ID: "guitar-c-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Гитара, аккорд C"},
	{ID: "guitar-e-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Гитара, аккорд E"},
	{ID: "guitar-g-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Гитара, аккорд G"},
	{ID: "guitar-a-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Гитара, аккорд A"},
	{ID: "gong-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Гонг №1"},
	{ID: "gong-2", CategoryID: MusicSpeakerSoundCategoryID, Name: "Гонг №2"},
	{ID: "horn-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Труба болельщика"},
	{ID: "horn-2", CategoryID: MusicSpeakerSoundCategoryID, Name: "Горн"},
	{ID: "violin-c-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Скрипка (до)"},
	{ID: "violin-c-2", CategoryID: MusicSpeakerSoundCategoryID, Name: "Скрипка (до верхнее)"},
	{ID: "violin-a-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Скрипка (ля)"},
	{ID: "violin-e-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Скрипка (ми)"},
	{ID: "violin-d-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Скрипка (ре)"},
	{ID: "violin-b-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Скрипка (си)"},
	{ID: "violin-g-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Скрипка (соль)"},
	{ID: "violin-f-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Скрипка (фа)"},
	{ID: "piano-c-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Фортепиано (до)"},
	{ID: "piano-c-2", CategoryID: MusicSpeakerSoundCategoryID, Name: "Фортепиано (до верхнее)"},
	{ID: "piano-a-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Фортепиано (ля)"},
	{ID: "piano-e-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Фортепиано (ми)"},
	{ID: "piano-d-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Фортепиано (ре)"},
	{ID: "piano-b-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Фортепиано (си)"},
	{ID: "piano-g-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Фортепиано (соль)"},
	{ID: "piano-f-1", CategoryID: MusicSpeakerSoundCategoryID, Name: "Фортепиано (фа)"},
	{ID: "wind-1", CategoryID: NatureSpeakerSoundCategoryID, Name: "Ветер №1"},
	{ID: "wind-2", CategoryID: NatureSpeakerSoundCategoryID, Name: "Ветер №2"},
	{ID: "thunder-1", CategoryID: NatureSpeakerSoundCategoryID, Name: "Гром №1"},
	{ID: "thunder-2", CategoryID: NatureSpeakerSoundCategoryID, Name: "Гром №2"},
	{ID: "jungle-1", CategoryID: NatureSpeakerSoundCategoryID, Name: "Джунгли №1"},
	{ID: "jungle-2", CategoryID: NatureSpeakerSoundCategoryID, Name: "Джунгли №2"},
	{ID: "rain-1", CategoryID: NatureSpeakerSoundCategoryID, Name: "Дождь №1"},
	{ID: "rain-2", CategoryID: NatureSpeakerSoundCategoryID, Name: "Дождь №2"},
	{ID: "forest-1", CategoryID: NatureSpeakerSoundCategoryID, Name: "Лес №1"},
	{ID: "forest-2", CategoryID: NatureSpeakerSoundCategoryID, Name: "Лес №2"},
	{ID: "sea-1", CategoryID: NatureSpeakerSoundCategoryID, Name: "Море №1"},
	{ID: "sea-2", CategoryID: NatureSpeakerSoundCategoryID, Name: "Море №2"},
	{ID: "fire-1", CategoryID: NatureSpeakerSoundCategoryID, Name: "Огонь №1"},
	{ID: "fire-2", CategoryID: NatureSpeakerSoundCategoryID, Name: "Огонь №2"},
	{ID: "stream-1", CategoryID: NatureSpeakerSoundCategoryID, Name: "Ручей №1"},
	{ID: "stream-2", CategoryID: NatureSpeakerSoundCategoryID, Name: "Ручей №2"},
}

var KnownSpeakerSoundCategories = map[SpeakerSoundCategoryID]SpeakerSoundCategory{
	ThingsSpeakerSoundCategoryID:  {ID: ThingsSpeakerSoundCategoryID, Name: "Вещи"},
	AnimalsSpeakerSoundCategoryID: {ID: AnimalsSpeakerSoundCategoryID, Name: "Животные"},
	GameSpeakerSoundCategoryID:    {ID: GameSpeakerSoundCategoryID, Name: "Игры"},
	HumanSpeakerSoundCategoryID:   {ID: HumanSpeakerSoundCategoryID, Name: "Люди"},
	MusicSpeakerSoundCategoryID:   {ID: MusicSpeakerSoundCategoryID, Name: "Музыка"},
	NatureSpeakerSoundCategoryID:  {ID: NatureSpeakerSoundCategoryID, Name: "Природа"},
}

var KnownSpeakerMusicPlayObjectTypes = []string{
	string(TrackMusicPlayObjectType),
	string(AlbumMusicPlayObjectType),
	string(ArtistMusicPlayObjectType),
	string(PlaylistMusicPlayObjectType),
	string(RadioMusicPlayObjectType),
	string(GenerativeMusicPlayObjectType),
	string(PodcastMusicPlayObjectType),
	string(PodcastEpisodeMusicPlayObjectType),
}

var KnownVideoStreamProtocols = []string{
	string(HLSStreamingProtocol),
	string(ProgressiveMP4StreamingProtocol),
}

// timer trigger scenarios
const DelayedScenarioMaxDuration = 7 * 24 * time.Hour // 1 week
const DefaultTimezone = "Europe/Moscow"
const DelayedScenarioMaxOvertime = 3 * time.Minute
const InvokedScenarioMaxOvertime = 3 * time.Hour

const (
	ScenarioLaunchScheduled ScenarioLaunchStatus = "SCHEDULED"
	ScenarioLaunchInvoked   ScenarioLaunchStatus = "INVOKED"
	ScenarioLaunchDone      ScenarioLaunchStatus = "DONE"
	ScenarioLaunchFailed    ScenarioLaunchStatus = "FAILED"
	ScenarioLaunchCanceled  ScenarioLaunchStatus = "CANCELED"

	ScenarioAll ScenarioLaunchStatus = "ALL" // artificial status for mobile handlers - do not use in db
)

const ScenarioVoiceTriggersNumLimit = 4     // if you dare to change this, be so kind and fix TriggersLimitReachedErrorMessage also
const ScenarioTimetableTriggersNumLimit = 1 // if you dare to change this, be so kind and fix TimetableTriggersLimitReachedErrorMessage also

const MockScenarioTriggerID = "mock-trigger"

const DefaultHouseholdID = "default"

type IconFormat string

const (
	RawIconFormat        IconFormat = "raw" // for just urls without formatting
	OriginalIconFormat   IconFormat = "orig"
	PNG40x40IconFormat   IconFormat = "png40x40"
	PNG80x80IconFormat   IconFormat = "png80x80"
	PNG120x120IconFormat IconFormat = "png120x120"
)

const (
	InappropriateHouseholdFilterReason        HypothesisFilterReason = "INAPPROPRIATE_HOUSEHOLD"
	InappropriateRoomFilterReason             HypothesisFilterReason = "INAPPROPRIATE_ROOM"
	InappropriateGroupFilterReason            HypothesisFilterReason = "INAPPROPRIATE_GROUP"
	InappropriateDevicesFilterReason          HypothesisFilterReason = "INAPPROPRIATE_DEVICES"
	TandemTVHypothesisFilterReason            HypothesisFilterReason = "TANDEM_TV_HYPOTHESIS"
	InappropriateCapabilityFilterReason       HypothesisFilterReason = "INAPPROPRIATE_CAPABILITY"
	InappropriateTurnOnAllDevicesFilterReason HypothesisFilterReason = "INAPPROPRIATE_TURN_ON_ALL_DEVICES"
	ShouldSpecifyHouseholdFilterReason        HypothesisFilterReason = "SHOULD_SPECIFY_HOUSEHOLD"
	AllGoodFilterReason                       HypothesisFilterReason = "ALL_GOOD"
)

type DeviceTriggerEntity string

const (
	CapabilityEntity DeviceTriggerEntity = "capability"
	PropertyEntity   DeviceTriggerEntity = "property"
)

type TimePeriod string

const (
	TimePeriodAM TimePeriod = "am"
	TimePeriodPM TimePeriod = "pm"
)

const (
	ScenarioStepActionsType ScenarioStepType = "scenarios.steps.actions"
	ScenarioStepDelayType   ScenarioStepType = "scenarios.steps.delay"
)

const (
	DoneScenarioLaunchDeviceActionStatus  ScenarioLaunchDeviceActionStatus = "done"
	ErrorScenarioLaunchDeviceActionStatus ScenarioLaunchDeviceActionStatus = "error"
)

const (
	DeviceFavoriteType         FavoriteType = "device"
	ScenarioFavoriteType       FavoriteType = "scenario"
	StereopairFavoriteType     FavoriteType = "stereopair"
	GroupFavoriteType          FavoriteType = "group"
	DevicePropertyFavoriteType FavoriteType = "device_property"
)
