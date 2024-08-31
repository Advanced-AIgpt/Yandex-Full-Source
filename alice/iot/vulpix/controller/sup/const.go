package sup

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

const (
	IOTSupProjectName         string = "iot"
	voiceDiscoveryErrorPushID string = "iot.voice_discovery.error"

	voiceDiscoveryFailureStatID           string = "iot.voice_discovery.failure"
	voiceDiscoveryWifiIs5GhzStatID        string = "iot.voice_discovery.wifi_5_Ghz"
	voiceDiscoveryNotAllowedSpeakerStatID string = "iot.voice_discovery.not_allowed_speaker"
	voiceDiscoveryNotAllowedClientStatID  string = "iot.voice_discovery.not_allowed_client"
)

const (
	AliceLogoIconID              string = "alice_logo"
	AliceLogoIcon                string = "https://yastatic.net/s3/home/apisearch/alice_icon.png"
	IOTDefaultTitle              string = "Алиса: Умный дом"
	IOTAppDefaultTitle           string = "Дом с Алисой"
	LightDiscoveryLink           string = "https://yandex.ru/quasar/external/device-discovery?deviceType=devices.types.light"
	SocketDiscoveryLink          string = "https://yandex.ru/quasar/external/device-discovery?deviceType=devices.types.socket"
	HubDiscoveryLink             string = "https://yandex.ru/quasar/external/device-discovery?deviceType=devices.types.hub"
	voiceDiscoveryThrottlePolicy string = "iot-voice-discovery"
)

const (
	defaultSecondsTTL uint64 = 3600
)

var discoveryLinks = map[model.DeviceType]string{
	model.LightDeviceType:  LightDiscoveryLink,
	model.SocketDeviceType: SocketDiscoveryLink,
	model.HubDeviceType:    HubDiscoveryLink,
}

var wifiIs5GhzPushTexts = map[model.DeviceType][]string{
	model.LightDeviceType: {
		"Для подключения лампочки нужна сеть 2,4 ГГц. Рассказываю, как ее найти или подключить",
		"Разведка сообщила мне, что ваша сеть не подходит для лампочки. Подробнее о том, как это исправить, здесь",
		"Кажется, ваша сеть не совсем подходит для лампочки, но это не беда. Это легко исправить!",
	},
	model.SocketDeviceType: {
		"Для подключения розетки нужна сеть 2,4 ГГц. Рассказываю, как ее найти или подключить",
		"Разведка сообщила мне, что ваша сеть не подходит для розетки. Подробнее о том, как это исправить, здесь",
		"Кажется, ваша сеть не совсем подходит для розетки, но это не беда. Это легко исправить!",
	},
	model.HubDeviceType: {
		"Для подключения умного пульта нужна сеть 2,4 ГГц. Рассказываю, как ее найти или подключить",
		"Разведка сообщила мне, что ваша сеть не подходит для умного пульта. Подробнее о том, как это исправить, здесь",
		"Кажется, ваша сеть не совсем подходит для умного пульта, но это не беда. Это легко исправить!",
	},
}

var discoveryErrorPushTexts = map[model.DeviceType][]string{
	model.LightDeviceType: {
		"Вы купили лампочку, я ее не нашла, вы находитесь здесь. Вот инструкция о том, как помочь мне найти вашу лампочку",
		"Мы с лампочкой пока не нашли друг друга, помогите нам это сделать - нажмите сюда",
		"Чтобы помочь мне найти вашу лампочку, нажмите здесь и следуйте инструкции",
	},
	model.SocketDeviceType: {
		"Вы купили розетку, я ее не нашла, вы находитесь здесь. Вот инструкция о том, как помочь мне найти вашу розетку",
		"Мы с розеткой пока не нашли друг друга, помогите нам это сделать - нажмите сюда",
		"Чтобы помочь мне найти вашу розетку, нажмите здесь и следуйте инструкции",
	},
	model.HubDeviceType: {
		"Вы купили умный пульт, я его не нашла, вы находитесь здесь. Вот инструкция о том, как помочь мне найти ваш умный пульт",
		"Мы с умным пультом пока не нашли друг друга, помогите нам это сделать - нажмите сюда",
		"Чтобы помочь мне найти ваш умный пульт, нажмите здесь и следуйте инструкции",
	},
}

var discoveryNotAllowedPushTexts = map[model.DeviceType][]string{
	model.LightDeviceType: {
		"Ну что, подключим лампочку во что бы то ни стало? Следуйте за мной",
		"Итак, давайте сделаем это здесь. Следуйте за мной",
		"Хьюстон, нет проблем - сейчас мы подключим лампочку. Поехали!",
	},
	model.SocketDeviceType: {
		"Ну что, подключим розетку во что бы то ни стало? Следуйте за мной",
		"Итак, давайте сделаем это здесь. Следуйте за мной",
		"Хьюстон, нет проблем - сейчас мы подключим розетку. Поехали!",
	},
	model.HubDeviceType: {
		"Ну что, подключим умный пульт во что бы то ни стало? Следуйте за мной",
		"Итак, давайте сделаем это здесь. Следуйте за мной",
		"Хьюстон, нет проблем - сейчас мы подключим умный пульт. Поехали!",
	},
}
