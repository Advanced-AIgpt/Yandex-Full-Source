package game

import (
	"testing"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/magic_ball/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/magic_ball/resources/replies"
)

func testRequest(t *testing.T, ctx *Context, request *sdk.Request, expectedState State, expectedResponse *sdk.Response) {
	game := NewMagicBallGame(sdkTest.CreateRandMock(seed))
	response, err := game.Handle(logger, ctx, request, sdkTest.DefaultMeta())
	assert.NoError(t, err)
	assert.Equal(t, expectedState, ctx.State)
	assert.Equal(t, expectedResponse, response)
}

func TestGame_StateGeneral(t *testing.T) {
	t.Run("no fallback", func(t *testing.T) {
		request := &sdk.Request{
			Command: "junk command",
		}
		_, ctx := createTestContext(true, State{})

		expectedState := State{
			GameState:       replies.QuestionState,
			RareFacts:       replies.GetRareFacts(),
			LastNotUsedFact: 4,
		}
		expectedResponse := &sdk.Response{
			Text: "Вы задаете вопрос, на который можно ответить «да» или «нет», а шар дает ответ.\n" +
				"Жду вашего вопроса.",
			Tts: "Вы задаете вопрос, на который можно ответить «да» или «нет», а шар дает ответ.\n" +
				"Жду вашего вопроса.",
			Buttons: []sdk.Button{
				{
					Title: "Правила",
					Hide:  true,
				},
			},
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_StateQuestion(t *testing.T) {
	t.Run("not question", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NotQuestionIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.QuestionState,
		})

		expectedState := State{
			GameState: replies.QuestionState,
		}
		expectedResponse := &sdk.Response{
			Text: "Я жду от вас вопроса - в этом и суть игры!",
			Tts:  "Я жду от вас вопроса --- в этом и суть игр+ы!",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("not ready", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NotReadyIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.QuestionState,
		})

		expectedState := State{
			GameState: replies.QuestionState,
		}
		expectedResponse := &sdk.Response{
			Text: "Ок. Я подожду.",
			Tts:  "Ок. Я подожду.",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("question", func(t *testing.T) {
		request := &sdk.Request{
			Command: "it's a question",
		}
		_, ctx := createTestContext(false, State{
			GameState:       replies.QuestionState,
			RareFacts:       replies.GetRareFacts(),
			LastNotUsedFact: 4,
		})

		expectedState := State{
			GameState: replies.SelectState,
			RareFacts: []string{
				"Росс из сериала «Друзья» не знал, как ему поступить, и обратился за помощью к магическому шару. Но я бы не верила всему, что говорит шар.",
				"Хотите — верьте, хотите — нет. Даже доктор Хаус пользовался магическим шаром, чтобы поставить диагноз, но, насколько я поняла, это была шутка.",
				"Вы, конечно, как хотите, но я бы не стала полагаться на советы шара всегда и во всём.",
				"Не верьте всему, что говорит шар. Он как-то раз всё правильно предсказал Барту Симпсону, но это было в мультфильме.",
				"Знаете, про магический шар даже сняли фильм — «Трасса 60». Но там говорится: «На все ваши ответы будут заданы вопросы».",
			},
			LastNotUsedFact: 3,
		}
		expectedResponse := &sdk.Response{
			Text: "Бесспорно.\n" +
				"Знаете, про магический шар даже сняли фильм — «Трасса 60». Но там говорится: «На все ваши ответы будут заданы вопросы».\n" +
				"Хотите спросить что-нибудь ещё?",
			Tts: "Бесспорно.\n" +
				"Знаете, про магический шар даже сняли фильм — «Трасса 60». Но там говорится: «На все ваши ответы будут заданы вопросы».\n" +
				"Хотите спросить что-нибудь ещё?",
			Buttons: []sdk.Button{
				{
					Title: "Да",
					Hide:  true,
				},
				{
					Title: "Правила",
					Hide:  true,
				},
			},
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_StateSelect(t *testing.T) {
	t.Run("yes", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.YesIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.SelectState,
		})

		expectedState := State{
			GameState: replies.QuestionState,
		}
		expectedResponse := &sdk.Response{
			Text: "Жду вашего вопроса.",
			Tts:  "Жду вашего вопроса.",
			Buttons: []sdk.Button{
				{
					Title: "Правила",
					Hide:  true,
				},
			},
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("question", func(t *testing.T) {
		request := &sdk.Request{
			Command: "junk command",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.SelectState,
			RareFacts: []string{
				"Знаете, про магический шар даже сняли фильм — «Трасса 60». Но там говорится: «На все ваши ответы будут заданы вопросы».",
				"Хотите — верьте, хотите — нет. Даже доктор Хаус пользовался магическим шаром, чтобы поставить диагноз, но, насколько я поняла, это была шутка.",
				"Вы, конечно, как хотите, но я бы не стала полагаться на советы шара всегда и во всём.",
				"Не верьте всему, что говорит шар. Он как-то раз всё правильно предсказал Барту Симпсону, но это было в мультфильме.",
				"Росс из сериала «Друзья» не знал, как ему поступить, и обратился за помощью к магическому шару. Но я бы не верила всему, что говорит шар.",
			},
			LastNotUsedFact: 4,
		})

		expectedState := State{
			GameState: replies.SelectState,
			RareFacts: []string{
				"Росс из сериала «Друзья» не знал, как ему поступить, и обратился за помощью к магическому шару. Но я бы не верила всему, что говорит шар.",
				"Хотите — верьте, хотите — нет. Даже доктор Хаус пользовался магическим шаром, чтобы поставить диагноз, но, насколько я поняла, это была шутка.",
				"Вы, конечно, как хотите, но я бы не стала полагаться на советы шара всегда и во всём.",
				"Не верьте всему, что говорит шар. Он как-то раз всё правильно предсказал Барту Симпсону, но это было в мультфильме.",
				"Знаете, про магический шар даже сняли фильм — «Трасса 60». Но там говорится: «На все ваши ответы будут заданы вопросы».",
			},
			LastNotUsedFact: 3,
		}
		expectedResponse := &sdk.Response{
			Text: "Бесспорно.\n" +
				"Знаете, про магический шар даже сняли фильм — «Трасса 60». Но там говорится: «На все ваши ответы будут заданы вопросы».\n" +
				"Хотите спросить что-нибудь ещё?",
			Tts: "Бесспорно.\n" +
				"Знаете, про магический шар даже сняли фильм — «Трасса 60». Но там говорится: «На все ваши ответы будут заданы вопросы».\n" +
				"Хотите спросить что-нибудь ещё?",
			Buttons: []sdk.Button{
				{
					Title: "Да",
					Hide:  true,
				},
				{
					Title: "Правила",
					Hide:  true,
				},
			},
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_MatchGlobalCommands(t *testing.T) {
	t.Run("start game", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.StartIntent,
		}
		_, ctx := createTestContext(false, State{})

		expectedState := State{
			GameState: replies.QuestionState,
		}
		expectedResponse := &sdk.Response{
			Text: "Вы задаете вопрос, на который можно ответить «да» или «нет», а шар дает ответ.\n" +
				"Жду вашего вопроса.",
			Tts: "Вы задаете вопрос, на который можно ответить «да» или «нет», а шар дает ответ.\n" +
				"Жду вашего вопроса.",
			Buttons: []sdk.Button{
				{
					Title: "Правила",
					Hide:  true,
				},
			},
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("rules", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.RulesIntent,
		}
		_, ctx := createTestContext(false, State{})

		expectedState := State{
			GameState:      replies.SelectState,
			UserWantsRules: true,
		}
		expectedResponse := &sdk.Response{
			Text: "Вот правила игры: вы задаёте любой вопрос, на который можно ответить «да», «нет» или «может быть»," +
				" а шар вам отвечает. Играем?",
			Tts: "Вот правила игры: вы задаёте любой вопрос, на который можно ответить да - нет - или может быть," +
				" а шар вам отвечает. Играем?",
			Buttons: []sdk.Button{
				{
					Title: "Да",
					Hide:  true,
				},
			},
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("end game", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.EndIntent,
		}
		_, ctx := createTestContext(false, State{})

		expectedState := State{
			GameState: replies.EndState,
		}
		expectedResponse := &sdk.Response{
			Text:       "Если захотите ещё раз посоветоваться со мной, просто скажите «Активируй Магический шар».",
			Tts:        "Если захотите ещё раз посоветоваться со мной, просто скажите «Активируй Магический шар».",
			EndSession: true,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("fallback", func(t *testing.T) {
		request := &sdk.Request{
			Command: "junk command",
		}
		_, ctx := createTestContext(false, State{})

		expectedState := State{}
		expectedResponse := &sdk.Response{
			Text: "Извините, я вас не поняла.",
			Tts:  "Извините, я вас не поняла.",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}
