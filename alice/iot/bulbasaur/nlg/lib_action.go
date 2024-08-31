package nlg

import (
	"a.yandex-team.ru/alice/library/go/nlg"
)

var TurnOn = libnlg.NLG{
	libnlg.NewAssetWithText("Включаю."),
}

var TurnOff = libnlg.NLG{
	libnlg.NewAssetWithText("Выключаю."),
	libnlg.NewAssetWithText("Окей, выключаем."),
	libnlg.NewAssetWithText("Окей, выключаю."),
}

var Open = libnlg.NLG{
	libnlg.NewAssetWithText("Открываю."),
}

var Close = libnlg.NLG{
	libnlg.NewAssetWithText("Закрываю."),
	libnlg.NewAssetWithText("Окей, закрываем."),
	libnlg.NewAssetWithText("Окей, закрываю."),
}

var Boil = libnlg.NLG{
	libnlg.NewAssetWithText("Окей, кипячу чайник."),
	libnlg.NewAssetWithText("Хорошо, ставлю чайник."),
}

var VacuumOff = libnlg.NLG{
	libnlg.NewAssetWithText("Возвращаю."),
}

var Brighten = libnlg.NLG{
	libnlg.NewAssetWithText("Окей, сделаем поярче."),
	libnlg.NewAssetWithText("Окей, делаем ярче."),
	libnlg.NewAssetWithText("Окей, добавим яркости."),
	libnlg.NewAssetWithText("Добавляю яркости."),
	libnlg.NewAssetWithText("Больше яркости. Окей."),
}

var MaxBrightness = libnlg.NLG{
	libnlg.NewAssetWithText("Окей, яркость на максимум."),
	libnlg.NewAssetWithText("Хорошо, яркость на максимум."),
	libnlg.NewAssetWithText("Хорошо. Максимум яркости."),
	libnlg.NewAssetWithText("Как скажете. Максимальная яркость."),
}

var Dim = libnlg.NLG{
	libnlg.NewAssetWithText("Окей, сделаем темнее."),
	libnlg.NewAssetWithText("Окей, делаем темнее."),
	libnlg.NewAssetWithText("Окей, убавим яркость."),
	libnlg.NewAssetWithText("Убавляю яркость."),
	libnlg.NewAssetWithText("Меньше яркости. Окей."),
}

var MaxDim = libnlg.NLG{
	libnlg.NewAssetWithText("Хорошо. Минимум яркости."),
	libnlg.NewAssetWithText("Как скажете. Минимальная яркость."),
}

var ChangeBrightness = libnlg.NLG{
	libnlg.NewAssetWithText("Меняю яркость."),
}

var ChangeColor = libnlg.NLG{
	libnlg.NewAssetWithText("Меняю цвет."),
}

var DecreaseTemperatureK = libnlg.NLG{
	libnlg.NewAssetWithText("Включаю свет потеплее."),
	libnlg.NewAssetWithText("Окей, больше тёплого света."),
	libnlg.NewAssetWithText("Окей, больше тёплого."),
}

var IncreaseTemperatureK = libnlg.NLG{
	libnlg.NewAssetWithText("Включаю свет похолоднее."),
	libnlg.NewAssetWithText("Окей, больше холодного света."),
	libnlg.NewAssetWithText("Окей, больше холодного."),
}

var ChangeTemperature = libnlg.NLG{
	libnlg.NewAssetWithText("Делаю."),
	libnlg.NewAssetWithText("Меняю температуру."),
}

var IncreaseTemperature = libnlg.NLG{
	libnlg.NewAssetWithText("Окей, теплее."),
	libnlg.NewAssetWithText("Окей, сделаем теплее."),
}

var DecreaseTemperature = libnlg.NLG{
	libnlg.NewAssetWithText("Окей, прохладнее."),
	libnlg.NewAssetWithText("Окей, сделаем холоднее."),
}

var MaxTemperature = libnlg.NLG{
	libnlg.NewAssetWithText("Ставлю самую высокую температуру."),
	libnlg.NewAssetWithText("Ставлю температуру на максимум."),
}

var MinTemperature = libnlg.NLG{
	libnlg.NewAssetWithText("Ставлю самую низкую температуру."),
	libnlg.NewAssetWithText("Ставлю температуру на минимум."),
}

var IncreaseHumidity = libnlg.NLG{
	libnlg.NewAssetWithText("Окей, прибавим влажность."),
	libnlg.NewAssetWithText("Прибавляю влажность."),
	libnlg.NewAssetWithText("Больше влажности. Окей."),
}

var DecreaseHumidity = libnlg.NLG{
	libnlg.NewAssetWithText("Окей, убавим влажность."),
	libnlg.NewAssetWithText("Убавляю влажность."),
	libnlg.NewAssetWithText("Меньше влажности. Окей."),
}

var MaxHumidity = libnlg.NLG{
	libnlg.NewAssetWithText("Окей, влажность на максимум."),
	libnlg.NewAssetWithText("Хорошо, влажность на максимум."),
	libnlg.NewAssetWithText("Хорошо. Максимум влажности."),
	libnlg.NewAssetWithText("Как скажете. Максимальная влажность."),
}

