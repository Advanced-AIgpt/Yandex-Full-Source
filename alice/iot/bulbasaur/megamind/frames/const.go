package frames

import (
	"a.yandex-team.ru/alice/library/go/libmegamind"
)

const (
	CancelAllScenariosFrameName           libmegamind.SemanticFrameName = "alice.iot.scenarios_cancel_all"
	CreateScenarioFrameName               libmegamind.SemanticFrameName = "alice.iot.scenarios_create"
	SpeakerActionFrameName                libmegamind.SemanticFrameName = "alice.iot.scenario.speaker.action"
	SpeakerActionDoneCallbackName         libmegamind.SemanticFrameName = "alice.iot.scenario.speaker.action.done"
	ScenarioStepActionsTypedSemanticFrame libmegamind.SemanticFrameName = "alice.iot.scenario.step.actions"
)

const (
	CancelOnetimeScenarioFrame = "alice.iot.scenarios_cancel_onetime_scenario"
	CancelScenarioCallbackName = "cancel_onetime_scenario"
)

// Granet frames
const (
	PetFeederFrame          libmegamind.SemanticFrameName = "alice.iot.pet_feeder"
	FindDevicesFrameName    libmegamind.SemanticFrameName = "alice.iot.discovery.find_devices"
	ScenarioLaunchFrameName libmegamind.SemanticFrameName = "alice.iot.scenario.launch"
	StartSearchFrameName    libmegamind.SemanticFrameName = "alice.iot.discovery.start_search"
	HowToDiscoveryFrameName libmegamind.SemanticFrameName = "alice.iot.discovery.how_to"

	ActionCapabilityColorSettingFrameName libmegamind.SemanticFrameName = "alice.iot.action.capability.color_setting"
	ActionCapabilityCustomButtonFrameName libmegamind.SemanticFrameName = "alice.iot.action.capability.custom_button"
	ActionCapabilityModeFrameName         libmegamind.SemanticFrameName = "alice.iot.action.capability.mode"
	ActionCapabilityOnOffFrameName        libmegamind.SemanticFrameName = "alice.iot.action.capability.on_off"
	ActionCapabilityRangeFrameName        libmegamind.SemanticFrameName = "alice.iot.action.capability.range"
	ActionCapabilityToggleFrameName       libmegamind.SemanticFrameName = "alice.iot.action.capability.toggle"
	ActionCapabilityVideoStreamFrameName  libmegamind.SemanticFrameName = "alice.iot.action.show_video_stream"

	QueryCapabilityColorSettingFrameName libmegamind.SemanticFrameName = "alice.iot.query.capability.color.setting"
	QueryCapabilityModeFrameName         libmegamind.SemanticFrameName = "alice.iot.query.capability.mode"
	QueryCapabilityOnOffFrameName        libmegamind.SemanticFrameName = "alice.iot.query.capability.on.off"
	QueryCapabilityRangeFrameName        libmegamind.SemanticFrameName = "alice.iot.query.capability.range"
	QueryCapabilityToggleFrameName       libmegamind.SemanticFrameName = "alice.iot.query.capability.toggle"
	QueryPropertyFloatFrameName          libmegamind.SemanticFrameName = "alice.iot.query.property.float"
	QueryStateFrameName                  libmegamind.SemanticFrameName = "alice.iot.query.state"

	SpecifyTimeFrameName libmegamind.SemanticFrameName = "alice.iot.scenarios_timespecify"
)

// Typed Semantic Frames
const (
	DeviceActionTypedSemanticFrame libmegamind.SemanticFrameName = "alice.iot.action.device"
	StartDiscoveryFrameName        libmegamind.SemanticFrameName = "alice.iot.discovery.start"
	FinishDiscoveryFrameName       libmegamind.SemanticFrameName = "alice.iot.discovery.finish"
	FinishSystemDiscoveryFrameName libmegamind.SemanticFrameName = "alice.iot.system_discovery.finish"
	StartTuyaBroadcastFrameName    libmegamind.SemanticFrameName = "alice.iot.discovery.start_tuya_broadcast"

	CentaurCollectMainScreenFrameName libmegamind.SemanticFrameName = "alice.centaur.collect_main_screen"

	VoiceprintStatusFrameName libmegamind.SemanticFrameName = "alice.multiaccount.enrollment_status"
)

func IsTypedSemanticFrame(frameName libmegamind.SemanticFrameName) bool {
	return TypedSemanticFrames[frameName]
}

var TypedSemanticFrames = map[libmegamind.SemanticFrameName]bool{
	DeviceActionTypedSemanticFrame: true,
}

var SupportsTimeSpecification = map[libmegamind.SemanticFrameName]bool{
	PetFeederFrame:                 true,
	ActionCapabilityOnOffFrameName: true,
	ScenarioLaunchFrameName:        true,
}

