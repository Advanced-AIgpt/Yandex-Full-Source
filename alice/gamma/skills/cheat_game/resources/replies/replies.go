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
	StartGameReply = "Start"
	FallbackReply  = "Fallback"

	YesStartGameReply = "Yes"
	NoStartGameReply  = "No"

	NextQuestionReply     = "NextQuestion"
	QuestionFallbackReply = "QuestionFallback"
	AnotherGameReply      = "AnotherGame"
	RestartGameReply      = "RestartGame"

	EndGameReply = "EndGame"

	badAnswers       = "bad"
	fineAnswers      = "fine"
	goodAnswers      = "good"
	excellentAnswers = "excellent"
)

var CueTemplates = map[string]map[string][]dialoglib.CueTemplate{
	GeneralState: {
		StartGameReply: []dialoglib.CueTemplate{
			{
				Text: "Конечно! Угадайте, правду я говорю или нет. В раунде пять вопросов." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}} Начинаем играть?",
				Voice: "Конечно! Угадайте, правду я говорю или нет. В раунде пять вопросов." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}} Начинаем играть?",
			},
			{
				Text: "Давайте. Вам просто нужно сказать, верите вы тому, что я говорю, или нет." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}} Играем?",
				Voice: "Давайте. Вам просто нужно сказать, верите вы тому, что я говорю, или нет." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}} Играем?",
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
		YesStartGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Отлично. Начинаем!",
				Voice: "Отлично. Начинаем!",
			},
			{
				Text:  "Прекрасно. Поехали!",
				Voice: "Прекрасно. Поехали!",
			},
			{
				Text:  "Класс. Давайте начнём.",
				Voice: "Класс. Давайте начнём.",
			},
			{
				Text:  "Отлично. Давайте начнём.",
				Voice: "Отлично. Давайте начнём.",
			},
			{
				Text:  "Прекрасно. Начинаем!",
				Voice: "Прекрасно. Начинаем!",
			},
			{
				Text:  "Класс. Поехали!",
				Voice: "Класс. Поехали!",
			},
		},
		NoStartGameReply: []dialoglib.CueTemplate{
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
	},
	QuestionState: {
		NextQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "Поехали дальше.",
				Voice: "Поехали дальше.",
			},
			{
				Text:  "Следующий вопрос.",
				Voice: "Следующий вопрос.",
			},
			{
				Text:  "Итак, не останавливаемся.",
				Voice: "Итак, не останавливаемся.",
			},
			{
				Text:  "У меня тогда другой вопрос.",
				Voice: "У меня тогда другой вопрос.",
			},
			{
				Text:  "Переходим к другому факту.",
				Voice: "Переходим к другому факту.",
			},
		},
		QuestionFallbackReply: []dialoglib.CueTemplate{
			{
				Text:  "Я вас не поняла. Просто скажите, верите вы мне или нет.",
				Voice: "Я вас не поняла. Просто скажите, верите вы мне или нет.",
			},
			{
				Text:  "Не понимаю вас. Просто скажите, верите вы мне или нет.",
				Voice: "Не понимаю вас. Просто скажите, верите вы мне или нет.",
			},
			{
				Text:  "Вам просто нужно сказать, верите ли вы тому, что я говорю, или нет.",
				Voice: "Вам просто нужно сказать, верите ли вы тому, что я говорю, или нет.",
			},
			{
				Text:  "Скажите, верите ли вы тому, что я говорю, или нет.",
				Voice: "Скажите, верите ли вы тому, что я говорю, или нет.",
			},
		},
		AnotherGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Давайте ещё?",
				Voice: "Давайте ещё?",
			},
			{
				Text:  "Сыграем ещё?",
				Voice: "Сыграем ещё?",
			},
			{
				Text:  "Хотите сыграть ещё раунд?",
				Voice: "Хотите сыграть ещё раунд?",
			},
			{
				Text:  "Может, сыграем ещё раз?",
				Voice: "Может, сыграем ещё раз?",
			},
			{
				Text:  "Потренируетесь ещё?",
				Voice: "Потренируетесь ещё?",
			},
			{
				Text:  "Играем дальше?",
				Voice: "Играем дальше?",
			},
		},
		RestartGameReply: []dialoglib.CueTemplate{
			{
				Text:  "У меня закончились вопросы и я перезапускаю игру. Ещё раунд?",
				Voice: "У меня закончились вопросы и я перезапускаю игру. Ещё раунд?",
			},
		},
		badAnswers: []dialoglib.CueTemplate{
			{
				Text:  "Ну как вам сказать. Кажется, {{.correct}} из {{.asked}}.",
				Voice: "Ну как вам сказать. Кажется, {{.correct}} из {{.asked}}.",
			},
			{
				Text:  "Вообще-то, у вас {{.correct}} из {{.asked}}.",
				Voice: "Вообще-то, у вас {{.correct}} из {{.asked}}.",
			},
			{
				Text:  "Вы только не расстраивайтесь и не удаляйте меня, но у вас {{.correct}} из {{.asked}}.",
				Voice: "Вы только не расстраивайтесь и не удаляйте меня, но у вас {{.correct}} из {{.asked}}.",
			},
		},
		fineAnswers: []dialoglib.CueTemplate{
			{
				Text:  "{{.correct}} из {{.asked}}. Неплохо. Но ещё есть к чему стремиться!",
				Voice: "{{.correct}} из {{.asked}}. Неплохо. Но ещё есть к чему стремиться!",
			},
			{
				Text:  "У вас {{.correct}} из {{.asked}}.",
				Voice: "У вас {{.correct}} из {{.asked}}.",
			},
		},
		goodAnswers: []dialoglib.CueTemplate{
			{
				Text:  "{{.correct}} из {{.asked}}.",
				Voice: "{{.correct}} из {{.asked}}.",
			},
			{
				Text:  "Здорово, у вас {{.correct}} из {{.asked}}.",
				Voice: "Здорово, у вас {{.correct}} из {{.asked}}.",
			},
			{
				Text:  "У вас {{.correct}} из {{.asked}}.",
				Voice: "У вас {{.correct}} из {{.asked}}.",
			},
		},
		excellentAnswers: []dialoglib.CueTemplate{
			{
				Text:  "Идеальный результат! {{.correct}} из {{.asked}}.",
				Voice: "Идеальный результат! {{.correct}} из {{.asked}}.",
			},
			{
				Text:  "{{.correct}} из {{.asked}}. Преклоняюсь перед вашей эрудицией.",
				Voice: "{{.correct}} из {{.asked}}. Преклоняюсь перед вашей эрудицией.",
			},
			{
				Text:  "Даже не верится. Ни одной ошибки. {{.correct}} из {{.asked}}.",
				Voice: "Даже не верится. Ни одной ошибки. {{.correct}} из {{.asked}}.",
			},
		},
	},
	EndGameState: {
		EndGameReply: []dialoglib.CueTemplate{
			{
				Text:  "Захотите поиграть ещё - скажите Алиса, давай поиграем в верю - не верю. ",
				Voice: "Захотите поиграть ещё - скажите Алиса, давай поиграем в верю - не верю. ",
			},
		},
	},
}

type Manager struct {
	dialoglib.RepliesManager
}

func (manager *Manager) ChooseRoundResultsCueTemplate(correct int) *dialoglib.CueTemplate {
	switch correct {
	case 0:
		return manager.ChooseCueTemplate(QuestionState, badAnswers)
	case 1:
		fallthrough
	case 2:
		return manager.ChooseCueTemplate(QuestionState, fineAnswers)
	case 3:
		fallthrough
	case 4:
		return manager.ChooseCueTemplate(QuestionState, goodAnswers)
	case 5:
		return manager.ChooseCueTemplate(QuestionState, excellentAnswers)
	default:
		return manager.ChooseCueTemplate(QuestionState, fineAnswers)
	}
}
