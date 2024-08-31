package nlg

import (
	"fmt"

	"a.yandex-team.ru/alice/library/go/nlg"
)

var CommonError = libnlg.NLG{
	libnlg.NewAssetWithText("Не справилась. Что-то не так."),
	libnlg.NewAssetWithText("Не вышло. Что-то не так."),
	libnlg.NewAssetWithText("Не получилось. Давайте ещё раз."),
	libnlg.NewAssetWithText("Не справилась."),
	libnlg.NewAssetWithText("Не вышло."),
	libnlg.NewAssetWithText("Не получилось."),
	libnlg.NewAssetWithText("Не выходит, извините."),
}

var CommonErrorHumming = libnlg.NLG{
	libnlg.NewAsset(
		"Не справилась. Что-то не так.",
		"<speaker audio=\"shitova_emotion_142.opus\"> Не справилась. Что-то не так.",
	),
	libnlg.NewAsset(
		"Не вышло. Что-то не так.",
		"<speaker audio=\"shitova_emotion_142.opus\"> Не вышло. Что-то не так.",
	),
	libnlg.NewAsset(
		"Не получилось. Давайте ещё раз.",
		"<speaker audio=\"shitova_emotion_142.opus\"> Не получилось. Давайте ещё раз.",
	),
	libnlg.NewAsset(
		"Не справилась.",
		"<speaker audio=\"shitova_emotion_142.opus\"> Не справилась.",
	),
	libnlg.NewAsset(
		"Не вышло.",
		"<speaker audio=\"shitova_emotion_142.opus\"> Не вышло.",
	),
	libnlg.NewAsset(
		"Не получилось.",
		"<speaker audio=\"shitova_emotion_142.opus\"> Не получилось.",
	),
	libnlg.NewAsset(
		"Не выходит, извините.",
		"<speaker audio=\"shitova_emotion_142.opus\"> Не выходит, извините.",
	),
}

var NoMatchesError = libnlg.NLG{
	libnlg.NewAssetWithText("Совпадений для умного дома не найдено."),
}

var AllActionsFailedError = libnlg.NLG{
	libnlg.NewAssetWithText("Простите, но, кажется, у меня не получилось. Посмотрите, что там стряслось."),
	libnlg.NewAssetWithText("Ой, кажется, что-то пошло не так. Проверьте, все ли в порядке с устройствами?"),
	libnlg.NewAssetWithText("Это печально, но у меня не получилось сделать это. Может быть, что-то сломалось?"),
	libnlg.NewAssetWithText("Не хочу вас расстраивать, но, похоже, что-то сломалось. Проверьте."),
}

var QueryTargetNotFound = libnlg.NLG{
	libnlg.NewAssetWithText("Странно - устройства не признаются. Проверьте, все ли с ними в порядке?"),
}

var QueryCannotDo = libnlg.NLG{
	libnlg.NewAssetWithText("Такого я пока узнать не могу."),
	libnlg.NewAssetWithText("С таким вопросом я не справлюсь."),
}

var QueryStateTooManyDevices = libnlg.NLG{
	libnlg.NewAssetWithText("Состояние какого устройства вы хотите узнать? Спросите конкретнее."),
}

var QueryTooManyModes = libnlg.NLG{
	libnlg.NewAssetWithText("У вашего устройства слишком много режимов. Уточните ваш запрос."),
}

var MultipleQueryTooManyModes = libnlg.NLG{
	libnlg.NewAssetWithText("У ваших устройств слишком много режимов. Уточните ваш запрос."),
}

var QueryMixedState = libnlg.NLG{
	libnlg.NewAssetWithText("Кажется, ваши устройства находятся в разных состояниях. Уточните ваш запрос."),
}

var QueryUnknownState = libnlg.NLG{
	libnlg.NewAssetWithText("Ой, про это устройство ничего сказать не могу."),
}

var MultipleQueryUnknownState = libnlg.NLG{
	libnlg.NewAssetWithText("Ой, про эти устройства ничего сказать не могу."),
}

var QueryMixedError = libnlg.NLG{
	libnlg.NewAsset(
		"Не смогла опросить ваши устройства - кажется, они все бунтуют. Посмотрите, что там не так?",
		"<speaker audio=\"shitova_emotion_142.opus\"> Не смогла опросить ваши устройства - кажется, они все бунтуют. Посмотрите, что там не так?",
	),
}

