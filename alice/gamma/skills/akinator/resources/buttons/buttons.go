package buttons

import sdk "a.yandex-team.ru/alice/gamma/sdk/golang"

var YesButton = sdk.Button{
	Title: "Да",
	Hide:  true,
}

var HowToPlayButton = sdk.Button{
	Title: "Правила",
	Hide:  true,
}

var StartButtons = []sdk.Button{
	YesButton,
	HowToPlayButton,
}

var OneMoreHintButton = sdk.Button{
	Title: "Еще подсказка",
	Hide:  true,
}

var SayAnswerButton = sdk.Button{
	Title: "Узнать ответ",
	Hide:  true,
}

var HintButtons = []sdk.Button{
	OneMoreHintButton,
	SayAnswerButton,
	HowToPlayButton,
}

var LetThinkButtons = HintButtons[0:2]

var AnswerButtons = []sdk.Button{
	{
		Title: "Да, я ещё подумаю",
		Hide:  true,
	},
	{
		Title: "Нет, скажи ответ",
		Hide:  true,
	},
}

var ContinueButtons = []sdk.Button{
	{
		Title: "Да, играем",
		Hide:  true,
	},
}

var PromoButtons = []sdk.Button{
	{
		Title: "«Что раньше»",
		Hide:  false,
		URL:   "https://dialogs.yandex.ru/store/skills/f0ad683c-chto-ran-sh",
	},
	{
		Title: "Играем в «Угадай актёра»",
		Hide:  true,
	},
}
