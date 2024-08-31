package mobile

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/inflector"
)

var scenarioVoiceTriggerTypeSuggests []string
var scenarioTextSuggests []string
var scenarioPhraseSuggests []string
var scenarioQuasarMusicSuggests []string
var scenarioQuasarTTSSuggests []string
var scenarioAllowedTriggerTypeSuggests []string

var modeInstanceInflections map[model.ModeCapabilityInstance]inflector.Inflection

type EventID string

var ActiveEventIDs []string

const ColorCapabilityInstance = "color" // magic var to incapsulate HSV or RGB

const (
	GarlandEventID            EventID = "event.garland"
	SwitchToDeviceTypeEventID EventID = "event.device.type.switch_to"
)

// Device types names
const (
	SocketTypeName string = "Розетка"
	SwitchTypeName string = "Выключатель"
	LightTypeName  string = "Освещение"
)

const (
	GroupItemInfoViewType      ItemInfoViewType = "group"
	DeviceItemInfoViewType     ItemInfoViewType = "device"
	StereopairItemInfoViewType ItemInfoViewType = "stereopair"
)

const (
	TimerLaunchScheduleType          LaunchScheduleType = "timer"
	TimetableLaunchScheduleType      LaunchScheduleType = "timetable"
	MultistepDelayLaunchScheduleType LaunchScheduleType = "multistep_delay"
)

const (
	DoneScenarioStepStatus ScenarioStepStatus = "DONE"
)

var deviceTypeNameMap map[model.DeviceType]string

var skillsInnerRatingMap map[string]int
var favoriteTypesSortingMap map[model.FavoriteType]int
var defaultBackgroundImageIDs []model.BackgroundImageID

var speakerNewsTopicsNameMap map[model.SpeakerNewsTopic]string

