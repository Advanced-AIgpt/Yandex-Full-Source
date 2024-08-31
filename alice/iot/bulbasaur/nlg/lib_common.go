package nlg

import (
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
)

var OK = libnlg.NLG{
	libnlg.NewAssetWithText("Окей."),
	libnlg.NewAssetWithText("Хорошо."),
	libnlg.NewAssetWithText("Есть."),
	libnlg.NewAssetWithText("Конечно."),
	libnlg.NewAssetWithText("Пожалуйста."),
	libnlg.NewAssetWithText("С удовольствием."),
	libnlg.NewAssetWithText("Без проблем."),
	libnlg.NewAssetWithText("Легко."),
}

var TurnOnSound = libnlg.NLG{
	libnlg.NewAsset("Окей.", `<speaker audio="alice-iot-sounds-turn-on.opus">`),
	libnlg.NewAsset("Хорошо.", `<speaker audio="alice-iot-sounds-turn-on.opus">`),
	libnlg.NewAsset("Есть.", `<speaker audio="alice-iot-sounds-turn-on.opus">`),
	libnlg.NewAsset("Конечно.", `<speaker audio="alice-iot-sounds-turn-on.opus">`),
	libnlg.NewAsset("Пожалуйста.", `<speaker audio="alice-iot-sounds-turn-on.opus">`),
	libnlg.NewAsset("С удовольствием.", `<speaker audio="alice-iot-sounds-turn-on.opus">`),
	libnlg.NewAsset("Без проблем.", `<speaker audio="alice-iot-sounds-turn-on.opus">`),
	libnlg.NewAsset("Легко.", `<speaker audio="alice-iot-sounds-turn-on.opus">`),
}

var TurnOffSound = libnlg.NLG{
	libnlg.NewAsset("Окей.", `<speaker audio="alice-iot-sounds-turn-off.opus">`),
	libnlg.NewAsset("Хорошо.", `<speaker audio="alice-iot-sounds-turn-off.opus">`),
	libnlg.NewAsset("Есть.", `<speaker audio="alice-iot-sounds-turn-off.opus">`),
	libnlg.NewAsset("Конечно.", `<speaker audio="alice-iot-sounds-turn-off.opus">`),
	libnlg.NewAsset("Пожалуйста.", `<speaker audio="alice-iot-sounds-turn-off.opus">`),
	libnlg.NewAsset("С удовольствием.", `<speaker audio="alice-iot-sounds-turn-off.opus">`),
	libnlg.NewAsset("Без проблем.", `<speaker audio="alice-iot-sounds-turn-off.opus">`),
	libnlg.NewAsset("Легко.", `<speaker audio="alice-iot-sounds-turn-off.opus">`),
}

var OnOffUnknownSound = libnlg.NLG{
	libnlg.NewAsset("Окей.", `<speaker audio="alice-iot-sounds-on-off-unknown-state.opus">`),
	libnlg.NewAsset("Хорошо.", `<speaker audio="alice-iot-sounds-on-off-unknown-state.opus">`),
	libnlg.NewAsset("Есть.", `<speaker audio="alice-iot-sounds-on-off-unknown-state.opus">`),
	libnlg.NewAsset("Конечно.", `<speaker audio="alice-iot-sounds-on-off-unknown-state.opus">`),
	libnlg.NewAsset("Пожалуйста.", `<speaker audio="alice-iot-sounds-on-off-unknown-state.opus">`),
	libnlg.NewAsset("С удовольствием.", `<speaker audio="alice-iot-sounds-on-off-unknown-state.opus">`),
	libnlg.NewAsset("Без проблем.", `<speaker audio="alice-iot-sounds-on-off-unknown-state.opus">`),
	libnlg.NewAsset("Легко.", `<speaker audio="alice-iot-sounds-on-off-unknown-state.opus">`),
}

var ScenarioRun = libnlg.NLG{
	libnlg.NewAssetWithText("Окей, поехали."),
}

var ScenarioRunSound = libnlg.NLG{
	libnlg.NewAsset("Окей, поехали.", `<speaker audio="alice-iot-sounds-scenario.opus">`),
}

var OptimisticOK = libnlg.NLG{
	libnlg.NewAssetWithText("Окей."),
	libnlg.NewAssetWithText("Секундочку."),
	libnlg.NewAssetWithText("Сейчас попробуем."),
}

var ScenarioCreateForSearchApp = libnlg.NLG{
	libnlg.NewAssetWithText("Отлично! Сделаем это вместе"),
	libnlg.NewAssetWithText("Давайте!"),
	libnlg.NewAssetWithText("Отличная мысль, поддерживаю"),
	libnlg.NewAssetWithText("Конечно. Сделаем наш мир лучше"),
	libnlg.NewAssetWithText("Конечно. Помогу вам"),
	libnlg.NewAssetWithText("Ок, сделаем это!"),
}

var ScenarioCreateForSpeaker = libnlg.NLG{
	libnlg.NewAssetWithText("Хорошо, давайте продолжим в приложении Дом с Алисой"),
	libnlg.NewAssetWithText("Отличная мысль! Продолжим в приложении Дом с Алисой"),
	libnlg.NewAssetWithText("Хорошо, я помогу, но в приложении Дом с Алисой"),
	libnlg.NewAssetWithText("Давайте сделаем это вместе в приложении Дом с Алисой"),
}

var DingForSpeaker = libnlg.NLG{
	libnlg.NewAsset("", `<speaker audio="notification.opus">`),
}