// DemoRoomIDToName contains human-readable names of demo rooms.
// Entries are taken from alice/library/iot/data/demo_sh.json.
var DemoRoomIDToName = map[string]string{
	"bedroom":     "спальня",
	"kitchen":     "кухня",
	"hall1":       "гостиная",
	"hall2":       "зал",
	"lobby1":      "коридор",
	"bathroom":    "ванная",
	"lobby2":      "прихожая",
	"nursery":     "детская",
	"toilet":      "туалет",
	"room":        "комната",
	"office1":     "кабинет",
	"balcony":     "балкон",
	"living_room": "living room",
	"office2":     "офис",
}

// DemoDeviceIDToName contains human-readable names of demo devices.
// Entries are taken from alice/library/iot/data/demo_sh.json.
var DemoDeviceIDToName = map[string]string{
	"light1":        "лампочка",
	"light2":        "лампа",
	"light3":        "люстра",
	"light4":        "ночник",
	"light5":        "настольная лампа",
	"light6":        "светильник",
	"light7":        "торшер",
	"light8":        "лента",
	"light9":        "лампа 2",
	"light10":       "yeelight color bulb",
	"light11":       "бра",
	"socket1":       "розетка",
	"socket2":       "удлинитель",
	"socket3":       "sky socket",
	"socket4":       "умная розетка",
	"socket5":       "сетевая розетка",
	"socket_fan":    "вентилятор",
	"socket_light1": "подсветка",
	"socket_other2": "вытяжка",
	"socket_light2": "гирлянда",
	"socket_mo":     "микроволновка",
	"other_vc1":     "пылесос",
	"other_kettle1": "чайник",
	"other_kettle2": "sky kettle",
	"other_vc2":     "робот пылесос",
	"other_wm":      "стиральная машина",
	"other_vc3":     "робот-пылесос",
	"switch1":       "выключатель",
	"ac1":           "кондиционер",
	"ac2":           "кондей",
	"other1":        "очиститель воздуха",
	"other3":        "кофеварка",
	"other4":        "утюг",
	"other5":        "термостат",
	"other6":        "гриль",
	"other7":        "фильтр",
	"other8":        "обогреватель",
	"other9":        "увлажнитель воздуха",
	"other10":       "кофемашина",
	"other11":       "бойлер",
	"other12":       "переключатель",
	"other13":       "электророзетка",
	"other14":       "духовка",
	"other15":       "плита",
	"other16":       "термопот",
	"other17":       "рисоварка",
	"other18":       "мультиварка",
	"other19":       "пароварка",
	"other20":       "конвектор",
	"other21":       "водонагреватель",
	"other22":       "тепловентилятор",
	"other23":       "диммер",
	"other24":       "электрокамин",
	"media_device1": "ресивер",
	"media_device2": "телевизор",
}

// DeviceTypeToName contains human-readable names of device types.
// Entries are taken from iot granets
var DeviceTypeToName = map[string]string{
	"devices.types.socket":                "розетка",
	"devices.types.light":                 "свет",
	"devices.types.switch":                "переключатель",
	"devices.types.thermostat.ac":         "кондиционер",
	"devices.types.media_device.tv":       "телевизор",
	"devices.types.media_device.receiver": "ресивер",
	"devices.types.media_device.tv_box":   "приставка",
	"devices.types.cooking.kettle":        "чайник",
	"devices.types.openable.curtain":      "шторы",
	"devices.types.purifier":              "очиститель воздуха",
	"devices.types.vacuum_cleaner":        "пылесос",
	"devices.types.cooking.coffee_maker":  "кофеварка",
	"devices.types.humidifier":            "увлажнитель воздуха",
	"devices.types.washing_machine":       "стиральная машина",
	"devices.types.remote_car":            "авто",
	"devices.types.dishwasher":            "посудомойка",
	"devices.types.cooking.multicooker":   "мультиварка",
	"devices.types.refrigerator":          "холодильник",
	"devices.types.iron":                  "утюг",
	"devices.types.fan":                   "вентилятор",
	"devices.types.sensor":                "сенсор",
	"devices.types.pet_feeder":            "кормушка",
}

// EverywhereRoomType is a special value of room type meaning all devices in a household must be considered
var EverywhereRoomType = "rooms.types.everywhere"

// Known string values for RangeValueSlot
const (
	MinRangeValue = "min"
	MaxRangeValue = "max"
)

var KnownStringRangeValues = map[string]bool{
	MinRangeValue: true,
	MaxRangeValue: true,
}

const UnknownModeValue = "unknown"

type ExtractionStatus string

const (
	OkExtractionStatus                    = "OK"
	DevicesNotFoundExtractionStatus       = "DEVICES_NOT_FOUND"
	MultipleHouseholdsExtractionStatus    = "MULTIPLE_HOUSEHOLDS"
	OnlyDemoRoomsExtractionStatus         = "ONLY_DEMO_ROOMS"
	ShortCommandWithMultiplePossibleRooms = "MULTIPLE_ROOMS_SUITABLE_FOR_SHORT_COMMAND"
	RequestToTandem                       = "REQUEST_TO_TANDEM"
)
