package replies

import dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"

const (
	GeneralState      = "general"
	StartState        = "start"
	MyShootState      = "my_shoot"
	UserShootState    = "user_shoot"
	UserCheatingState = "user_cheating_state"
	EndRoundState     = "end_round"
	EndGameState      = "end_game"
)

const (
	StartReply    = "start"
	RestartReply  = "restart"
	RulesReply    = "rules"
	WaitReply     = "wait"
	FallbackReply = "fallback"

	PrepareFieldReply = "prepare"

	MyShootReply = "my_shoot"

	KillReply           = "kill"
	InjuredReply        = "injured"
	AwayReply           = "away"
	CellUsedReply       = "used"
	InjuredReShootReply = "injured re-shoot"
	AwayReShootReply    = "away re-shoot"

	EndCheatingStateReply  = "EndCheating"
	LargeShipReply         = "large ship"
	WrongShipSequenceReply = "wrong ship sequence"
	WrongInjuryReply       = "wrong injury"

	EndRoundReply  = "end_round"
	WinReply       = "win"
	NotWinReply    = "notwin"
	LoseReply      = "lose"
	SurrenderReply = "surrender"

	EndReply = "end"
)

var LettersTTS = map[string]string{
	"а": "а",
	"б": "б",
	"в": "вээ",
	"г": "г",
	"д": "д",
	"е": "ye",
	"ж": "ж",
	"з": "з",
	"и": "и",
	"к": "ка",
}

