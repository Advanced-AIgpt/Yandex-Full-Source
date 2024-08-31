package buttons

import (
	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

var GeneralButtons = []sdk.Button{
	{Title: "Правила", Hide: true},
}

var PauseButtons = []sdk.Button{
	{Title: "Да", Hide: true},
	{Title: "Нет", Hide: true},
}

var NewGameButtons = []sdk.Button{
	{Title: "Да, ещё раз", Hide: true},
}

func FormGuessButtons(url string) []sdk.Button {
	return []sdk.Button{
		{Title: "Другой вариант", Hide: true},
		{Title: "Новая строчка", Hide: true},
		{Title: "Послушать на Я.Музыке", URL: url},
		{Title: "Правила", Hide: true},
	}
}
