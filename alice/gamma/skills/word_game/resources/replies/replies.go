package replies

import "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"

const (
	GeneralState             = "general"
	StartGameState           = "start"
	RulesState               = "rules"
	DefinitionState          = "definition"
	AfterMainWordChangeState = "mainword"
	TipState                 = "tip"
	UserSetMainWordState     = "user"
	EndRoundState            = "endRound"
	EndGameState             = "endGame"
)

const (
	StartGameReply                   = "Start"
	FallbackReply                    = "Fallback"
	RestartGameReply                 = "RestartGame"
	RestartWithChangeIntentGameReply = "RestartWithChangeIntent"
	RulesGameReply                   = "Rules"
	EndGameStatsReply                = "Stats"
	EndGameReply                     = "EndGame"

	MainWordGameReply = "MainWord"

	UserAllWordsGuessedReply  = "UserAllGuessed"
	AliceAllWordsGuessedReply = "AliceAllGuessed"

	PerfectUserWordReply   = "PerfectUserWord"
	MyWordGameReply        = "My"
	UsedWordGameReply      = "Used"
	UsingMainWordGamePeply = "UsingMain"
	UnknownWordReply       = "Unknown"
	WrongSubsequenceReply  = "WrongSub"
	WrongLetterReply       = "WrongLetter"

	UserStateStartReply          = "UserStateStart"
	WrongMainWordFromUserReply   = "WrongMainWordUser"
	CorrectMainWordFromUserReply = "CorrectMainWordUser"

	DefinitionYandexReply   = "definitionYandex"
	DefinitionWikiReply     = "definitionWiki"
	DefinitionContinueReply = "definitionContinue"

	BetterTipReply = "betterTip"
	TipReply       = "tip"
	NoTipsReply    = "noTips"
	AboutTipReply  = "aboutTip"

	NotNounReply       = "NotNounReply"
	NotCommonNounReply = "NotCommonReply"
)

const ExtraTalk = "extra"

const (
	ExtraTalkGood  = "extraGood"
	ExtraTalkAgree = "extraAgree"
	ExtraTalkThink = "extraThink"
)

const (
	WiktionaryURL  = "https://ru.wiktionary.org/wiki/"
	WiktionaryText = "Посмотреть в Викисловаре"
	YandexURL      = "https://yandex.ru/search/?text=значение+слова+"
	YandexText     = "Искать в Яндексе"
)

var ProperNLU = []string{"geo", "geoname", "fio", "company"}

var LettersTTS = map[string]string{
	"а": "+а",
	"б": "б+э",
	"в": "в+э",
	"г": "г+э",
	"д": "д+э",
	"е": "+е",
	"ё": "ё",
	"ж": "ж+э",
	"з": "з+э",
	"и": "+и",
	"й": "й",
	"к": "к+а",
	"л": "+эл",
	"м": "+эм",
	"н": "+эн",
	"о": "+о",
	"п": "п+э",
	"р": "+эр",
	"с": "+эс",
	"т": "т+э",
	"у": "+у",
	"ф": "+эф",
	"х": "х+а",
	"ц": "ц+э",
	"ч": "ч+э",
	"ш": "ш+э",
	"щ": "щ+а",
	"ъ": "твёрдый знак",
	"ы": "ы <[yy]>", //??
	"ь": "мягкий знак",
	"э": "+э",
	"ю": "+ю",
	"я": "+я",
}