var CueTemplates = map[string]map[string][]dialoglib.CueTemplate{
	GeneralState: {
		StartReply: []dialoglib.CueTemplate{
			{
				Text:  "Конечно, давайте сыграем в \"Морской бой\". Начнём?",
				Voice: "Конечно, давайте сыграем в \"Морской бой\". Начнём?",
			},
			{
				Text:  "Йо-хо-хо и бутылка рома. Конечно, сыграем. Начинаем?",
				Voice: "Йо-хо-хо и бутылка рома. Конечно, сыграем. Начинаем?",
			},
			{
				Text:  "Отлично, играем в \"Морской бой\". Начинаем игру?",
				Voice: "Отлично, играем в \"Морской бой\". Начинаем игру?",
			},
		},
		RestartReply: []dialoglib.CueTemplate{
			{
				Text:  "Хорошо, давайте сыграем заново. Начинаем?",
				Voice: "Хорошо, давайте сыграем заново. Начинаем?",
			},
			{
				Text:  "Ладно, начнем заново. Играем?",
				Voice: "Ладно, начнем заново. Играем?",
			},
		},
		RulesReply: []dialoglib.CueTemplate{
			{
				Text: "Напоминаю правила: в начале игры каждый из игроков размещает флот кораблей на своем игровом поле — квадрат 10×10.\n" +
					"Горизонтали нумеруются сверху вниз от 1 до 10, а вертикали помечаются буквами слева направо от А до К. " +
					"Всего мы выставляем по 10 кораблей: 1 четырёхпалубный корабль, 2 трёхпалубных корабля, 3 двухпалубных корабля, 4 однопалубных корабля. " +
					"Корабли не должны иметь клетки, которые нахоядстя рядом, даже по диагонали.\n" +
					"Затем мы по очереди стреляем и стараемся потопить чужие корабли. Ход игрока заканчивается, когда он промахивается.\n" +
					"Во время своего хода можно назвать букву и число или имя и число. Во втором случае я возьму первую букву из имени вместо буквы.",
				Voice: "Напоминаю правила: в начале игры каждый из игроков размещает флот кораблей на своем игровом поле — квадрат 10×10.\n" +
					"Горизонтали нумеруются сверху вниз от 1 до 10, а вертикали помечаются буквами слева направо от А до К. " +
					"Всего мы выставляем по 10 кораблей: 1 четырёхпалубный корабль, 2 трёхпалубных корабля, 3 двухпалубных корабля, 4 однопалубных корабля. " +
					"Корабли не должны иметь клетки, которые нахоядстя рядом, даже по диагонали.\n" +
					"Затем мы по очереди стреляем и стараемся потопить чужие корабли. Ход игрока заканчивается, когда он промахивается.\n" +
					"Во время своего хода можно назвать букву и число или имя и число. Во втором случае я возьму первую букву из имени вместо буквы.",
			},
		},
		WaitReply: []dialoglib.CueTemplate{
			{
				Text:  "Хорошо. Я подожду.",
				Voice: "Хорошо. Я подожду.",
			},
			{
				Text:  "Буду ждать",
				Voice: "Буду ждать",
			},
		},
		FallbackReply: []dialoglib.CueTemplate{
			{
				Text:  "Извините, я вас не поняла.",
				Voice: "Извините, я вас не поняла.",
			},
		},
	},

	StartState: {
		PrepareFieldReply: []dialoglib.CueTemplate{
			{
				Text:  "Приготовьте, пожалуйста, поле. Как будете готовы, стреляйте.",
				Voice: "Приготовьте, пожалуйста, поле. Как будете готовы, стреляйте.",
			},
		},
	},

	MyShootState: {
		KillReply: []dialoglib.CueTemplate{
			{
				Text:  "Ура!",
				Voice: "Ура!",
			},
			{
				Text:  "Отлично!",
				Voice: "Отлично!",
			},
			{
				Text:  "А у меня неплохо получается!",
				Voice: "А у меня неплохо получается!",
			},
		},
		MyShootReply: []dialoglib.CueTemplate{
			{
				Text:  "Мой выстрел: «{{.Letter.Text}}» {{.Number}}",
				Voice: "Мой выстрел: {{.Letter.Voice}} {{.Number}}",
			},
			{
				Text:  "Давайте попробуем: «{{.Letter.Text}}» {{.Number}}",
				Voice: "Давайте попробуем: «{{.Letter.Voice}}» {{.Number}}",
			},
			{
				Text:  "Пусть будет: «{{.Letter.Text}}» {{.Number}}",
				Voice: "Пусть будет: «{{.Letter.Voice}}» {{.Number}}",
			},
		},
		InjuredReply: []dialoglib.CueTemplate{
			{
				Text:  "Это было несложно.",
				Voice: "Это было несложно.",
			},
			{
				Text:  "Везёт сильнейшим!",
				Voice: "Везёт сильнейшим!",
			},
		},
		AwayReply: []dialoglib.CueTemplate{
			{
				Text:  "Как жаль. Ваша очередь стрелять.",
				Voice: "Как жаль. Ваша очередь стрелять.",
			},
			{
				Text:  "Эх, стреляйте.",
				Voice: "Эх, стреляйте.",
			},
			{
				Text:  "Ну и ну, обычно я не промахиваюсь. Ваш ход.",
				Voice: "Ну и ну, обычно я не промахиваюсь. Ваш ход.",
			},
		},
		NotWinReply: []dialoglib.CueTemplate{
			{
				Text:  "Пока флот полностью не потоплен, вы можете продолжать игру.",
				Voice: "Пока флот полностью не потоплен, вы можете продолжать игру.",
			},
			{
				Text:  "У вас еще есть корабли, продолжайте.",
				Voice: "У вас еще есть корабли, продолжайте.",
			},
		},
	},

	UserShootState: {
		KillReply: []dialoglib.CueTemplate{
			{
				Text:  "Вы убили мой корабль. Ваш ход.",
				Voice: "Вы убили мой корабль. Ваш ход.",
			},
			{
				Text:  "Вы потопили мой корабль. Стреляйте дальше.",
				Voice: "Вы потопили мой корабль. Стреляйте дальше.",
			},
			{
				Text:  "Ну всё, потопили. Продолжайте.",
				Voice: "Ну всё, потопили. Продолжайте.",
			},
		},
		InjuredReply: []dialoglib.CueTemplate{
			{
				Text:  "Ранен. Стреляйте дальше.",
				Voice: "Ранен. Стреляйте дальше.",
			},
			{
				Text:  "Есть пробитие, мой корабль ранен. Продолжайте.",
				Voice: "Есть пробитие, мой корабль ранен. Продолжайте.",
			},
			{
				Text:  "Попали, но не убили. Ваш ход.",
				Voice: "Попали, но не убили. Ваш ход.",
			},
		},
		AwayReply: []dialoglib.CueTemplate{
			{
				Text:  "Мимо. Теперь моя очередь.",
				Voice: "Мимо. Теперь моя очередь.",
			},
			{
				Text:  "Промах. В следующий раз повезет. Мой ход.",
				Voice: "Промах. В следующий раз повезет. Мой ход.",
			},
			{
				Text:  "Не попали. Теперь я стреляю.",
				Voice: "Не попали. Теперь я стреляю.",
			},
		},
		CellUsedReply: []dialoglib.CueTemplate{
			{
				Text:  "Вы уже стреляли в эту клетку!",
				Voice: "Вы уже стреляли в эту клетку!",
			},
			{
				Text:  "Кажется, вы забыли, что стреляли в эту клетку.",
				Voice: "Кажется, вы забыли, что стреляли в эту клетку.",
			},
		},
		InjuredReShootReply: []dialoglib.CueTemplate{
			{
				Text:  "И тут стояла часть моего корабля. Поменяйте ход, пожалуйста.",
				Voice: "И тут стояла часть моего корабля. Поменяйте ход, пожалуйста.",
			},
			{
				Text:  "И тут стояла часть моего корабля. Выберите другую клетку.",
				Voice: "И тут стояла часть моего корабля. Выберите другую клетку.",
			},
		},
		AwayReShootReply: []dialoglib.CueTemplate{
			{
				Text:  "И тут ничего не было. Выберите другую клетку.",
				Voice: "И тут ничего не было. Выберите другую клетку.",
			},
			{
				Text:  "И тут ничего не было. Поменяйте ход, пожалуйста.",
				Voice: "И тут ничего не было. Поменяйте ход, пожалуйста.",
			},
		},
	},

	UserCheatingState: {
		LargeShipReply: []dialoglib.CueTemplate{
			{
				Text:  "Похоже, что вы поставили слишком большой корабль на свое поле.",
				Voice: "Похоже, что вы поставили слишком большой корабль на свое поле.",
			},
		},
		WrongInjuryReply: []dialoglib.CueTemplate{
			{
				Text:  "Ваш корабль ранен, но все клетки рядом с ним не могут содержать часть корабля. Будем считать, что я потопила ваш корабль.",
				Voice: "Ваш корабль ранен, но все клетки рядом с ним не могут содержать часть корабля. Будем считать, что я потопила ваш корабль.",
			},
		},
		WrongShipSequenceReply: []dialoglib.CueTemplate{
			{
				Text:  "Кажется, у ваc неправильный набор кораблей на поле.",
				Voice: "Кажется, у ваc неправильный набор кораблей на поле.",
			},
		},
	},
	EndRoundState: {
		EndRoundReply: []dialoglib.CueTemplate{
			{
				Text: "{{if eq .Status \"win\"}}" +
					"Все мои корабли убиты. Вы выиграли!" +
					"{{else if eq .Status \"lose\"}}" +
					"Ура! Я победила!" +
					"{{else if eq .Status \"cheating\"}}" +
					"К сожалению, вы проиграли." +
					"{{else}}" +
					"К сожалению, вы сдались." +
					"{{end}}" +
					" Хотите сыграть ещё раз?",
				Voice: "{{if eq .Status \"win\"}}" +
					"Все мои корабли убиты. Вы выиграли!" +
					"{{else if eq .Status \"lose\"}}" +
					"Ура! Я победила!" +
					"{{else if eq .Status \"cheating\"}}" +
					"К сожалению, вы проиграли." +
					"{{else}}" +
					"К сожалению, вы сдались." +
					"{{end}}" +
					" Хотите сыграть ещё раз?",
			},
		},
		WinReply: []dialoglib.CueTemplate{
			{
				Text:  "Все мои корабли убиты. Вы выиграли!",
				Voice: "Все мои корабли убиты. Вы выиграли!",
			},
		},
		LoseReply: []dialoglib.CueTemplate{
			{
				Text:  "Ура! Я победила!",
				Voice: "Ура! Я победила!",
			},
		},
		EndCheatingStateReply: []dialoglib.CueTemplate{
			{
				Text:  "К сожалению, вы проиграли.",
				Voice: "К сожалению, вы проиграли.",
			},
		},
		SurrenderReply: []dialoglib.CueTemplate{
			{
				Text:  "К сожалению, вы сдались.",
				Voice: "К сожалению, вы сдались.",
			},
			{
				Text:  "Как жаль, что вы сдались.",
				Voice: "Как жаль, что вы сдались.",
			},
		},
		RestartReply: []dialoglib.CueTemplate{
			{
				Text:  "Хотите сыграть ещё раз?",
				Voice: "Хотите сыграть ещё раз?",
			},
		},
	},

	EndGameState: {
		EndReply: []dialoglib.CueTemplate{
			{
				Text:  "Если захотите сыграть ещё раз, просто скажите \"Алиса, давай сыграем в морской бой\".",
				Voice: "Если захотите сыграть ещё раз, просто скажите \"Алиса, давай сыграем в морской бой\".",
			},
		},
	},
}