var QueryTimeout = libnlg.NLG{
	libnlg.NewAssetWithText("Что-то ваши устройства долго не отвечают. Попробуйте посмотреть в приложении Дом с Алисой."),
}

var CannotDo = libnlg.NLG{
	libnlg.NewAssetWithText("Такого я пока не умею."),
	libnlg.NewAssetWithText("Этого я не умею"),
	libnlg.NewAssetWithText("С этим я не справлюсь."),
	libnlg.NewAssetWithText("Миссия невыполнима."),
	libnlg.NewAssetWithText("Такому я ещё не научилась."),
}

var InappropriateRoom = libnlg.NLG{
	libnlg.NewAssetWithText("Ой, простите, а вы не ошиблись с комнатой? Не нахожу тут такого"),
	libnlg.NewAssetWithText("Простите, но в этой комнате такого нет"),
}

var InappropriateHousehold = libnlg.NLG{
	libnlg.NewAssetWithText("Ой, простите, а вы не ошиблись с домом? Не нахожу тут такого"),
	libnlg.NewAssetWithText("Простите, но в этом доме такого нет"),
}

var InappropriateGroup = libnlg.NLG{
	libnlg.NewAssetWithText("В данной группе нет таких устройств."),
}

var UnsupportedClient = libnlg.NLG{
	libnlg.NewAssetWithText("С умным домом я справлюсь лучше на телефоне или умной колонке."),
}

var SelfDrivingCarsNotSupported = libnlg.NLG{
	libnlg.NewAssetWithText("Здесь я пока этого не умею."),
}

var NeedLogin = libnlg.NLG{
	libnlg.NewAssetWithText("Чтобы управлять Умным домом, вам нужно войти в свой Яндекс аккаунт."),
}

var NeedDevices = libnlg.NLG{
	libnlg.NewAssetWithText("Я и такое умею — но здесь нужны устройства для Умного дома Яндекса."),
	libnlg.NewAssetWithText("Я бы с радостью — но здесь нужны специальные устройства для умного дома."),
}

var CheckSettings = libnlg.NLG{
	libnlg.NewAssetWithText("Не могу выполнить запрос. Проверьте настройки умного дома."),
}

func CannotFindDevice(deviceName string) libnlg.NLG {
	return libnlg.NLG{
		libnlg.NewAssetWithText(fmt.Sprintf("Не нашла устройство %q. Проверьте настройки умного дома.", deviceName)),
	}
}

var CannotFindDevices = libnlg.NLG{
	libnlg.NewAssetWithText("Не нашла подходящих устройств. Проверьте настройки умного дома."),
}

var CannotFindRoom = libnlg.NLG{
	libnlg.NewAssetWithText("Не нашла запрошенную комнату. Проверьте настройки умного дома."),
}

func CannotFindRequestedRoom(roomName string) libnlg.NLG {
	return libnlg.NLG{
		libnlg.NewAssetWithText(fmt.Sprintf("Не нашла комнату %q. Проверьте настройки умного дома.", roomName)),
	}
}

var CannotFindRooms = libnlg.NLG{
	libnlg.NewAssetWithText("Не нашла подходящих комнат. Проверьте настройки умного дома."),
}

var CannotFindLightDevices = libnlg.NLG{
	libnlg.NewAssetWithText("Не нашла устройств с типом \"освещение\". Проверьте настройки умного дома."),
}

func CannotFindRequestedDeviceType(deviceTypeName string) libnlg.NLG {
	return libnlg.NLG{
		libnlg.NewAssetWithText(fmt.Sprintf("Не нашла устройств с типом %q. Проверьте настройки умного дома.", deviceTypeName)),
	}
}

