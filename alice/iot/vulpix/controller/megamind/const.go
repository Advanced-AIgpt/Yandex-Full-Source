package megamind

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/nlg"
)

const (
	maximumErrorsCountDuringTuyaPolling int = 3
)

const (
	shortBroadcastTimeoutMs int32 = 30000
	longBroadcastTimeoutMs  int32 = 60000
)

const (
	StartBroadcastDirectiveName string = "start_broadcast"
	StopBroadcastDirectiveName  string = "stop_broadcast"
	OpenURIDirectiveName        string = "open_uri_directive"

	IoTDiscoveryStartDirectiveName       string = "iot_discovery_start"
	IoTDiscoveryStopDirectiveName        string = "iot_discovery_stop"
	IoTDiscoveryCredentialsDirectiveName string = "iot_discovery_credentials"
)

const (
	CancelVoiceDiscoveryCallbackName = "cancel_voice_discovery"
	CancelVoiceDiscoveryFrameSlug    = "cancel_voice_discovery"

	Connect2VoiceDiscoveryCallbackName    = "commit_action"
	Connect2VoiceDiscoveryFrameSlug       = "step2_voice_discovery"
	Connect2VoiceDiscoveryDeviceTypeField = "device_type"
)

type ClientInfoType string

const (
	UnsupportedClientClientInfoType  ClientInfoType = "unsupported_client"
	UnsupportedSpeakerClientInfoType ClientInfoType = "unsupported_speaker"
	SearchAppClientInfoType          ClientInfoType = "search_app"
	IotAppIOSClientInfoType          ClientInfoType = "iot_app_ios"
	IotAppAndroidClientInfoType      ClientInfoType = "iot_app_android"
	StandaloneAliceClientInfoType    ClientInfoType = "standalone_alice"
	SpeakerClientInfoType            ClientInfoType = "speaker"
)

var nlgScenarioConnectFrameRunForDeviceType = map[model.DeviceType]libnlg.NLG{
	model.LightDeviceType: {
		libnlg.NewAssetWithText("Сейчас подключим! Это займет не более 30 секунд."),
		libnlg.NewAssetWithText("Начинаю подключение лампочки. Займет секунд 30."),
		libnlg.NewAssetWithText("Сейчас попробую подключить вашу лампочку. Полминуты и готово."),
	},
	model.SocketDeviceType: {
		libnlg.NewAssetWithText(`Без проблем! Пожалуйста, зажмите кнопку на розетке. Когда индикатор начнет быстро мигать, скажите: "Алиса, готово!"`),
	},
	model.HubDeviceType: {
		libnlg.NewAssetWithText(`С удовольствием! Пожалуйста, зажмите кнопку на пульте. Когда индикатор начнет быстро мигать, скажите: "Алиса, готово!"`),
	},
}

var nlgScenarioConnect2FrameRun = map[model.DeviceType]libnlg.NLG{
	model.LightDeviceType: {
		libnlg.NewAssetWithText("Отлично, начинаю поиск лампочки. Это займет не больше минуты."),
	},
	model.SocketDeviceType: {
		libnlg.NewAssetWithText("Отлично, ищу розетку. Это займет не больше минуты."),
	},
	model.HubDeviceType: {
		libnlg.NewAssetWithText("Отлично, подключаю пульт. Это займет не больше минуты."),
	},
}

var nlgScenarioConnect2Frame = libnlg.NLG{
	libnlg.NewAssetWithText(`Почему-то не вижу лампочку. Пожалуйста, выключите и включите лампочку пять раз подряд, каждый раз оставляя её включенной на две секунды. Когда лампочка начнет мигать, скажите: "Алиса, готово!"`),
}

var nlgUnsupportedSpeaker = libnlg.NLG{
	libnlg.NewAssetWithText(`Я бы рада, но в этой модели колонки так подключить не получится. Давайте продолжим в приложении Дом с Алисой!`),
	libnlg.NewAssetWithText(`Увы, но на этой колонке устройство придется подключать иначе. Давайте продолжим в приложении Дом с Алисой!`),
}

var nlgUnsupportedClient = libnlg.NLG{
	libnlg.NewAssetWithText(`Я бы рада, но здесь так подключить не получится. Давайте продолжим в приложении Дом с Алисой или умной колонке!`),
	libnlg.NewAssetWithText("С умным домом я справлюсь лучше на телефоне или умной колонке."),
}

var nlgNeedLogin = libnlg.NLG{
	libnlg.NewAssetWithText("Чтобы подключить умное устройство, вам нужно войти в свой Яндекс аккаунт."),
}

var nlgSearchApp = libnlg.NLG{
	libnlg.NewAssetWithText("Хорошо."),
}

var nlgIrrelevant = libnlg.NLG{
	libnlg.NewAssetWithText("К сожалению, я ничего не нашла."),
}

var nlgCannotDo = libnlg.NLG{
	libnlg.NewAssetWithText("Я такому пока не научилась."),
}

var nlgCommonError = libnlg.NLG{
	libnlg.NewAssetWithText("К сожалению, что-то пошло не так."),
}

