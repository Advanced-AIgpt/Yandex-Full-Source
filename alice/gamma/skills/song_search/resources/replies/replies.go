package replies

import (
	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/song_search/api"
)

const (
	GeneralState = "general"
	PauseState   = "pause"
	SearchState  = "search"
	GuessState   = "guess"
	LoseState    = "lose"
	EndGameState = "end"
)

const (
	StartGameReply = "Start"
	RulesReply     = "Rules"
	FallbackReply  = "Fallback"

	YesContinueReply = "Yes"
	NoContinueReply  = "No"

	EngageReply = "Engage"

	FirstSearchReply   = "FirstSearch"
	ShortSearchReply   = "ShortSearch"
	MoreWordsReply     = "MoreWords"
	AnotherGuessReply  = "AnotherGuess"
	AnotherGameReply   = "AnotherGame"
	NoResultsReply     = "NoResults"
	SearchEndReply     = "SearchEnd"
	ThankfulReply      = "Thankful"
	PauseFallbackReply = "PauseFallback"

	normalGuessReply         = "NormalGuess"
	obsceneGuessPreviewReply = "ObsceneGuessPreview"
	obsceneGuessTextReply    = "ObsceneGuessText"

	AnotherRoundReply = "AnotherRound"

	EndGameReply = "EndGame"
)

