package replies

import (
	dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
)

const (
	GeneralState   = "general"
	StartGameState = "start"
	RulesState     = "select"
	QuestionState  = "question"
	EndGameState   = "end"
)

const (
	StartGameReply          = "start"
	RulesReply              = "rules"
	FallbackReply           = "fallback"
	RestartReply            = "restart"
	HowDoesAliceKnowReply   = "how"
	ResultReply             = "result"
	SuccessResultReply      = "success"
	LoseResultReply         = "lose"
	OneSuccessQuestionReply = "one_s"
	OneLoseQuestionReply    = "one_l"
	CantSayResultReply      = "cant_say"

	YesStartReply = "yes_start"
	NoStartReply  = "no"

	RightAnswerReply            = "ok"
	WrongAnswerReply            = "wa"
	ExplanationReply            = "explanation"
	SkipQuestionReply           = "next"
	InvalidAnswersReply         = "multiple"
	AskRightAnswerReply         = "ask"
	AskToGuessReply             = "guess"
	NotNumberAnswerReply        = "not_num"
	UndefinedAnswerReply        = "undefined"
	RepeatQuestionReply         = "repeat"
	ComfortUserReply            = "comfort"
	ComplimentUserReply         = "compliment"
	StimulateUserReply          = "stimulate"
	NextRoundReply              = "next_round"
	PartsQuestionReply          = "Parts"
	CoatingQuestionReply        = "Coating"
	EmploymentQuestionReply     = "Employment"
	ExtremitiesQuestionReply    = "Extremities"
	NumExtremitiesQuestionReply = "NUMBER"
	ClassQuestionReply          = "Class"
	SavageryQuestionReply       = "Savagery"
	PromoReply                  = "promo"
	CantUnderstandReply         = "cant"
	DontTryReply                = "dont_try"
	MisunderstandingReply       = "misunderstanding"

	EndGameReply = "end"
)

