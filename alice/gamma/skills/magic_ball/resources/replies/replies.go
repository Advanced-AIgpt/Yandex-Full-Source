package replies

import (
	"encoding/json"
	"golang.org/x/xerrors"

	dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/library/go/core/resource"
)

const (
	GeneralState  = "general"
	QuestionState = "question"
	SelectState   = "select"
	EndState      = "end"
)

const (
	StartReply    = "start"
	FallbackReply = "fallback"

	QuestionReply = "question"

	RulesReply         = "rules"
	PredictionReply    = "prediction"
	NotQuestionReply   = "not_question"
	UserNotReadyReply  = "not_ready"
	RareReply          = "rare"
	LessRareReply      = "less_rare"
	AskToContinueReply = "continue"

	EndReply = "end"
)

func GetReplies() (replies []dialoglib.Cue, err error) {
	if err = json.Unmarshal(resource.Get("replies.json"), &replies); err != nil {
		return nil, xerrors.Errorf("Resource is an invalid json resource")
	}
	return
}

func GetRareFacts() []string {
	return []string{
		"Знаете, про магический шар даже сняли фильм — «Трасса 60». Но там говорится: «На все ваши ответы будут заданы вопросы».",
		"Хотите — верьте, хотите — нет. Даже доктор Хаус пользовался магическим шаром, чтобы поставить диагноз, но, насколько я поняла, это была шутка.",
		"Вы, конечно, как хотите, но я бы не стала полагаться на советы шара всегда и во всём.",
		"Не верьте всему, что говорит шар. Он как-то раз всё правильно предсказал Барту Симпсону, но это было в мультфильме.",
		"Росс из сериала «Друзья» не знал, как ему поступить, и обратился за помощью к магическому шару. Но я бы не верила всему, что говорит шар.",
	}
}

var CueTemplates = map[string]map[string][]dialoglib.CueTemplate{
	GeneralState: {
		StartReply: []dialoglib.CueTemplate{
			{
				Text: "Вы задаете вопрос, на который можно ответить «да» или «нет», а шар дает ответ." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}}",
				Voice: "Вы задаете вопрос, на который можно ответить «да» или «нет», а шар дает ответ." +
					"{{if .IsStation }} Если надоест, скажите: «Алиса, хватит».{{end}}",
			},
		},
		FallbackReply: []dialoglib.CueTemplate{
			{
				Text:  "Извините, я вас не поняла.",
				Voice: "Извините, я вас не поняла.",
			},
		},
	},
	SelectState: {
		QuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "Жду вашего вопроса.",
				Voice: "Жду вашего вопроса.",
			},
			{
				Text:  "Спрашивайте.",
				Voice: "Спрашивайте.",
			},
		},
	},
	QuestionState: {
		RulesReply: []dialoglib.CueTemplate{
			{
				Text: "Вот правила игры: вы задаёте любой вопрос, на который можно ответить «да», «нет» или «может быть»," +
					" а шар вам отвечает. " +
					"{{if .IsStation }}Если надоест, скажите: «Алиса, хватит». {{end}}" +
					"Играем?",
				Voice: "Вот правила игры: вы задаёте любой вопрос, на который можно ответить да - нет - или может быть," +
					" а шар вам отвечает. " +
					"{{if .IsStation }}Если надоест, скажите: «Алиса, хватит». {{end}}" +
					"Играем?",
			},
		},
		PredictionReply: []dialoglib.CueTemplate{
			{
				Text:  "{{.Prediction.Text}}",
				Voice: "{{.Prediction.Voice}}",
			},
		},
		NotQuestionReply: []dialoglib.CueTemplate{
			{
				Text:  "Я жду от вас вопроса - в этом и суть игры!",
				Voice: "Я жду от вас вопроса --- в этом и суть игр+ы!",
			},
			{
				Text:  "Вы же хотели поиграть. Я жду от вас вопроса.",
				Voice: "Вы же хотели поиграть. Я жду от вас вопроса.",
			},
		},
		UserNotReadyReply: []dialoglib.CueTemplate{
			{
				Text:  "Ок. Я подожду.",
				Voice: "Ок. Я подожду.",
			},
		},
		RareReply: []dialoglib.CueTemplate{
			{
				Text:  "{{.Fact}}",
				Voice: "{{.Fact}}",
			},
		},
		LessRareReply: []dialoglib.CueTemplate{
			{
				Text:  "Только не перекладывайте всю ответственность за принятие ваших решений на шар.",
				Voice: "Только не перекладывайте всю ответственность за принятие ваших решений на шар.",
			},
			{
				Text:  "Знаете, шар — это, конечно, хорошо, но он знает не всё.",
				Voice: "Знаете, шар — это, конечно, хорошо, но он знает не всё.",
			},
			{
				Text:  "Но я бы не доверяла шару судьбы на 100%.",
				Voice: "Но я бы не доверяла шару судьбы на 100%.",
			},
		},
		AskToContinueReply: []dialoglib.CueTemplate{
			{
				Text:  "Хотите спросить что-нибудь ещё?",
				Voice: "Хотите спросить что-нибудь ещё?",
			},
			{
				Text:  "Ещё вопрос?",
				Voice: "Ещё вопрос?",
			},
		},
	},
	EndState: {
		EndReply: []dialoglib.CueTemplate{
			{
				Text:  "Если захотите ещё раз посоветоваться со мной, просто скажите «Активируй Магический шар».",
				Voice: "Если захотите ещё раз посоветоваться со мной, просто скажите «Активируй Магический шар».",
			},
		},
	},
}