var CueTemplates = map[string]map[string][]dialoglib.CueTemplate{
	GeneralState: {
		StartGameReply: []dialoglib.CueTemplate{
			{
				Text: "Я угадаю любую мелодию с семи нот!" +
					"{{if .IsStation}} Если надоест, скажите: «Алиса, хватит».{{end}}",
				Voice: "Я угадаю любую мелодию с семи нот!" +
					"{{if .IsStation}} Если надоест, скажите: «Алиса, хватит».{{end}}",
			},
			{
				Text: "Музыка — моя стихия." +
					"{{if .IsStation}} Если надоест, скажите: «Алиса, хватит».{{end}}",
				Voice: "Музыка — моя стихия." +
					"{{if .IsStation}} Если надоест, скажите: «Алиса, хватит».{{end}}",
			},
		},
		RulesReply: []dialoglib.CueTemplate{
			{
				Text: "Вот правила игры: вы говорите строчку из песни на русском языке, " +
					"а я подхватываю и говорю следующие. " +
					"Строчка должна состоять из трех и более слов. " +
					"Если я ошибусь, вы сможете попросить меня поискать другие варианты с этим текстом." +
					"{{if .IsStation}} Если надоест, скажите: «Алиса, хватит».{{end}} Продолжим?",
				Voice: "Вот правила игр+ы: вы говорите строчку из песни на русском языке, " +
					"а я подхватываю и говорю следующие. " +
					"Строчка должна состоять из трех и более слов. " +
					"Если я ошибусь, вы сможете попросить меня поискать другие варианты с этим текстом." +
					"{{if .IsStation}} Если надоест, скажите: «Алиса, хватит».{{end}} Продолжим?",
			},
		},
		FallbackReply: []dialoglib.CueTemplate{
			{
				Text:  "Извините, я вас не поняла.",
				Voice: "Извините, я вас не поняла.",
			},
		},
	},
	PauseState: {
		YesContinueReply: []dialoglib.CueTemplate{
			{
				Text:  "Отлично.",
				Voice: "Отлично.",
			},
			{
				Text:  "Прекрасно.",
				Voice: "Прекрасно.",
			},
			{
				Text:  "Класс.",
				Voice: "Класс.",
			},
		},
		NoContinueReply: []dialoglib.CueTemplate{
			{
				Text:  "Хорошо.",
				Voice: "Хорошо.",
			},
			{
				Text:  "Договорились.",
				Voice: "Договорились.",
			},
			{
				Text:  "Тогда заканчиваем.",
				Voice: "Тогда заканчиваем.",
			},
		},
		PauseFallbackReply: []dialoglib.CueTemplate{
			{
				Text:  "Извините, я вас не поняла. Просто скажите мне, да или нет. Продолжим игру?",
				Voice: "Извините, я вас не поняла. Просто скажите мне, да или нет. Продолжим игру?",
			},
		},
	},
	SearchState: {
		EngageReply: []dialoglib.CueTemplate{
			{
				Text:  "Напомните мне какую-нибудь строчку из песни на русском языке.",
				Voice: "Напомните мне какую-нибудь строчку из песни на русском языке.",
			},
		},
		ShortSearchReply: []dialoglib.CueTemplate{
			{
				Text:  "Как-то маловато слов.",
				Voice: "Как-то маловато слов.",
			},
			{
				Text:  "Я угадываю песни только с трех слов.",
				Voice: "Я угадываю песни только с трех слов.",
			},
			{
				Text:  "Нашла слишком много песен.",
				Voice: "Нашла слишком много песен.",
			},
		},
		MoreWordsReply: []dialoglib.CueTemplate{
			{
				Text:  "Можно поподробнее?",
				Voice: "Можно поподробнее?",
			},
			{
				Text:  "Давайте цитату чуть подлиннее.",
				Voice: "Давайте цитату чуть подлиннее.",
			},
			{
				Text:  "Скажите хотя бы строчку.",
				Voice: "Скажите хотя бы строчку.",
			},
		},
	},
	GuessState: {
		FirstSearchReply: []dialoglib.CueTemplate{
			{
				Text: "Если я ошиблась, вы можете попросить меня поискать другие варианты с этим текстом. " +
					"Если всё правильно, я могу попробовать угадать ещё одну песню. Просто скажите строчку.",
				Voice: "Если я ошиблась, вы можете попросить меня поискать другие варианты с этим текстом. " +
					"Если всё правильно, я могу попробовать угадать ещё одну песню. Просто скажите строчку."},
		},
		ThankfulReply: []dialoglib.CueTemplate{
			{
				Text:  "Я старалась.",
				Voice: "Я старалась.",
			},
			{
				Text:  "Приятно слышать.",
				Voice: "Приятно слышать.",
			},
		},
		AnotherRoundReply: []dialoglib.CueTemplate{
			{
				Text:  "Если хотите, чтобы я угадала ещё какую-нибудь песню, просто скажите строчку.",
				Voice: "Если хотите, чтобы я угадала ещё какую-нибудь песню, просто скажите строчку.",
			},
		},
		AnotherGuessReply: []dialoglib.CueTemplate{
			{
				Text:  "Может быть, эта?",
				Voice: "Может быть, эта?",
			},
			{
				Text:  "Как насчёт этой?",
				Voice: "Как насчёт этой?",
			},
			{
				Text:  "Я просто так не сдамся. Эта?",
				Voice: "Я просто так не сдамся. Эта?",
			},
		},
		normalGuessReply: []dialoglib.CueTemplate{
			{
				Text:  "{{.Text}}\n\n{{.Artist}} - {{.Title}}",
				Voice: "{{.Text}}\n\n{{.Artist}} - {{.Title}}",
			},
		},
		obsceneGuessPreviewReply: []dialoglib.CueTemplate{
			{
				Text:  "Вы что! Я это петь не буду. Даже у голосового помощника есть совесть.",
				Voice: "Вы что! Я это петь не буду. Даже у голосового помощника есть совесть.",
			},
			{
				Text:  "Давайте я сделаю вид, что этого не слышала. А вы сделаете вид, что ничего не говорили. И попробуем другую песню.",
				Voice: "Давайте я сделаю вид, что этого не слышала. А вы сделаете вид, что ничего не говорили. И попробуем другую песню.",
			},
			{
				Text:  "Давайте лучше что-нибудь другое — я таких слов вслух не говорю.",
				Voice: "Давайте лучше что-нибудь другое — я таких слов вслух не говорю.",
			},
			{
				Text:  "Серьезно? Уши вянут от такого. Хоть у меня и нет ушей. Давайте попробуем другую песню.",
				Voice: "Серьезно? Уши вянут от такого. Хоть у меня и нет ушей. Давайте попробуем другую песню.",
			},
			{
				Text:  "За кого вы меня принимаете? Давайте что-нибудь ещё.",
				Voice: "За кого вы меня принимаете? Давайте что-нибудь ещё.",
			},
			{
				Text:  "Давайте попробуем другую песню.",
				Voice: "Давайте попробуем другую песню.",
			},
			{
				Text:  "Давайте что-нибудь ещё.",
				Voice: "Давайте что-нибудь ещё.",
			},
			{
				Text:  "Лучше что-нибудь другое.",
				Voice: "Лучше что-нибудь другое.",
			},
		},
		obsceneGuessTextReply: []dialoglib.CueTemplate{
			{
				Text: "Если я не ошибаюсь, это {{.Artist}} - {{.Title}}. " +
					"Но я это петь не буду. Вдруг нас дети услышат.",
				Voice: "Если я не ошибаюсь, это {{.Artist}} - {{.Title}}. " +
					"Но я это петь не буду. Вдруг нас дети услышат.",
			},
			{
				Text: "Кажется, это {{.Artist}} - {{.Title}}. " +
					"Я такое петь не могу — программист Алексей не разрешает. " +
					"Но можете послушать. На свой страх и риск.",
				Voice: "Кажется, это {{.Artist}} - {{.Title}}. " +
					"Я такое петь не могу — программист Алексей не разрешает. " +
					"Но можете послушать. На свой страх и риск.",
			},
			{
				Text: "Давайте я сделаю вид, что этого не слышала. " +
					"Очень похоже на {{.Artist}} - {{.Title}}. " +
					"А мне такое петь нельзя.",
				Voice: "Давайте я сделаю вид, что этого не слышала. " +
					"Очень похоже на {{.Artist}} - {{.Title}}. " +
					"А мне такое петь нельзя.",
			},
			{
				Text: "Вы что. А если нас дети услышат? " +
					"Это ведь {{.Artist}} - {{.Title}}. Я это петь не могу.",
				Voice: "Вы что. А если нас дети услышат? " +
					"Это ведь {{.Artist}} - {{.Title}}. Я это петь не могу.",
			},
		},
	},
	LoseState: {
		NoResultsReply: []dialoglib.CueTemplate{
			{
				Text:  "Похоже, знаток музыки тут всё же не я.",
				Voice: "Похоже, знаток музыки тут всё же не я.",
			},
			{
				Text:  "Эта песня мне незнакома.",
				Voice: "Эта песня мне незнакома.",
			},
			{
				Text:  "Ничего не могу найти.",
				Voice: "Ничего не могу найти.",
			},
			{
				Text:  "Это что-то новое для меня.",
				Voice: "Это что-то новое для меня.",
			},
			{
				Text:  "Кажется, знаток музыки тут не я.",
				Voice: "Кажется, знаток музыки тут не я.",
			},
		},
		SearchEndReply: []dialoglib.CueTemplate{
			{
				Text:  "Больше ничего не могу найти. Даже я неидеальна.",
				Voice: "Больше ничего не могу найти. Даже я неидеальна.",
			},
			{
				Text:  "Не знаю никаких других песен с этой строчкой. Я не волшебница, я только учусь.",
				Voice: "Не знаю никаких других песен с этой строчкой. Я не волшебница, я только учусь.",
			},
		},
		AnotherGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Сыграем ещё раз?",
				Voice: "Сыграем ещё раз?",
			},
			{
				Text:  "Ещё?",
				Voice: "Ещё?",
			},
			{
				Text:  "Давайте ещё?",
				Voice: "Давайте ещё?",
			},
			{
				Text:  "Играем ещё раз?",
				Voice: "Играем ещё раз?",
			},
		},
	},
	EndGameState: {
		EndGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Захотите поиграть ещё - скажите Алиса, давай поиграем в Угадай песню.",
				Voice: "Захотите поиграть ещё - скажите Алиса, давай поиграем в Угадай песню.",
			},
		},
	},
}

type Manager struct {
	dialoglib.RepliesManager
}

func (manager *Manager) ChooseGuessCueTemplate(guess api.SearchResult, explicitPreview bool) *dialoglib.CueTemplate {
	if guess.Explicit {
		return manager.ChooseCueTemplate(GuessState, obsceneGuessTextReply)
	} else if explicitPreview {
		return manager.ChooseCueTemplate(GuessState, obsceneGuessPreviewReply)
	} else {
		return manager.ChooseCueTemplate(GuessState, normalGuessReply)
	}
}