var nlgScenarioBroadcastSuccessYandexForDeviceType = map[model.DeviceType]libnlg.NLG{
	model.LightDeviceType: {
		libnlg.NewAssetWithText("Вот и всё. Теперь вы живете в новом свете! Если хотите включить синий или зеленый, так мне и скажите."),
		libnlg.NewAssetWithText(`Ура! Лампочка подключена. Теперь вы можете управлять светом - голосом. Достаточно сказать: "Смени свет на розовый", и будет как в сказке.`),
		libnlg.NewAssetWithText("Замечательно. Надеюсь, вам станет значительно светлее. А если попр+осите включить теплый белый, то еще и теплее."),
	},
	model.HubDeviceType: {
		libnlg.NewAssetWithText("Ну вот. Пульт подключен. Теперь зайдите в приложение Яндекса, чтобы добавить устройства, которыми хотите управлять с помощью пульта. Жизнь становится комфортнее - правда же?"),
	},
	model.SocketDeviceType: {
		libnlg.NewAssetWithText("Ура! Я все нашла. Теперь ваша розетка работает. Включите в нее что-нибудь. Например, лампу. А потом скажите мне: Алиса, включи розетку. Магия - свет горит!."),
	},
}

var nlgScenarioBroadcastSuccessOtherDevices = map[model.DeviceType]libnlg.NLG{
	model.LightDeviceType: {
		libnlg.NewAssetWithText("Я что-то нашла, но, кажется, это не лампочка Яндекса, а что-то другое полезное. Но я не против! Пользуйтесь на здоровье"),
	},
	model.SocketDeviceType: {
		libnlg.NewAssetWithText("Я что-то нашла, но, кажется, это не розетка Яндекса, а что-то другое полезное. Но я не против! Пользуйтесь на здоровье"),
	},
	model.HubDeviceType: {
		libnlg.NewAssetWithText("Я что-то нашла, но, кажется, это не умный пульт Яндекса, а что-то другое полезное. Но я не против! Пользуйтесь на здоровье"),
	},
}

var nlgScenarioBroadcastFailureApply = libnlg.NLG{
	libnlg.NewAssetWithText("Так, у нас с устройством не вышло соединиться, пожалуйста, попробуйте еще раз. Давайте продолжим в приложении Яндекса, там будет проще с этим разобраться. "),
}

var nlgScenarioWifi5GHz = libnlg.NLG{
	libnlg.NewAssetWithText("Ох, слушайте, сеть, к которой подключена колонка, не подходит для подключения устройства. Давайте продолжим в приложении Яндекса, там будет проще с этим разобраться."),
}

var nlgDiscoveryCancel = libnlg.NLG{
	libnlg.NewAssetWithText("Хорошо, перестала."),
}

var nlgHowToAllowedSpeakerForDeviceType = map[model.DeviceType]libnlg.NLG{
	model.LightDeviceType: {
		libnlg.NewAssetWithText(`Давайте сделаем это вместе! Итак, выключите и включите лампочку пять раз подряд, каждый раз оставляя её включенной на две секунды. Когда лампочка начнет мигать, скажите: "Алиса, подключи лампочку Яндекса".`),
		libnlg.NewAssetWithText(`Ничего сложного в этом нет! Итак, выключите и включите лампочку пять раз подряд, каждый раз оставляя её включенной на две секунды. Когда лампочка начнет мигать, скажите: "Алиса, подключи лампочку Яндекса".`),
		libnlg.NewAssetWithText(`Сейчас подскажу - это очень просто! Итак, выключите и включите лампочку пять раз подряд, каждый раз оставляя её включенной на две секунды. Когда лампочка начнет мигать, скажите: "Алиса, подключи лампочку Яндекса".`),
	},
	model.HubDeviceType: {
		libnlg.NewAssetWithText(`Давайте сделаем это вместе! Просто скажите: "Алиса, подключи умный пульт Яндекса".`),
	},
	model.SocketDeviceType: {
		libnlg.NewAssetWithText(`Давайте сделаем это вместе! Просто скажите: "Алиса, подключи розетку Яндекса".`),
	},
}

var nlgHowToSearchAppForDeviceType = map[model.DeviceType]libnlg.NLG{
	model.LightDeviceType: {
		libnlg.NewAssetWithText(`Вы можете легко сделать это голосом - достаточно сказать мне: "Алиса, подключи лампочку".`),
	},
	model.HubDeviceType: {
		libnlg.NewAssetWithText(`Вы можете легко сделать это голосом - достаточно сказать мне: "Алиса, подключи умный пульт".`),
	},
	model.SocketDeviceType: {
		libnlg.NewAssetWithText(`Вы можете легко сделать это голосом - достаточно сказать мне: "Алиса, подключи розетку".`),
	},
}

var nlgHowToNotAllowedSpeakerForDeviceType = map[model.DeviceType]libnlg.NLG{
	model.LightDeviceType: {
		libnlg.NewAssetWithText(`Я могу вам помочь, но только в приложении Яндекса. Попросите меня там: "Алиса, подключи лампочку" и я все сделаю.`),
	},
	model.HubDeviceType: {
		libnlg.NewAssetWithText(`Давайте сделаем это вместе! Удерживайте кнопку на пульте. Когда индикатор на пульте начнет быстро мигать, скажите: "Алиса, подключи умный пульт Яндекса".`),
	},
	model.SocketDeviceType: {
		libnlg.NewAssetWithText(`Давайте сделаем это вместе! Удерживайте кнопку на розетке. Когда индикатор на ней начнет быстро мигать, скажите: "Алиса, подключи розетку Яндекса".`),
	},
}
