package discovery

import (
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
)

// start discovery stage nlg
var (
	startSearchNLG       = libnlg.FromVariant("Начинаю поиск устройств.")
	unsupportedClientNLG = libnlg.NLG{
		libnlg.NewAssetWithText(`Похоже, что с этого устройства у меня не получится ничего найти. Давайте продолжим в приложении Дом с Алисой!`),
		libnlg.NewAssetWithText(`На этом устройстве такое сделать не выйдет. Давайте продолжим в приложении Дом с Алисой!`),
	}
	discoveryUnavailableNLG = libnlg.FromVariant(`Я бы рада, но здесь так подключить не получится. Давайте продолжим в приложении Дом с Алисой!`)
	startSearchErrorNLG     = libnlg.FromVariant("Не могу начать поиск - что-то идет не так. Но я скоро исправлюсь - просто попробуйте чуть позже.")
	okNLG                   = libnlg.FromVariant("Хорошо.")
)

// finish discovery stage nlg
var (
	finishDiscoveryNoResultsNLG          = libnlg.FromVariant("Что-то я ничего не нашла. Убедитесь, что ваши устройства находятся в режиме подключения и попробуйте поискать ещё раз.")
	finishDiscoveryUnsupportedDevicesNLG = libnlg.FromVariant("Я что-то нашла, но подключить не смогла - еще не научилась работать с этими устройствами. Но обязательно постараюсь научиться!")
	finishDiscoverySingleResultNLG       = libnlg.FromVariant("Прекрасные новости - нашла устройство. Пользуйтесь.")
	finishDiscoveryMultipleResultNLG     = libnlg.FromVariant("Прекрасные новости - нашла сразу несколько устройств. Теперь можно настраивать сценарии в приложении Дом с Алисой, попробуйте!")

	// different device types result nlg
	finishDiscoveryTemperatureSensorNLG = libnlg.FromVariant("Прекрасные новости - нашла устройство. Теперь можно спрашивать какая температура дома, а также настраивать сценарии в приложении Дом с Алисой, попробуйте!")
	finishDiscoveryButtonSensorNLG      = libnlg.FromVariant("Прекрасные новости - нашла устройство. Теперь можно настраивать сценарии в приложении Дом с Алисой. Например, станция сыграет мелодию или выключит свет в определенное время, попробуйте!")
	finishDiscoverySensorNLG            = libnlg.FromVariant("Прекрасные новости - нашла устройство. Теперь можно настраивать сценарии в приложении Дом с Алисой, попробуйте!")
)

// how to discovery nlg
var (
	howToDiscoveryNLG = libnlg.FromVariant(`Ничего сложного в этом нет! Переведите ваше устройство в режим подключения. Когда будете готовы, скажите: "Алиса, найди устройство".`)
)