var CueTemplates = map[string]map[string][]dialoglib.CueTemplate{
	GeneralState: {
		StartGameReply: []dialoglib.CueTemplate{
			{
				Text: "Отлично! Мы с вами по очереди составляем маленькие слова из большого. " +
					"Вы можете сменить большое слово, если скажете «новое слово» или «хочу загадать слово». " +
					"Или в любой момент попросить у меня подсказку. А пока давайте попробуем со словом {{.MainWord.Text}}" +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит». {{end}}",
				Voice: "Отлично! Мы с вами по очереди составляем маленькие слова из большого. " +
					"Вы можете сменить большое слово, если скажете «новое слово» или «хочу загадать слово». " +
					"Или в любой момент попросить у меня подсказку. А пока давайте попробуем со словом {{.MainWord.Voice}}" +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит». {{end}}",
			},
		},
		FallbackReply: []dialoglib.CueTemplate{
			{
				Text:  "Извините, я вас не поняла.",
				Voice: "Извините, я вас не поняла.",
			},
		},
	},
	StartGameState: {
		MainWordGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Текущее слово: {{.MainWord.Text}}",
				Voice: "Текущее слово: {{.MainWord.Voice}}",
			},
		},
		MyWordGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Моё слово: {{.Reply}}",
				Voice: "Моё слово: {{.Reply}}",
			},
		},
		UsingMainWordGamePeply: []dialoglib.CueTemplate{
			{
				Text:  "Это загаданное слово. Оно не засчитывается.",
				Voice: "Это загаданное слово. Оно не засчитывается.",
			},
		},
		UsedWordGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Это слово уже было. Давайте другое!",
				Voice: "Это слово уже было. Давайте другое!",
			},
		},
		UnknownWordReply: []dialoglib.CueTemplate{
			{
				Text:  "Этого слова нет в моём словаре.",
				Voice: "Этого сл+ова нет в моём словаре.",
			},
		},
		WrongSubsequenceReply: []dialoglib.CueTemplate{
			{
				Text:  "Ещё одной буквы «{{.Letter.Text}}» нет в слове {{.CurrentWord.Text}}.", //tts для букв добавить
				Voice: "Ещё одной буквы {{.Letter.Voice}} - нет в слове {{.CurrentWord.Voice}}.",
			},
		},
		WrongLetterReply: []dialoglib.CueTemplate{
			{
				Text:  "Буквы «{{.Letter.Text}}» нет в слове {{.CurrentWord.Text}}.", //tts для букв добавить
				Voice: "Буквы {{.Letter.Voice}} - нет в слове {{.CurrentWord.Voice}}.",
			},
		},
		RestartGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Хорошо, пусть будет слово {{.MainWord.Text}}. Начинайте!",
				Voice: "Хорошо, пусть будет слово {{.MainWord.Voice}}. Начинайте!",
			},
		},
		RestartWithChangeIntentGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Итак, мы составляем слова из слова {{.MainWord.Text}}",
				Voice: "Итак, мы составляем слов+а из слова {{.MainWord.Voice}}",
			},
		},
		UserAllWordsGuessedReply: []dialoglib.CueTemplate{
			{
				Text:  "Вы угадали все слова, которые я знала! Ну ничего себе!\nСыграем ещё раз?",
				Voice: "Вы угадали все слова, которые я знала! Ну ничего себе!\nСыграем ещё раз?",
			},
		},
		AliceAllWordsGuessedReply: []dialoglib.CueTemplate{
			{
				Text:  "Мы угадали все слова, которые я знала! Ну ничего себе!\nСыграем ещё раз?",
				Voice: "Мы угадали все слова, которые я знала! Ну ничего себе!\nСыграем ещё раз?",
			},
		},
		PerfectUserWordReply: []dialoglib.CueTemplate{
			{
				Text:  "Вау! Длинное слово.",
				Voice: "Вау! Длинное слово.",
			},
			{
				Text:  "Отличное слово.",
				Voice: "Отличное слово.",
			},
			{
				Text:  "Прекрасно!",
				Voice: "Прекрасно!",
			},
		},
		NotCommonNounReply: []dialoglib.CueTemplate{
			{
				Text:  "В этой игре нельзя использовать имена собственные — только нарицательные.",
				Voice: "В этой игре нельзя использовать имена собственные — только нарицательные.",
			},
		},
		NotNounReply: []dialoglib.CueTemplate{
			{
				Text:  "Слова должны быть существительными. Скажите другое слово.",
				Voice: "Слова должны быть существительными. Скажите другое слово.",
			},
			{
				Text:  "В этой игре можно использовать только существительные. Выберите другое слово.",
				Voice: "В этой игре можно использовать только существительные. Выберите другое слово.",
			},
			{
				Text:  "Мы играем только с существительными. Придумайте другое слово.",
				Voice: "Мы играем только с существительными. Придумайте другое слово.",
			},
		},
	},
	RulesState: {
		RulesGameReply: []dialoglib.CueTemplate{
			{
				Text: "Правила игры просты: я выбираю или вы выбираете какое-нибудь длинное слово. " +
					"Дальше из его букв мы составляем более короткие слова. " +
					"Слова должны быть существительными в единственном числе. " +
					"Составлять слова будем по очереди. " +
					"Вы можете сменить большое слово, если скажете «новое слово» или «хочу загадать слово»." +
					" Или в любой момент попросить у меня подсказку.{{if .IsStation }} Если надоест, скажите: «Алиса, хватит». {{end}}" +
					" \n Продолжаем?",
				Voice: "Правила игры просты: - я выбираю - или вы выбираете - какое-нибудь длинное слово." +
					"  Дальше из его букв мы составляем более короткие слова." +
					" Слов+а должны быть существительными в единственном числе." +
					"  Составлять слов+а будем по очереди. " +
					"Вы можете сменить большое слово, если скажете «новое слово» или «хочу загадать слово»." +
					" Или в любой момент попросить у меня подсказку.{{if .IsStation }} Если надоест, скажите: «Алиса, хватит». {{end}}" +
					" \n Продолжаем?",
			},
		},
	},
	DefinitionState: {
		DefinitionWikiReply: []dialoglib.CueTemplate{
			{
				Text:  "Значения слова {{.Word}}:\n{{.Definitions}}",
				Voice: "Значения сл+ова {{.Word}}:\n{{.Definitions}}",
			},
		},
		DefinitionYandexReply: []dialoglib.CueTemplate{
			{
				Text:  "Под рукой значения этого слова нет, но можем посмотреть в Яндексе.",
				Voice: "Под рукой значения этого сл+ова нет, но можем посмотреть в Яндексе.",
			},
			{
				Text:  "Давайте посмотрим значение в Яндексе!",
				Voice: "Давайте посмотрим значение в Яндексе!",
			},
		},
		DefinitionContinueReply: []dialoglib.CueTemplate{
			{
				Text:  "Продолжаем?",
				Voice: "Продолжаем?",
			},
		},
	},
	TipState: {
		TipReply: []dialoglib.CueTemplate{
			{
				Text:  "{{.Word}} — переставьте эти буквы так, чтобы получилось подходящее слово.",
				Voice: "{{.Word}} — переставьте эти буквы так, чтобы получилось подходящее слово.",
			},
		},
		BetterTipReply: []dialoglib.CueTemplate{
			{
				Text:  "Давайте лучше я дам тебе подсказку.",
				Voice: "Давайте лучше я дам тебе подсказку.",
			},
			{
				Text:  "У меня есть для вас подсказка.",
				Voice: "У меня есть для вас подсказка.",
			},
		},
		NoTipsReply: []dialoglib.CueTemplate{
			{
				Text:  "Кажется, у меня закончились все подсказки.",
				Voice: "Кажется, у меня закончились все подсказки.",
			},
		},
		AboutTipReply: []dialoglib.CueTemplate{
			{
				Text:  "Просто попробуйте переставить буквы местами.",
				Voice: "Просто попробуйте переставить буквы местами.",
			},
		},
	},
	UserSetMainWordState: {
		FallbackReply: []dialoglib.CueTemplate{
			{
				Text:  "Повторите, пожалуйста, слово. Не поняла.",
				Voice: "Повторите, пожалуйста, слово. Не поняла.",
			},
		},
		UserStateStartReply: []dialoglib.CueTemplate{
			{
				Text:  "С каким словом будем играть?",
				Voice: "С каким словом будем играть?",
			},
		},
		CorrectMainWordFromUserReply: []dialoglib.CueTemplate{
			{
				Text:  "Отлично!",
				Voice: "Отлично!",
			},
			{
				Text:  "Принимается!",
				Voice: "Принимается!",
			},
		},
		WrongMainWordFromUserReply: []dialoglib.CueTemplate{
			{
				Text: "Я не знаю ни одного слова, которое можно составить из букв слова {{.Word}}." +
					" Выберите, пожалуйста, другое слово.",
				Voice: "Я не знаю ни одного сл+ова, которое можно составить из букв сл+ова {{.Word}}." +
					" Выберите, пожалуйста, другое слово.",
			},
		},
	},
	ExtraTalk: {
		ExtraTalkGood: []dialoglib.CueTemplate{
			{
				Text:  "Совершенно согласна! Назовите слово, состоящее из букв, которые есть в слове {{.Word.Text}}.",
				Voice: "Совершенно согласна! Назовите слово, состоящее из букв, которые есть в слове {{.Word.Voice}}.",
			},
		},
		ExtraTalkThink: []dialoglib.CueTemplate{
			{
				Text:  "Я буду ждать.",
				Voice: "Я буду ждать.",
			},
			{
				Text:  "Конечно, мы с вами никуда не спешим.",
				Voice: "Конечно, мы с вами никуда не спешим.",
			},
		},
		ExtraTalkAgree: []dialoglib.CueTemplate{
			{
				Text:  "Назовите слово, состоящее из букв, которые есть в слове {{.Word.Text}}.",
				Voice: "Назовите слово, состоящее из букв, которые есть в слове  {{.Word.Voice}}.",
			},
		},
	},
	EndGameState: {
		EndGameStatsReply: []dialoglib.CueTemplate{
			{
				Text:  "Угадано {{.NumberStats}} {{.WordStats}}. Очень неплохо!",
				Voice: "Угадано {{.NumberStats}} {{.WordStats}}. Очень неплохо!",
			},
		},
		EndGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Если захотите ещё поиграть, скажите «Давай сыграем в слова».",
				Voice: "Если захотите ещё поиграть, скажите «Давай сыграем в слова».",
			},
		},
	},
}

type Manager struct {
	dialoglib.RepliesManager
}
