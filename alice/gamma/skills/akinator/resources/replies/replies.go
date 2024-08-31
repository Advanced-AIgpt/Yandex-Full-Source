package replies

import (
	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
)

const (
	GeneralState   = "general"
	StartGameState = "start"
	GuessState     = "guess"
	EndRoundState  = "endRound"
	EndGameState   = "end"
)
const (
	StartGameReply          = "startGame"
	DontUnderstandReply     = "dontUnderstand"
	MisUnderstandingReply   = "misunderstand"
	FallbackReply           = "fallback"
	HowToPlayReply          = "howToPlay"
	UserMakesGameReply      = "userMakes"
	NotReadyReply           = "notReady"
	YesStartReply           = "yes"
	NoStartReply            = "no"
	CorrectAnswerReply      = "true"
	WrongAnswerReply        = "false"
	LetUserThinkReply       = "letThink"
	AskAnswerReply          = "askAnswer"
	DontSayAnswerReply      = "dontSay"
	HowToPlayAtTheGameReply = "howPlay"
	OneMoreHintReply        = "nextHint"
	HintReply               = "hint"
	SayAnswerReply          = "sayAnswer"
	LoseReply               = "lose"
	NextRoundReply          = "next"
	PromoReply              = "promo"
	NoMoreActorsReply       = "noMore"
	ExitGameReply           = "exit"
)

