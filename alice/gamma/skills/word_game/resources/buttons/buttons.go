package buttons

import sdk "a.yandex-team.ru/alice/gamma/sdk/golang"

var NoTipsButtons = []sdk.Button{
	{Title: "Новое слово", Hide: true},
	{Title: "Хочу загадать слово", Hide: true},
	{Title: "Значение слова", Hide: true},
	{Title: "Правила", Hide: true},
}

var DefaultGameButtons = []sdk.Button{
	{Title: "Подсказка", Hide: true},
	{Title: "Новое слово", Hide: true},
	{Title: "Хочу загадать слово", Hide: true},
	{Title: "Значение слова", Hide: true},
	{Title: "Правила", Hide: true},
}

var NoDefinitionGameButtons = []sdk.Button{
	{Title: "Подсказка", Hide: true},
	{Title: "Новое слово", Hide: true},
	{Title: "Хочу загадать слово", Hide: true},
	{Title: "Правила", Hide: true},
}

var UserMainWordButtons = []sdk.Button{
	{Title: "Случайное слово", Hide: true},
	{Title: "Правила", Hide: true},
}

var DefaultContinueButtons = []sdk.Button{
	{Title: "Да", Hide: true},
}

func NewContinueDefinitionButtons(text, url string) []sdk.Button {
	buttons := continueDefinitionButtons
	buttons[0].Title = text
	buttons[0].URL = url
	return buttons
}

var continueDefinitionButtons = []sdk.Button{
	{Title: "", URL: "", Hide: false},
	{Title: "Да", Hide: true},
}