func init() {
	ActiveEventIDs = []string{string(GarlandEventID), string(SwitchToDeviceTypeEventID)}

	scenarioVoiceTriggerTypeSuggests = []string{"Вечеринка", "Время уборки", "До вечера", "Добрый день", "Добрый вечер", "Доброе утро", "Спокойной ночи", "Я дома", "Я ухожу", "У нас гости"}
	scenarioTextSuggests = []string{"Расскажи новости", "Какая сейчас погода?", "Включи музыку", "Включи радио", "Сколько ехать до работы?", "Есть ли пробки?"}
	scenarioPhraseSuggests = []string{"Добро пожаловать!", "Уборка запущена", "С возвращением!", "Приятного чаепития!"}
	scenarioQuasarMusicSuggests = []string{"The Beatles - Yesterday", "Metallica - Enter Sandman", "David Bowie - Starman"}
	scenarioQuasarTTSSuggests = make([]string, 0, len(scenarioPhraseSuggests))
	scenarioQuasarTTSSuggests = append(scenarioQuasarTTSSuggests, scenarioPhraseSuggests...)
	scenarioAllowedTriggerTypeSuggests = []string{string(model.VoiceScenarioTriggerType)}

	modeInstanceInflections = map[model.ModeCapabilityInstance]inflector.Inflection{
		model.ThermostatModeInstance: {
			Im:   "термостат",
			Rod:  "термостата",
			Dat:  "термостату",
			Vin:  "термостат",
			Tvor: "термостатом",
			Pr:   "термостате",
		},
		model.FanSpeedModeInstance: {
			Im:   "скорость вентиляции",
			Rod:  "скорости вентиляции",
			Dat:  "скорости вентиляции",
			Vin:  "скорость вентиляции",
			Tvor: "скоростью вентиляции",
			Pr:   "скорости вентиляции",
		},
		model.WorkSpeedModeInstance: {
			Im:   "скорость работы",
			Rod:  "скорости работы",
			Dat:  "скорости работы",
			Vin:  "скорость работы",
			Tvor: "скоростью работы",
			Pr:   "скорости работы",
		},
		model.CleanUpModeInstance: {
			Im:   "уборка",
			Rod:  "уборки",
			Dat:  "уборке",
			Vin:  "уборку",
			Tvor: "уборкой",
			Pr:   "уборке",
		},
		model.ProgramModeInstance: {
			Im:   "программа",
			Rod:  "программы",
			Dat:  "программе",
			Vin:  "программу",
			Tvor: "программой",
			Pr:   "программе",
		},
		model.InputSourceModeInstance: {
			Im:   "источник сигнала",
			Rod:  "источника сигнала",
			Dat:  "источнику сигнала",
			Vin:  "источник сигнала",
			Tvor: "источником сигнала",
			Pr:   "источнике сигнала",
		},
		model.CoffeeModeInstance: {
			Im:   "кофе",
			Rod:  "кофе",
			Dat:  "кофе",
			Vin:  "кофе",
			Tvor: "кофе",
			Pr:   "кофе",
		},
		model.SwingModeInstance: {
			Im:   "направление воздуха",
			Rod:  "направления воздуха",
			Dat:  "направлению воздуха",
			Vin:  "направление воздуха",
			Tvor: "направлением воздуха",
			Pr:   "направлении воздуха",
		},
		model.HeatModeInstance: {
			Im:   "нагрев",
			Rod:  "нагрева",
			Dat:  "нагреву",
			Vin:  "нагрев",
			Tvor: "нагревом",
			Pr:   "нагреве",
		},
		model.DishwashingModeInstance: {
			Im:   "мойка посуды",
			Rod:  "мойки посуды",
			Dat:  "мойке посуды",
			Vin:  "мойку посуды",
			Tvor: "мойкой посуды",
			Pr:   "мойке посуды",
		},
		model.TeaModeInstance: {
			Im:   "режим чая",
			Rod:  "режима чая",
			Dat:  "режиму чая",
			Vin:  "режим чая",
			Tvor: "режимом чая",
			Pr:   "режиме чая",
		},
	}

	deviceTypeNameMap = make(map[model.DeviceType]string)
	deviceTypeNameMap[model.SocketDeviceType] = SocketTypeName
	deviceTypeNameMap[model.SwitchDeviceType] = SwitchTypeName
	deviceTypeNameMap[model.LightDeviceType] = LightTypeName

	skillsInnerRatingMap = map[string]int{
		model.XiaomiSkill:     0, // Xiaomi
		model.AqaraSkill:      1, // Aqara
		model.RedmondSkill:    2, // Redmond
		model.SamsungSkill:    3, // Samsung
		model.LGSkill:         4, // LG
		model.NewPhilipsSkill: 5, // Philips (official)
		model.LegrandSkill:    6, // Legrand
		model.RubetekSkill:    7, // Rubetek
		model.ElariSkill:      8, // Elari
		model.DigmaSkill:      9, // Digma
	}

	favoriteTypesSortingMap = map[model.FavoriteType]int{
		model.ScenarioFavoriteType:       0,
		model.DevicePropertyFavoriteType: 1,
		model.DeviceFavoriteType:         2,
		model.GroupFavoriteType:          3,
	}

	defaultBackgroundImageIDs = []model.BackgroundImageID{
		model.Default1BackgroundImageID,
		model.Default2BackgroundImageID,
		model.Default3BackgroundImageID,
		model.Default4BackgroundImageID,
	}

	speakerNewsTopicsNameMap = map[model.SpeakerNewsTopic]string{
		model.PoliticsSpeakerNewsTopic:  "Политика",
		model.SocietySpeakerNewsTopic:   "Общество",
		model.BusinessSpeakerNewsTopic:  "Экономика",
		model.WorldSpeakerNewsTopic:     "В мире",
		model.SportSpeakerNewsTopic:     "Спорт",
		model.IncidentSpeakerNewsTopic:  "Происшествия",
		model.IndexSpeakerNewsTopic:     "Главное",
		model.CultureSpeakerNewsTopic:   "Культура",
		model.ComputersSpeakerNewsTopic: "Технологии",
		model.ScienceSpeakerNewsTopic:   "Наука",
		model.AutoSpeakerNewsTopic:      "Авто",
	}
}
