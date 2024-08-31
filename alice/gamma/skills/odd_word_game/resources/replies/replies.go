package replies

import (
	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
)

const (
	GeneralState   = "general"
	StartGameState = "start"
	QuestionState  = "question"
	EndGameState   = "end"
)

const (
	StartGameReply = "start_game"
	FallbackReply  = "fallback"

	YesStartReply = "yes"
	NoStartReply  = "dontStart"
	ContinueReply = "continue"

	FirstQuestionReply         = "first_quest"
	DefaultQuestionReply       = "def_quest"
	CurrentQuestionReply       = "question"
	CheatAnswerReply           = "more1"
	InvalidAnswerReply         = "invalid"
	UserDoesntKnowReply        = "dont_know"
	UserWantsToThinkReply      = "want_think"
	UserDoesntWantToThinkReply = "dont_want_think"
	RepeatReply                = "repeat"
	MisUnderstandingReply      = "misunderstand"
	MeaningOfTheWordReply      = "meaning"
	SuccessfulHintReply        = "ok_hint"
	UnsuccessfulHintReply      = "bad_hint"
	SuccessfulLevelUpReply     = "ok_up"
	UnsuccessfulLevelUpReply   = "bad_up"
	SuccessfulLevelDownReply   = "ok_level_down"
	UnsuccessfulLevelDownReply = "bad_level_down"
	WinningReply               = "win"
	SuccessExplanationReply    = "answer"
	RightAnswerReply           = "ok_answer"
	WrongAnswerReply           = "wrong_answer"
	LoseExplanationReply       = "false_answer"
	IncorrectWordNumberReply   = "more4"
	GuessOddWordReply          = "guess_odd"

	EndGameReply = "end_game"
)

