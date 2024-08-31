package megamind

import "time"

type IrrelevantReason string

const (
	ProcessorNotFound          IrrelevantReason = "ProcessorNotFound"
	MissingIotNluResult        IrrelevantReason = "MissingIotNluResult"
	TandemTVHypotheses         IrrelevantReason = "TandemTVHypotheses"
	MissingHypotheses          IrrelevantReason = "MissingHypotheses"
	MissingCallbackDirective   IrrelevantReason = "MissingCallbackDirective"
	UnknownFrame               IrrelevantReason = "UnknownFrame"
	GranetProcessorIsPreferred IrrelevantReason = "GranetProcessorIsPreferred"
)

const (
	IoTProductScenarioName = "iot_do" // should be snake cased
	IoTScenarioIntent      = "iot"    // main iot intent

	IoTScenariosScenarioIntent = "IoTScenarios"
)

type FrameProcessorName string

const (
	MMApplyTimeout      = time.Millisecond * 4950 // 5000ms is current MM timeout: https://st.yandex-team.ru/ALICEOPS-658
	DefaultApplyTimeout = time.Second
)

// EnableGranetProcessorsExp is a megamind experiment indicating that new granet processors should be used instead of BegemotProcessor
const EnableGranetProcessorsExp = "iot_granet_processors"

var DemoEntityValueToName = map[string]string{
	// devices
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

	// rooms
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