// singular error codes
var (
	DoorOpenError = libnlg.NLG{
		libnlg.NewAssetWithText("Ой, кажется, открыта дверца. Закройте её и повторите команду."),
	}

	LidOpenError = libnlg.NLG{
		libnlg.NewAssetWithText("Упс, кажется, вы забыли закрыть крышку. Закройте её и повторите команду."),
	}

	RemoteControlDisabledError = libnlg.NLG{
		libnlg.NewAssetWithText("Сначала нужно спросить разрешения у самого устройства: проверьте, на нём должна быть специальная кнопка."),
	}

	NotEnoughWaterError = libnlg.NLG{
		libnlg.NewAssetWithText("Ой, недостаточно воды. Долейте её и повторите команду."),
	}

	LowChargeLevelError = libnlg.NLG{
		libnlg.NewAssetWithText("Кажется, устройство разрядилось. Пожалуйста, повторите после зарядки."),
	}

	ContainerFullError = libnlg.NLG{
		libnlg.NewAssetWithText("Кажется, контейнер переполнился. Очистите его и повторите команду."),
	}

	ContainerEmptyError = libnlg.NLG{
		libnlg.NewAssetWithText("Ой, кажется, вы забыли что-то положить - контейнер пуст. Наполните его и повторите команду."),
	}

	DripTrayFullError = libnlg.NLG{
		libnlg.NewAssetWithText("Заполнился сливной поддон. Очистите его и повторите команду."),
	}

	DeviceStuckError = libnlg.NLG{
		libnlg.NewAssetWithText("Аларм, возникло препятствие! Пожалуйста, устраните его."),
	}

	DeviceOffError = libnlg.NLG{
		libnlg.NewAssetWithText("Ой, не получается. Сначала включите устройство."),
	}

	FirmwareOutOfDateError = libnlg.NLG{
		libnlg.NewAssetWithText("Не могу запустить без обновления. Пожалуйста, обновите прошивку вашему устройству (речь не обо мне)."),
	}

	NotEnoughDetergentError = libnlg.NLG{
		libnlg.NewAssetWithText("Я бы рада, да закончилось моющее средство. Добавьте его и повторите команду."),
	}

	AccountLinkingError = libnlg.NLG{
		libnlg.NewAssetWithText("Устройство отвязалось, необходимо снова связать аккаунты в приложении Дом с Алисой."),
	}

	HumanInvolvementNeededError = libnlg.NLG{
		libnlg.NewAssetWithText("Кажется, что-то пошло не так. Для устранения ошибки нужна ваша помощь."),
	}

	DeviceUnreachable = libnlg.NLG{
		libnlg.NewAssetWithText("Устройство недоступно, проверьте, пожалуйста, интернет или подключено ли оно к сети."),
	}

	DeviceBusy = libnlg.NLG{
		libnlg.NewAssetWithText("Прямо сейчас устройство занято. Попробуйте позднее."),
	}

	DeviceNotFound = libnlg.NLG{
		libnlg.NewAssetWithText("Не могу найти такое устройство."),
	}

	InternalError = libnlg.NLG{
		libnlg.NewAssetWithText("Что-то пошло не так. Подождите немного и попробуйте снова."),
	}

	InvalidAction = libnlg.NLG{
		libnlg.NewAssetWithText("Устройство сообщает мне, что оно этого не умеет."),
	}

	InvalidValue = libnlg.NLG{
		libnlg.NewAssetWithText("К сожалению, такое значение недопустимо."),
	}

	NotSupportedInCurrentMode = libnlg.NLG{
		libnlg.NewAssetWithText("В текущем режиме это невозможно."),
	}
)