var CueTemplates = map[string]map[string][]dialoglib.CueTemplate{
	GeneralState: {
		StartGameReply: []dialoglib.CueTemplate{
			{
				Text: "Я загадываю актёра – вы отгадываете. У вас 5 попыток." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}} Сыграем?",
				Voice: "Я загадываю актёра – вы отгадываете. У вас 5 попыток." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}} Сыграем?",
			},
		},
		MisUnderstandingReply: []dialoglib.CueTemplate{
			{
				Text:  "Хм, я что-то вас не понимаю.",
				Voice: "Хм, я что-то вас не понимаю.",
			},
			{
				Text:  "Так... Мне трудно вас понять.",
				Voice: "Так... Мне трудно вас понять.",
			},
		},
		DontUnderstandReply: []dialoglib.CueTemplate{
			{
				Text: "Мне казалось вы хотели сыграть в «Угадай актёра»" +
					" (я рассказываю факты из биографии какого-нибудь актёра, а вы пытаетесь его угадать)." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}} " +
					"Сыграем?",
				Voice: "Мне казалось вы хотели сыграть в «Угадай актёра» " +
					"(я рассказываю факты из биографии какого-нибудь актёра, а вы пытаетесь его угадать)." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}} " +
					"Сыграем?",
			},
			{
				Text: "Вы ведь хотели поиграть в «Угадай актёра»!" +
					" Я уже загадала актёра и подобрала факты для него. " +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}} " +
					"Может, сыграем?",
				Voice: "Вы ведь хотели поиграть в «Угадай актёра»!" +
					" Я уже загадала актёра и подобрала факты для него. " +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}} " +
					"Может, сыграем?",
			},
		},
		FallbackReply: []dialoglib.CueTemplate{
			{
				Text:  "Извините, я вас не поняла.",
				Voice: "Извините, я вас не поняла.",
			},
		},
		UserMakesGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Нет-нет, загадывать буду я. Сыграем?",
				Voice: "Нет-нет, загадывать буду я. Сыграем?",
			},
		},
		NotReadyReply: []dialoglib.CueTemplate{
			{
				Text:  "Ок. Я подожду.",
				Voice: "Ок. Я подожду.",
			},
		},
	},
	StartGameState: {
		YesStartReply: []dialoglib.CueTemplate{
			{
				Text:  "Начинаем.",
				Voice: "Начинаем.",
			},
			{
				Text:  "Погнали.",
				Voice: "Погнали.",
			},
		},
		NoStartReply: []dialoglib.CueTemplate{
			{
				Text:  "Ну нет так нет.",
				Voice: "Ну нет так нет.",
			},
			{
				Text:  "Давайте закончим!",
				Voice: "Давайте закончим!",
			},
			{
				Text:  "Хорошо, давайте закончим!",
				Voice: "Хорошо, давайте закончим!",
			},
			{
				Text:  "Тогда заканчиваем.",
				Voice: "Тогда заканчиваем.",
			},
		},
		HowToPlayReply: []dialoglib.CueTemplate{
			{
				Text: "Давайте я напомню правила игры. Я рассказываю факты из биографии какого-нибудь актёра," +
					" а вы пытаетесь его угадать. У вас пять попыток. Сыграем?",
				Voice: "Давайте я напомню правила игры. Я рассказываю факты из биографии какого-нибудь актёра," +
					" а вы пытаетесь его угадать. У вас пять попыток. Сыграем?",
			},
		},
	},
	GuessState: {
		CorrectAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Оскар за сообразительность достается вам!",
				Voice: "Оскар за сообразительность достается вам!",
			},
			{
				Text:  "Верно! Вы неотразимы!",
				Voice: "Верно! Вы неотразимы!",
			},
			{
				Text:  "Отлично сработано!",
				Voice: "Отлично сработано!",
			},
		},
		WrongAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Нет.",
				Voice: "Нет.",
			},
			{
				Text:  "Не то.",
				Voice: "Не то.",
			},
			{
				Text:  "Неправильно.",
				Voice: "Неправильно.",
			},
			{
				Text:  "Не угадали.",
				Voice: "Не угадали.",
			},
		},
		LetUserThinkReply: []dialoglib.CueTemplate{
			{
				Text:  "Вижу, вам требуется время подумать. Я подожду. Только чур не искать в Яндексе!",
				Voice: "Вижу, вам требуется время подумать. Я подожду. Только чур не искать в Яндексе!",
			},
		},
		AskAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Рано вы сдаётесь! Может, ещё подумаете?",
				Voice: "Рано вы сдаётесь! Может, ещё подумаете?",
			},
		},
		DontSayAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Хвалю за настойчивость! Повторю подсказку.",
				Voice: "Хвалю за настойчивость! Повторю подсказку.",
			},
		},
		HowToPlayAtTheGameReply: []dialoglib.CueTemplate{
			{
				Text: "Я рассказываю факты из биографии какого-нибудь актёра," +
					" а вы пытаетесь его угадать. Это весело! Давайте попробуем ещё раз.",
				Voice: "Я рассказываю факты из биографии какого-нибудь актёра," +
					" а вы пытаетесь его угадать. Это весело! Давайте попробуем ещё раз.",
			},
		},
		OneMoreHintReply: []dialoglib.CueTemplate{
			{
				Text:  "Ок.",
				Voice: "Ок.",
			},
			{
				Text:  "Хорошо.",
				Voice: "Хорошо.",
			},
			{
				Text:  "Ладно.",
				Voice: "Ладно.",
			},
			{
				Text:  "Ну хорошо.",
				Voice: "Ну хорошо.",
			},
		},
		HintReply: []dialoglib.CueTemplate{
			{
				Text:  "{{.CurrentHint}}",
				Voice: "{{.CurrentHint}}",
			},
		},
	},
	EndRoundState: {
		SayAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Ну и ладно. Ответ - {{.NameToWrite}}.",
				Voice: "Ну и ладно. Ответ - {{.NameToSay}}.",
			},
		},
		LoseReply: []dialoglib.CueTemplate{
			{
				Text:  "Все подсказки закончились. Правильный ответ - {{.NameToWrite}}. Не расстраивайтесь.",
				Voice: "Все подсказки закончились. Правильный ответ - {{.NameToSay}}. Не расстраивайтесь.",
			},
		},
		NextRoundReply: []dialoglib.CueTemplate{
			{
				Text:  "Хотите сыграть ещё?",
				Voice: "Хотите сыграть ещё?",
			},
			{
				Text:  "Сыграем ещё раз?",
				Voice: "Сыграем ещё раз?",
			},
		},
		PromoReply: []dialoglib.CueTemplate{
			{
				Text:  "Кстати, у меня есть ещё игра «Что раньше». Пройдите по ссылке, чтобы попробовать.",
				Voice: "Кстати, у меня есть ещё игра «Что раньше». Пройдите по ссылке, чтобы попробовать.",
			},
		},
	},
	EndGameState: {
		NoMoreActorsReply: []dialoglib.CueTemplate{
			{
				Text:  "Вы угадали всех актёров в моей базе! Пожалуйста, выберите другую игру.",
				Voice: "Вы угадали всех актёров в моей базе! Пожалуйста, выберите другую игру.",
			},
		},
		ExitGameReply: []dialoglib.CueTemplate{
			{
				Text: "Если захотите ещё раз поиграть в «Угадай актёра»," +
					" просто скажите «Давай сыграем в Угадай актёра».",
				Voice: "Если захотите ещё раз поиграть в «Угадай актёра»," +
					" просто скажите «Давай сыграем в Угадай актёра».",
			},
		},
	},
}
