package buttons

import sdk "a.yandex-team.ru/alice/gamma/sdk/golang"

var DefaultContinueButtons = []sdk.Button{
	{Title: "Да", Hide: true},
}

var DefaultGameButtons = []sdk.Button{
	{Title: "Правила", Hide: true},
	{Title: "Начать заново", Hide: true},
	{Title: "Сдаться", Hide: true},
}

var AnswersOnShootingButtons = []sdk.Button{
	{Title: "Убил", Hide: true},
	{Title: "Ранил", Hide: true},
	{Title: "Мимо", Hide: true},
}