// multiple error codes
var (
	MultipleDoorOpenError = libnlg.NLG{
		libnlg.NewAssetWithText("Ой, кажется, на устройствах открыты дверцы. Закройте их и повторите команду."),
	}
	MultipleLidOpenError = libnlg.NLG{
		libnlg.NewAssetWithText("Упс, кажется, вы забыли закрыть крышки на устройствах. Закройте их и повторите команду."),
	}
	MultipleRemoteControlDisabledError = libnlg.NLG{
		libnlg.NewAssetWithText("Сначала нужно спросить разрешения у самих устройств: проверьте, на них должны быть специальные кнопки."),
	}
	MultipleNotEnoughWaterError = libnlg.NLG{
		libnlg.NewAssetWithText("Ой, нет, в ваших устройствах недостаточно воды. Долейте и повторите команду!"),
	}
	MultipleLowChargeLevelError = libnlg.NLG{
		libnlg.NewAssetWithText("Кажется, ваши устройства разрядились. Пожалуйста, повторите после зарядки."),
	}
	MultipleContainerFullError = libnlg.NLG{
		libnlg.NewAssetWithText("Кажется, контейнеры устройств переполнились. Надо бы очистить!"),
	}
	MultipleContainerEmptyError = libnlg.NLG{
		libnlg.NewAssetWithText("Ой, кажется, вы забыли что-то положить - контейнеры в устройствах пусты. Необходимо заполнить!"),
	}
	MultipleDripTrayFullError = libnlg.NLG{
		libnlg.NewAssetWithText("Во всех устройствах заполнились сливные поддоны, надо почистить, а потом повторить команду."),
	}
	MultipleDeviceStuckError = libnlg.NLG{
		libnlg.NewAssetWithText("Алярма, возникли препятствия! Пожалуйста, устраните их."),
	}
	MultipleDeviceOffError = libnlg.NLG{
		libnlg.NewAssetWithText("Ой, не получается, дело в том, что устройства нужно сначала включить."),
	}
	MultipleFirmwareOutOfDateError = libnlg.NLG{
		libnlg.NewAssetWithText("Не могу запустить без обновления - пожалуйста, обновите прошивки на ваших устройствах."),
	}
	MultipleNotEnoughDetergentError = libnlg.NLG{
		libnlg.NewAssetWithText("Я бы рада, да во всех устройствах закончилось моющее средство."),
	}
	MultipleAccountLinkingError = libnlg.NLG{
		libnlg.NewAssetWithText("Эти устройства куда-то отвязались, необходимо снова связать аккаунты в приложении Дом с Алисой."),
	}
	MultipleHumanInvolvementNeededError = libnlg.NLG{
		libnlg.NewAssetWithText("Кажется, что-то пошло не так - для устранения ошибки нужна ваша помощь."),
	}
	MultipleDeviceUnreachable = libnlg.NLG{
		libnlg.NewAssetWithText("Устройства недоступны, проверьте, пожалуйста, интернет или подключены ли они к сети."),
	}
	MultipleDeviceBusy = libnlg.NLG{
		libnlg.NewAssetWithText("Прямо сейчас устройства заняты, давайте попробуем это позже."),
	}
	MultipleDeviceNotFound = libnlg.NLG{
		libnlg.NewAssetWithText("Не могу найти эти устройства."),
	}
	MultipleInternalError = libnlg.NLG{
		libnlg.NewAssetWithText("Что-то пошло не так, и я, к сожалению, не знаю, что."),
	}
	MultipleInvalidAction = libnlg.NLG{
		libnlg.NewAssetWithText("Устройства сообщают мне, что они этого не умеют."),
	}
	MultipleInvalidValue = libnlg.NLG{
		libnlg.NewAssetWithText("К сожалению, такое значение недопустимо."),
	}
	MultipleNotSupportedInCurrentMode = libnlg.NLG{
		libnlg.NewAssetWithText("В текущем режиме работы устройств это невозможно."),
	}
)

var PastAction = libnlg.NLG{
	libnlg.NewAssetWithText("Если бы я могла менять что-то в прошлом, мы бы сейчас уже были в будущем."),
	libnlg.NewAssetWithText("В таком далеком прошлом мои технологии еще не работали."),
	libnlg.NewAssetWithText("Машину времени изобрели, а я не заметила?"),
}

var FutureAction = libnlg.NLG{
	libnlg.NewAssetWithText("Вот это у вас горизонт планирования. Я так далеко не загадываю. Попросите ближе к этой дате?"),
	libnlg.NewAssetWithText("Вы шутите? Это ж еще дожить надо. Попросите меня об этом ближе к дате."),
	libnlg.NewAssetWithText("Это настолько заранее, что я могу забыть. Напомните ближе к тому?"),
}

var NoTimeSpecified = libnlg.NLG{
	libnlg.NewAssetWithText("Окей. А в какое время?"),
	libnlg.NewAssetWithText("Ладно. Но вы должны назвать точное время."),
	libnlg.NewAssetWithText("А вдруг я сделаю это не вовремя? Укажите точное время."),
}

var NoHouseholdSpecifiedAction = libnlg.NLG{
	libnlg.NewAssetWithText("Уточните, где именно?"),
	libnlg.NewAssetWithText("Где мне это сделать?"),
	libnlg.NewAssetWithText("Сделаю, только уточните где."),
	libnlg.NewAssetWithText("В каком доме сделать?"),
}

var NoHouseholdSpecifiedQuery = libnlg.NLG{
	libnlg.NewAssetWithText("Уточните, где именно?"),
}