var MinHumidity = libnlg.NLG{
	libnlg.NewAssetWithText("Окей, влажность на минимум."),
	libnlg.NewAssetWithText("Хорошо, влажность на минимум."),
	libnlg.NewAssetWithText("Хорошо. Минимум влажности."),
	libnlg.NewAssetWithText("Как скажете. Минимальная влажность."),
}

var ChangeHumidity = libnlg.NLG{
	libnlg.NewAssetWithText("Делаю."),
	libnlg.NewAssetWithText("Меняю влажность."),
}

var StopCleaning = libnlg.NLG{
	libnlg.NewAssetWithText("Заканчиваю уборку."),
}

var IncreaseVolume = libnlg.NLG{
	libnlg.NewAsset("Делаю громче.", ""),
	libnlg.NewAsset("Делаю погромче.", ""),
	libnlg.NewAsset("Окей, делаю погромче.", ""),
}

var DecreaseVolume = libnlg.NLG{
	libnlg.NewAsset("Делаю тише.", ""),
	libnlg.NewAsset("Делаю потише.", ""),
	libnlg.NewAsset("Окей, делаю потише.", ""),
	libnlg.NewAsset("Окей, убавляю громкость.", ""),
}

var ChangeVolume = libnlg.NLG{
	libnlg.NewAsset("Меняю громкость.", ""),
}

var MaxVolume = libnlg.NLG{
	libnlg.NewAsset("Окей, громкость на максимум.", ""),
	libnlg.NewAsset("Хорошо, громкость на максимум.", ""),
	libnlg.NewAsset("Хорошо. Максимум громкости.", ""),
	libnlg.NewAsset("Как скажете. Максимальная громкость.", ""),
	libnlg.NewAsset("Хорошо. Максимум громкости.", ""),
	libnlg.NewAsset("Окей, максимум громкости.", ""),
}

var MinVolume = libnlg.NLG{
	libnlg.NewAsset("Окей, минимальная громкость.", ""),
	libnlg.NewAsset("Окей, громкость — минимальная.", ""),
	libnlg.NewAsset("Окей, минимум звука.", ""),
}

var IncreaseChannel = libnlg.NLG{
	libnlg.NewAsset("Включаю следующий канал.", ""),
	libnlg.NewAsset("Включаю следующий.", ""),
	libnlg.NewAsset("Переключаю.", ""),
	libnlg.NewAsset("Окей, следующий канал.", ""),
	libnlg.NewAsset("Окей, переключаю.", ""),
}

var DecreaseChannel = libnlg.NLG{
	libnlg.NewAsset("Включаю предыдущий канал.", ""),
	libnlg.NewAsset("Включаю предыдущий.", ""),
	libnlg.NewAsset("Переключаю назад.", ""),
	libnlg.NewAsset("Возвращаюсь на предыдущий канал.", ""),
	libnlg.NewAsset("Окей, предыдущий канал.", ""),
	libnlg.NewAsset("Окей, переключаю назад.", ""),
}

var ChangeChannel = libnlg.NLG{
	libnlg.NewAsset("Переключаю.", ""),
}

var PressMuteButton = libnlg.NLG{
	libnlg.NewAsset(`Нажимаю кнопку "Mute".`, ""),
}

var NextMode = libnlg.NLG{
	libnlg.NewAssetWithText("Включаю следующий режим."),
	libnlg.NewAssetWithText("Окей. Следующий режим."),
	libnlg.NewAssetWithText("Как скажете. Следующий режим."),
	libnlg.NewAssetWithText("Следующий режим, поехали."),
}

var PreviousMode = libnlg.NLG{
	libnlg.NewAssetWithText("Включаю предыдущий режим."),
	libnlg.NewAssetWithText("Окей. Предыдущий режим."),
	libnlg.NewAssetWithText("Как скажете. Предыдущий режим."),
	libnlg.NewAssetWithText("Предыдущий режим, поехали."),
}

var NextWorkingMode = libnlg.NLG{
	libnlg.NewAssetWithText("Включаю следующий режим работы."),
	libnlg.NewAssetWithText("Окей. Следующий режим работы."),
	libnlg.NewAssetWithText("Как скажете. Следующий режим работы."),
	libnlg.NewAssetWithText("Следующий режим работы, поехали."),
}

var PreviousWorkingMode = libnlg.NLG{
	libnlg.NewAssetWithText("Включаю предыдущий режим работы."),
	libnlg.NewAssetWithText("Окей. Предыдущий режим работы."),
	libnlg.NewAssetWithText("Как скажете. Предыдущий режим работы."),
	libnlg.NewAssetWithText("Предыдущий режим работы, поехали."),
}

var SwitchToHeatMode = libnlg.NLG{
	libnlg.NewAssetWithText("Включаю режим обогрева."),
	libnlg.NewAssetWithText("Окей, обогрев."),
	libnlg.NewAssetWithText("Окей, сейчас будет теплее."),
	libnlg.NewAssetWithText("Окей. Режим обогрева."),
}

var SwitchToCoolMode = libnlg.NLG{
	libnlg.NewAssetWithText("Включаю режим охлаждения."),
	libnlg.NewAssetWithText("Окей, охлаждение."),
	libnlg.NewAssetWithText("Окей, сейчас будет прохладно."),
	libnlg.NewAssetWithText("Окей. Режим охлаждения."),
}

//911