var CueTemplates = map[string]map[string][]dialoglib.CueTemplate{
	GeneralState: {
		StartGameReply: []dialoglib.CueTemplate{
			{
				Text: "Сыграем в детскую игру «Зоология»! Я буду задавать вопросы о животных, а вы попытайтесь ответить на них.\n" +
					"Если вы готовы, скажите «да»." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}}\n" +
					"Давайте начнём?",
				Voice: "Сыграем в детскую игру «Зоол+огиаа»! Я буду задавать вопросы о животных, а вы попытайтесь ответить на них.\n" +
					"Если вы готовы, скажите -- «да»." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}}\n" +
					"Давайте начнём?",
			},
		},
		RulesReply: []dialoglib.CueTemplate{
			{
				Text: "Правила игры такие: я задаю вам разные вопросы о животных и даю два варианта ответа или прошу назвать число. В раунде 5 вопросов. " +
					"Постарайтесь ответить на все вопросы!\n" +
					"Продолжим?",
				Voice: "Правила игры такие: я задаю вам разные вопросы о животных и даю два варианта ответа или прошу назвать число. В раунде 5 вопросов. " +
					"Постарайтесь ответить на все вопросы!\n" +
					"Продолжим?",
			},
		},
		FallbackReply: []dialoglib.CueTemplate{
			{
				Text:  "Извините, я вас не поняла.",
				Voice: "Извините, я вас не поняла.",
			},
		},
		RestartReply: []dialoglib.CueTemplate{
			{
				Text:  "Всегда рада.",
				Voice: "Всегда рада.",
			},
			{
				Text:  "Ура, снова играем!",
				Voice: "Ура, снова играем!",
			},
			{
				Text:  "Конечно, давайте сыграем ещё!",
				Voice: "Конечно, давайте сыграем ещё!",
			},
			{
				Text:  "Да, конечно!",
				Voice: "Да, конечно!",
			},
		},
		HowDoesAliceKnowReply: []dialoglib.CueTemplate{
			{
				Text:  "Я нахожу ответы в земном Интернете.",
				Voice: "Я нахожу ответы в земном Интернете.",
			},
			{
				Text:  "Я подглядываю в ответы в земном Интернете.",
				Voice: "Я подглядываю в ответы в земном Интернете.",
			},
		},
		ResultReply: []dialoglib.CueTemplate{
			{
				Text:  "Вы ответили правильно на {{.Count}} {{.Answers}} из {{.Questions}}!",
				Voice: "Вы ответили правильно на {{.Count}} {{.Answers}} из {{.Questions}}!",
			},
		},
		SuccessResultReply: []dialoglib.CueTemplate{
			{
				Text:  "Вы ответили правильно на все {{.Count}} {{.Answers}}!",
				Voice: "Вы ответили правильно на всиэ {{.Count}} {{.Answers}}!",
			},
		},
		LoseResultReply: []dialoglib.CueTemplate{
			{
				Text:  "Вы не ответили ни на один вопрос из {{.Questions}}...",
				Voice: "Вы не ответили ни на один вопрос из {{.Questions}}...",
			},
		},
		OneSuccessQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "Был всего 1 вопрос, и вы ответили на него правильно!",
				Voice: "Был всего 1 вопрос, и вы ответили на него правильно!",
			},
		},
		OneLoseQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "Был всего один вопрос, и вы не смогли на него ответить!",
				Voice: "Был всего один вопрос, и вы не смогли на него ответить!",
			},
		},
		CantSayResultReply: []dialoglib.CueTemplate{
			{
				Text:  "Не могу сказать.",
				Voice: "Не могу сказать.",
			},
		},
	},

	StartGameState: {
		YesStartReply: []dialoglib.CueTemplate{
			{
				Text:  "Отлично!",
				Voice: "Отлично!",
			},
		},
		NoStartReply: []dialoglib.CueTemplate{
			{
				Text:  "Ну нет так нет.",
				Voice: "Ну нет так нет.",
			},
		},
	},

	QuestionState: {
		RightAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Точно!",
				Voice: "Точно!",
			},
			{
				Text:  "Ответ правильный! Вы молодец!",
				Voice: "Ответ правильный! Вы молодец!",
			},
			{
				Text:  "Это правильно!",
				Voice: "Это правильно!",
			},
			{
				Text:  "Точно! Всё так.",
				Voice: "Точно! Всё так.",
			},
			{
				Text:  "Верно!",
				Voice: "Верно!",
			},
			{
				Text:  "Правильно!",
				Voice: "Правильно!",
			},
		},
		WrongAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "К сожалению, это неправильный ответ.",
				Voice: "К сожалению, это неправильный ответ.",
			},
			{
				Text:  "Нет, увы!",
				Voice: "Нет, увы!",
			},
			{
				Text:  "В этот раз не получилось. Ответ неправильный.",
				Voice: "В этот раз не получилось. Ответ неправильный.",
			},
			{
				Text:  "В этот раз неправильно.",
				Voice: "В этот раз неправильно.",
			},
			{
				Text:  "Неправильно.",
				Voice: "Неправильно.",
			},
		},
		ExplanationReply: []dialoglib.CueTemplate{
			{
				Text:  "Правильный ответ: {{.RightAnswer.Text}}.",
				Voice: "Правильный ответ: {{.RightAnswer.Voice}}.",
			},
		},
		SkipQuestionReply: []dialoglib.CueTemplate{
			{
				Text: "Правильный ответ: " +
					"{{if eq .QuestionType \"Parts\"}}" +
					"у {{.RightAnswer.Text}}" +
					"{{else}}" +
					"{{.RightAnswer.Text}}" +
					"{{end}}. " +
					"Давайте перейдём к следующему вопросу.",
				Voice: "Правильный ответ: " +
					"{{if eq .QuestionType \"Parts\"}}" +
					"у {{.RightAnswer.Voice}}" +
					"{{else}}" +
					"{{.RightAnswer.Voice}}" +
					"{{end}}. " +
					"Давайте перейдём к следующему вопросу.",
			},
		},
		InvalidAnswersReply: []dialoglib.CueTemplate{
			{
				Text:  "Так не пойдет, вам нужно выбрать один вариант ответа.",
				Voice: "Так не пойдет, вам нужно выбрать один вариант ответа.",
			},
			{
				Text:  "Так нельзя! Выберите, пожалуйста, один вариант.",
				Voice: "Так нельзя! Выберите, пожалуйста, один вариант.",
			},
			{
				Text:  "Это нечестно! Нужно выбрать что-то одно.",
				Voice: "Это нечестно! Нужно выбрать что-то одно.",
			},
		},
		AskRightAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Назовите правильный ответ!",
				Voice: "Назовите правильный ответ!",
			},
			{
				Text:  "А какой ответ правильный?",
				Voice: "А какой ответ правильный?",
			},
			{
				Text:  "Какой же тогда ответ верный?",
				Voice: "Какой же тогда ответ верный?",
			},
			{
				Text:  "Какой ответ тогда правильный?",
				Voice: "Какой ответ тогда правильный?",
			},
		},
		AskToGuessReply: []dialoglib.CueTemplate{
			{
				Text:  "Может, попробуете угадать?",
				Voice: "Может, попробуете угадать?",
			},
			{
				Text:  "Попробуйте угадать!",
				Voice: "Попробуйте угадать!",
			},
			{
				Text:  "Ну, как вы думаете?",
				Voice: "Ну, как вы думаете?",
			},
			{
				Text:  "Ответьте наугад!",
				Voice: "Ответьте наугад!",
			},
		},
		NotNumberAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Такой ответ не подходит! Нужно назвать число.",
				Voice: "Такой ответ не подходит! Нужно назвать число.",
			},
			{
				Text:  "Попробуйте угадать число!",
				Voice: "Попробуйте угадать число!",
			},
			{
				Text:  "Если не знаете точное число, ответьте наугад!",
				Voice: "Если не знаете точное число, ответьте наугад!",
			},
		},
		UndefinedAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Что-то я не поняла ваш ответ... Попробуете ещё раз или перейдём к следующему вопросу?",
				Voice: "Что-то я не поняла ваш ответ... Попробуете ещё раз или перейдём к следующему вопросу?",
			},
			{
				Text:  "Не понимаю... Ещё одну попытку или следующий вопрос?",
				Voice: "Не понимаю... Ещё одну попытку или следующий вопрос?",
			},
			{
				Text:  "Я не смогла понять ваш ответ. Попробуем ещё раз или следующий вопрос?",
				Voice: "Я не смогла понять ваш ответ. Попробуем ещё раз или следующий вопрос?",
			},
		},
		NextRoundReply: []dialoglib.CueTemplate{
			{
				Text:  "Давайте ещё разок?",
				Voice: "Давайте ещё разок?",
			},
			{
				Text:  "Давайте ещё?",
				Voice: "Давайте ещё?",
			},
			{
				Text:  "Играем дальше?",
				Voice: "Играем дальше?",
			},
			{
				Text:  "Хотите сыграть ещё?",
				Voice: "Хотите сыграть ещё?",
			},
			{
				Text:  "Может, сыграем ещё раз?",
				Voice: "Может, сыграем ещё раз?",
			},
		},
		RepeatQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "Хорошо, повторяю вопрос.",
				Voice: "Хорошо, повторяю вопрос.",
			},
			{
				Text:  "Давайте попробую ещё раз задать вопрос.",
				Voice: "Давайте попробую ещё раз задать вопрос.",
			},
		},
		ComfortUserReply: []dialoglib.CueTemplate{
			{
				Text:  "В следующий раз наверняка будет лучше!",
				Voice: "В следующий раз наверняка будет лучше!",
			},
			{
				Text:  "Немного тренировки — и ваш результат улучшится!",
				Voice: "Немного тренировки — и ваш результат улучшится!",
			},
		},
		ComplimentUserReply: []dialoglib.CueTemplate{
			{
				Text:  "Прекрасный результат!",
				Voice: "Прекрасный результат!",
			},
			{
				Text:  "Очень неплохо!",
				Voice: "Очень неплохо!",
			},
			{
				Text:  "Гениально!",
				Voice: "Гениально!",
			},
		},
		StimulateUserReply: []dialoglib.CueTemplate{
			{
				Text:  "Скоро вы достигнете совершенства!",
				Voice: "Скоро вы достигнете совершенства!",
			},
			{
				Text:  "Вам нужно немного потренироваться!",
				Voice: "Вам нужно немного потренироваться!",
			},
			{
				Text:  "В следующий раз будет лучше!",
				Voice: "В следующий раз будет лучше!",
			},
		},
		PartsQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "У кого есть {{ .Question.KeyWord.Text }}: у {{ .Question.Right.Text }} или у {{ .Question.Wrong.Text }}?",
				Voice: "У кого есть {{ .Question.KeyWord.Voice }}: у {{ .Question.Right.Voice }} или у {{ .Question.Wrong.Voice }}?",
			},
		},
		CoatingQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "У {{ .Question.KeyWord.Text }} {{ .Question.Right.Text}} или {{ .Question.Wrong.Text }}?",
				Voice: "У {{ .Question.KeyWord.Voice }} {{ .Question.Right.Voice}} или {{ .Question.Wrong.Voice }}?",
			},
		},
		EmploymentQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "Кто {{ .Question.KeyWord.Text }} - {{ .Question.Right.Text }} или {{ .Question.Wrong.Text }}?",
				Voice: "Кто {{ .Question.KeyWord.Voice }} - {{ .Question.Right.Voice }} или {{ .Question.Wrong.Voice }}?",
			},
		},
		ExtremitiesQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "Что есть у {{ .Question.KeyWord.Text }}: {{ .Question.Right.Text }} или {{ .Question.Wrong.Text }}?",
				Voice: "Что есть у {{ .Question.KeyWord.Voice }}: {{ .Question.Right.Voice }} или {{ .Question.Wrong.Voice }}?",
			},
		},
		NumExtremitiesQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "Сколько {{ .Question.Right.Text }} у {{ .Question.KeyWord.Text }}?",
				Voice: "Сколько {{ .Question.Right.Voice }} у {{ .Question.KeyWord.Voice }}?",
			},
		},
		ClassQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "{{ .Question.KeyWord.Text }} - это {{ .Question.Right.Text }} или {{ .Question.Wrong.Text }}?",
				Voice: "{{ .Question.KeyWord.Voice }} - это {{ .Question.Right.Voice }} или {{ .Question.Wrong.Voice }}?",
			},
		},
		SavageryQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "{{ .Question.KeyWord.Text }} - {{ .Question.Right.Text }} или {{ .Question.Wrong.Text }} животное?",
				Voice: "{{ .Question.KeyWord.Voice }} - {{ .Question.Right.Voice }} или {{ .Question.Wrong.Voice }} животное?",
			},
		},
		PromoReply: []dialoglib.CueTemplate{
			{
				Text:  "Кстати, ещё у меня есть игра «Найди лишнее». Пройдите по ссылке, чтобы попробовать.",
				Voice: "Кстати, ещё у меня есть игра «Найди лишнее». Пройдите по ссылке, чтобы попробовать.",
			},
		},
		CantUnderstandReply: []dialoglib.CueTemplate{
			{
				Text:  "Я не понимаю. Ещё попытку или следующий вопрос?",
				Voice: "Я не понимаю. Ещё попытку или следующий вопрос?",
			},
			{
				Text:  "Я не поняла, вы хотите ещё одну попытку или следующий вопрос?",
				Voice: "Я не поняла, вы хотите ещё одну попытку или следующий вопрос?",
			},
		},
		DontTryReply: []dialoglib.CueTemplate{
			{
				Text:  "Хорошо, давайте перейдём к следующему вопросу.",
				Voice: "Хорошо, давайте перейдём к следующему вопросу.",
			},
			{
				Text:  "Хорошо, пропустим этот вопрос.",
				Voice: "Хорошо, пропустим этот вопрос.",
			},
		},
		MisunderstandingReply: []dialoglib.CueTemplate{
			{
				Text:  "Опять не удалось вас понять... Давайте-ка к следующему вопросу.",
				Voice: "Опять не удалось вас понять... Давайте-ка к следующему вопросу.",
			},
		},
	},

	EndGameState: {
		EndGameReply: []dialoglib.CueTemplate{
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
	},
}