var CannotFindFan = libnlg.NLG{
	libnlg.NewAssetWithText("С удовольствием. Только сначала подключите его к умному дому. Если у вас обычный вентилятор и умная розетка, вставьте одно в другое и переименуйте розетку в приложении Дом с Алисой. А если ваш вентилятор и сам умный, добавьте его в приложение как самостоятельное умное устройство. Когда всё настроите, магия заработает."),
}

var CannotFindHeater = libnlg.NLG{
	libnlg.NewAssetWithText("С удовольствием. Только сначала подключите его к умному дому. Если у вас обычный обогреватель и умная розетка, вставьте одно в другое и переименуйте розетку в приложении Дом с Алисой. А если ваш обогреватель и сам умный, добавьте его в приложение как самостоятельное умное устройство. Когда всё настроите, магия заработает."),
}

var CannotFindAC = libnlg.NLG{
	libnlg.NewAssetWithText("Легко. Только сначала подключите его к умному дому. Если у вас обычный кондиционер и умный пульт Яндекса, найдите его в приложении Дом с Алисой, нажмите «Добавить пульт», выберите «Кондиционер» — а дальше поможет приложение. А если ваш кондиционер и сам умный, добавьте его в приложение как самостоятельное умное устройство. Когда всё настроите, я смогу управлять кондиционером. Только попросите."),
}

var CannotFindTVBox = libnlg.NLG{
	libnlg.NewAssetWithText("Не вижу ваш ресивер. Вы его подключили? Если у вас обычный ресивер и умный пульт Яндекса, найдите его в приложении Дом с Алисой, нажмите «Добавить пульт», выберите ресивер и следуйте инструкции. А если ваш ресивер и сам умный, добавьте его в приложение как самостоятельное умное устройство. Когда всё будет готово, возвращайтесь — я смогу переключать каналы, включать ресивер и так далее."),
}

var CannotFindTV = libnlg.NLG{
	libnlg.NewAssetWithText("Кажется, вы его не подключили. Если у вас обычный телевизор и умный пульт Яндекса, найдите его в приложении Дом с Алисой, нажмите «Добавить пульт», выберите телевизор и слушайтесь приложения. А если ваш телевизор и сам умный, добавьте его в приложение как самостоятельное умное устройство. Когда всё настроите, возвращайтесь — и будем вместе смотреть кино."),
}

var CannotFindFireplace = libnlg.NLG{
	libnlg.NewAssetWithText("Я бы с радостью, но сначала нужно подключить его к умному дому. Если у вас обычный электрокамин и умный пульт Яндекса, найдите его в приложении Дом с Алисой, нажмите «Добавить пульт» и выберите ручную настройку. Дальше просто слушайтесь приложения. А если ваш электрокамин и сам умный, добавьте его в приложение как самостоятельное умное устройство. Когда всё будет готово, приходите — магия заработает."),
}

var IntervalActionTimeoutReached = libnlg.NLG{
	libnlg.NewAssetWithText("Простите, у меня не получилось сделать то, что вы просите. Пожалуйста, повторите команду снова."),
}

var TimeSpecificationNotSupported = libnlg.NLG{
	libnlg.NewAssetWithText("К сожалению, это действие нельзя запланировать на определённое время."),
}

var WeirdDateTimeRelativity = libnlg.NLG{
	libnlg.NewAssetWithText("Не разобралась, на какое время запланировать действие. Уточните, пожалуйста."),
}

var MultipleHouseholdsInRequest = libnlg.NLG{
	libnlg.NewAssetWithText("Я пока не научилась общаться с устройствами сразу в нескольких домах. Пожалуйста, укажите один."),
}

var TurnOnDevicesIsForbidden = libnlg.NLG{
	libnlg.NewAssetWithText("В целях безопасности я не могу включить всё и сразу. Уточните, пожалуйста, что именно нужно включить и попросите меня снова."),
	libnlg.NewAssetWithText("К сожалению, я не могу включить сразу всё, но вы можете создать подобный сценарий в приложении Дом с Алисой."),
	libnlg.NewAssetWithText("Включать сразу всё небезопасно и дорого. Уточните, пожалуйста, какое устройство включить и снова попросите меня об этом."),
}

var TvIsNotPluggedIn = libnlg.NLG{
	libnlg.NewAssetWithText("Пожалуйста, подключите устройство к телевизору и повторите попытку."),
}

var CantPlayVideoOnDevice = libnlg.NLG{
	libnlg.NewAssetWithText("Простите, я не могу воспроизвести видео на этом устройстве."),
}
