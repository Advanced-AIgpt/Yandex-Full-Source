package patterns

import (
	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

const (
	ShowRulesIntent      = "rules"
	YesContinueIntent    = "yes"
	NoContinueIntent     = "no"
	CorrectGuessIntent   = "correct"
	IncorrectGuessIntent = "incorrect"
	AnotherSearchIntent  = "another"
	SearchIntent         = "search"
	EndGameIntent        = "end"
	ExplicitIntent       = "explicit"
)

var ContinueGame = []sdk.Pattern{
	{
		Name:    YesContinueIntent,
		Pattern: "* (ага|угу|давай|да|хорошо|конечно|продолжаем|поехали|ещё) *",
	},
	{
		Name:    NoContinueIntent,
		Pattern: "(не надо|не хочу|неа|нет|не|потом)",
	},
}

var GuessSong = []sdk.Pattern{
	{
		Name:    IncorrectGuessIntent,
		Pattern: "(не (то|та|угадала|прав*|попала)|нет|промах|(другой|еще|следующий) вариант|ошиблась|непривильн*)",
	},
	{
		Name:    CorrectGuessIntent,
		Pattern: "(угадала|молодец|* прав*|супер|круто|красавчик|класс)",
	},
	{
		Name:    AnotherSearchIntent,
		Pattern: "(новая|другая) строчка",
	},
}

var GlobalCommands = []sdk.Pattern{
	{
		Name: EndGameIntent,
		Pattern: "* (перерыв|хватит|перестань|замолчи|остановись) * |" +
			"[я] устал* |" +
			"* (сегодня (хватит|достаточно|все) [хватит] [достаточно]) * |" +
			"Алиса, хватит.",
	},
	{
		Name:    ShowRulesIntent,
		Pattern: "(как играть|* правила|что делать|как выйти)",
	},
}

var Explicit = []sdk.Pattern{
	{
		Name: ExplicitIntent,
		Pattern: "* (*пизд*|*сучк*|сука|суки|суч*|ипать*|*пизьд*|аху*|наху*|ахре*|охрене*|херн*|*пезд*|пiдр*|" +
			"педар*|педер*|разеб*|ахер*|говн*|дерьм*|*говно*|гавн*|говен*|гавен*|охуе*|*пидор*|*пидар*|*мудох*|*хуй*|" +
			"*хуи*|поху*|порно*|хуя*|хуе*|хуё*|пидр*|пидор*|пидар*|шлюх*|чмыр*|чмо*|мудл*|муда*|муди*|мудо*|отеб*|" +
			"*траха*|*трахн*|*бляд*|*блят*|бля|наеб*|поеб*|педор*|педр*|пезд*|пидер*|eб*|замуд*|*ъеб*|ниху*|" +
			"гандон*|*ъёб*|*выеб*|*заеб*|заёб*|еба*|ебо*|отеб*|проеб*|вебат*|вебан*|наеб*|уеб*|ебу*|ябу*|яба*|ябе*|" +
			"eb*|еб*|hyi*|*pizd*|ohu*|ahu*|hu*|*trahat*|*trahn*|*blyad*|*blyat*|eba*|vyeb*.|*oeb*|" +
			"(пошла/пошел) [бы] ты на|(да/а) [не] (пошла/пошел) [бы] ты|не пойти бы тебе на|" +
			"обосса*|онанист*|онанизм*|мастурб*|куни|кунил*|дроч*|подроч*|отсос*|~анал|~анальный|анус*|капец|копец|" +
			"манда|член*|клитор|вагин*|пенис*|*еблан*|уебан*) *",
	},
}