var CueTemplates = map[string]map[string][]dialoglib.CueTemplate{
	GeneralState: {
		StartGameReply: []dialoglib.CueTemplate{
			{
				Text: "Сыграем в детскую игру «Найди лишнее»! " +
					"Я буду называть слова, а вы попробуйте угадать, какое среди них лишнее." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}}\n" +
					"Если вы готовы, скажите «да». Давайте начнём?",
				Voice: "Сыграем в детскую игру «Найди лишнее»! " +
					"Я буду называть слова, а вы попробуйте угадать, какое среди них лишнее." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}}\n" +
					"Если вы готовы, скажите «да». Давайте начнём?",
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
		YesStartReply: []dialoglib.CueTemplate{
			{
				Text:  "Отлично!",
				Voice: "Отлично!",
			},
		},
		NoStartReply: []dialoglib.CueTemplate{
			{
				Text:  "Очень жаль.",
				Voice: "Очень жаль.",
			},
			{
				Text:  "Ладно, в другой раз.",
				Voice: "Ладно, в другой раз.",
			},
			{
				Text:  "Ну нет так нет.",
				Voice: "Ну нет - так нет.",
			},
		},
		ContinueReply: []dialoglib.CueTemplate{
			{
				Text:  "Отлично! Продолжаем!",
				Voice: "Отлично! Продолжаем!",
			},
		},
	},
	QuestionState: {
		FirstQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "Итак, назовите лишнее слово:",
				Voice: "Итак, назовите лишнее слово:",
			},
		},
		DefaultQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "Следующее:",
				Voice: "Следующее:",
			},
		},
		CurrentQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "{{range $i, $word := .Words}}{{if ne $i 0}} - {{end}}{{$word.Word.Text}}{{end}}.",
				Voice: "{{range $i, $word := .Words}}{{if ne $i 0}} - {{end}}{{$word.Word.Voice}}{{end}}",
			},
		},
		CheatAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Так не пойдёт! Нужно назвать одно слово.",
				Voice: "Так не пойдёт! Нужно назвать одно слово.",
			},
		},
		InvalidAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Пожалуйста, назовите слово из списка:",
				Voice: "Пожалуйста, назовите слово из списка:",
			},
			{
				Text:  "Пожалуйста, выберите слово из списка:",
				Voice: "Пожалуйста, выберите слово из списка:",
			},
		},
		UserDoesntKnowReply: []dialoglib.CueTemplate{
			{
				Text:  "Может, подумаете ещё немного?",
				Voice: "Может, подумаете ещё немного?",
			},
		},
		UserWantsToThinkReply: []dialoglib.CueTemplate{
			{
				Text:  "Замечательно! Итак, найдите здесь лишнее слово:",
				Voice: "Замечательно! Итак, найдите здесь лишнее слово:",
			},
		},
		UserDoesntWantToThinkReply: []dialoglib.CueTemplate{
			{
				Text:  "Ладно, можете не отвечать.",
				Voice: "Ладно, можете не отвечать.",
			},
			{
				Text:  "Хорошо, пропустим этот вопрос.",
				Voice: "Хорошо, пропустим этот вопрос.",
			},
			{
				Text:  "Так и быть, скажу вам правильный ответ.",
				Voice: "Так и быть, скажу вам правильный ответ.",
			},
		},
		RepeatReply: []dialoglib.CueTemplate{
			{
				Text:  "Повторяю:",
				Voice: "Повторяю:",
			},
		},
		MisUnderstandingReply: []dialoglib.CueTemplate{
			{
				Text:  "Попробуйте сказать ещё раз, я не очень поняла ваш ответ.",
				Voice: "Попробуйте сказать ещё раз, я не очень поняла ваш ответ.",
			},
			{
				Text:  "Простите, но я вас не расслышала. Повторите ваш ответ ещё раз.",
				Voice: "Простите, но я вас не расслышала. Повторите ваш ответ ещё раз.",
			},
			{
				Text:  "Кажется, я вас не понимаю. Попробуйте сказать ещё раз.",
				Voice: "Кажется, я вас не понимаю. Попробуйте сказать ещё раз.",
			},
		},
		MeaningOfTheWordReply: []dialoglib.CueTemplate{
			{
				Text:  "{{ .Text }} - это {{ .Type }}.",
				Voice: "{{ .Voice }} - это {{ .Type }}.",
			},
		},
		SuccessfulHintReply: []dialoglib.CueTemplate{
			{
				Text:  "Большинство из перечисленного относится к классу {{ .Type }}. Что к нему не относится?",
				Voice: "Большинство из перечисленного относится к классу {{ .Type }}. Что к нему не относится?",
			},
		},
		UnsuccessfulHintReply: []dialoglib.CueTemplate{
			{
				Text:  "Не знаю, как вам ещё подсказать. Подумайте и попытайтесь предположить ответ.",
				Voice: "Не знаю, как вам ещё подсказать. Подумайте и попытайтесь предположить ответ.",
			},
		},
		SuccessfulLevelUpReply: []dialoglib.CueTemplate{
			{
				Text:  "Ну, если вы так уверены в себе, попробуем что-нибудь посложнее.",
				Voice: "Ну, если вы так уверены в себе, попробуем что-нибудь посложнее.",
			},
			{
				Text:  "Ладно, предложу вам вопрос посложнее.",
				Voice: "Ладно, предложу вам вопрос посложнее.",
			},
			{
				Text:  "Хорошо, давайте попробуем кое-что посложнее.",
				Voice: "Хорошо, давайте попробуем кое-что посложнее.",
			},
		},
		UnsuccessfulLevelUpReply: []dialoglib.CueTemplate{
			{
				Text:  "Сложнее уже некуда. Ответите на вопросы этого уровня — и вы выиграли! Итак, какое слово лишнее:",
				Voice: "Сложнее уже некуда. Ответите на вопросы этого уровня — и вы выиграли! Итак, какое слово лишнее:",
			},
		},
		SuccessfulLevelDownReply: []dialoglib.CueTemplate{
			{
				Text:  "Хорошо, попробую задать вопрос полегче.",
				Voice: "Хорошо, попробую задать вопрос полегче.",
			},
			{
				Text:  "Ладно, предложу вам вопрос полегче.",
				Voice: "Ладно, предложу вам вопрос полегче.",
			},
			{
				Text:  "У меня есть и более простые вопросы.",
				Voice: "У меня есть и более простые вопросы.",
			},
		},
		UnsuccessfulLevelDownReply: []dialoglib.CueTemplate{
			{
				Text:  "Не могу придумать ничего проще! Соберитесь и попробуйте определить, какое слово здесь лишнее:",
				Voice: "Не могу придумать ничего проще! Соберитесь и попробуйте определить, какое слово здесь лишнее:",
			},
		},
		RightAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Верно!",
				Voice: "Верно!",
			},
			{
				Text:  "Конечно!",
				Voice: "Конечно!",
			},
			{
				Text:  "Абсолютно точно!",
				Voice: "Абсолютно точно!",
			},
			{
				Text:  "Это правильно.",
				Voice: "Это правильно.",
			},
		},
		WinningReply: []dialoglib.CueTemplate{
			{
				Text: "Замечательно! Этот раунд вы выиграли — ответили даже на самые сложные вопросы. Ваш результат: " +
					"{{ .CountOfRightAnswers }} {{ .Answer }} из {{ .CountOfQuestions }}. Хотите сыграть еще?",
				Voice: "Замечательно! Этот раунд вы выиграли — ответили даже на самые сложные вопросы. Ваш результат: " +
					"{{ .CountOfRightAnswers }} {{ .Answer }} из {{ .CountOfQuestions }}. Хотите сыграть еще?",
			},
		},
		SuccessExplanationReply: []dialoglib.CueTemplate{
			{
				Text:  "{{ .Text }} - это {{ .CorrectType }}, а остальное - это {{ .OtherType }}.",
				Voice: "{{ .Voice }} - это {{ .CorrectType }}, а остальное - это {{ .OtherType }}.",
			},
		},
		WrongAnswerReply: []dialoglib.CueTemplate{
			{
				Text:  "Не совсем так!",
				Voice: "Не совсем так!",
			},
		},
		LoseExplanationReply: []dialoglib.CueTemplate{
			{
				Text: "Правильный ответ - это {{ .OddWord.Text }}. Потому что {{ .OddWord.Text }} - это {{ .CorrectType }}," +
					" а остальное - это {{ .OtherType }}.",
				Voice: "Правильный ответ - это {{ .OddWord.Voice }}. Потому что {{ .OddWord.Text }} - это {{ .CorrectType }}," +
					" а остальное - это {{ .OtherType }}.",
			},
			{
				Text:  "Правильный ответ - это {{ .OddWord.Text }}. Остальное - это {{ .OtherType }}.",
				Voice: "Правильный ответ - это {{ .OddWord.Voice }}. Остальное - это {{ .OtherType }}.",
			},
		},
		IncorrectWordNumberReply: []dialoglib.CueTemplate{
			{
				Text:  "Там было всего 4 варианта ответа.",
				Voice: "Там было всего 4 варианта ответа.",
			},
		},
		GuessOddWordReply: []dialoglib.CueTemplate{
			{
				Text:  "Итак, какое слово лишнее:",
				Voice: "Итак, какое слово лишнее:",
			},
		},
	},
	EndGameState: {
		EndGameReply: []dialoglib.CueTemplate{
			{
				Text: "Хорошо, заканчиваем игру! Ваш результат: {{ .CountOfRightAnswers }} {{ .Answer }} из {{ .CountOfQuestions }}. " +
					"Если захотите ещё раз поиграть в «Найди лишнее», просто скажите «Включи 'Найди лишнее'».",
				Voice: "Хорошо, заканчиваем игру! Ваш результат: {{ .CountOfRightAnswers }} {{ .Answer }} из {{ .CountOfQuestions }}. " +
					"Если захотите ещё раз поиграть в «Найди лишнее», просто скажите «Включи 'Найди лишнее'».",
			},
		},
	},
}
