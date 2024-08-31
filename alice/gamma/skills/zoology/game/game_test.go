package game

import (
	"testing"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/zoology/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/zoology/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/zoology/resources/replies"
)

func testRequest(t *testing.T, ctx *Context, request *sdk.Request, expectedState State, expectedResponse *sdk.Response) {
	game := NewZoologyGame(sdkTest.CreateRandMock(seed))
	response, err := game.Handle(logger, ctx, request, sdkTest.DefaultMeta())
	assert.NoError(t, err)
	assert.Equal(t, expectedState, ctx.State)
	assert.Equal(t, expectedResponse, response)
}

var testButtons = []sdk.Button{
	{
		Title: "Шерсть",
		Hide:  true,
	},
	{
		Title: "Перья",
		Hide:  true,
	},
	{
		Title: "Правила",
		Hide:  true,
	},
}

func testFirstRoundState(lastAnswerType string, questions int) RoundState {
	return RoundState{
		Questions:      questions,
		CurrentType:    "Coating",
		PreviousAnimal: 0,
		LastAnswerType: lastAnswerType,
		CurrentQuestion: &Question{
			Right: dialoglib.Cue{
				Text:  "шерсть",
				Voice: "шерсть",
			},
			Wrong: dialoglib.Cue{
				Text:  "перья",
				Voice: "перья",
			},
			KeyWord: dialoglib.Cue{
				Text:  "кошки",
				Voice: "к+ошки",
			},
			Value:   "0",
			Buttons: testButtons,
		},
	}
}

func testSecondRoundState() RoundState {
	return RoundState{
		Questions:      1,
		CurrentType:    "Parts",
		RightAnswers:   0,
		PreviousAnimal: 4,
		LastAnswerType: UncertainAnswer,
		CurrentQuestion: &Question{
			Right: dialoglib.Cue{
				Text:  "коровы",
				Voice: "кор+овы",
			},
			Value: "4",
		},
	}
}

func TestGame_StateGeneral(t *testing.T) {
	t.Run("no fallback", func(t *testing.T) {
		request := &sdk.Request{
			Command: "junk command",
		}
		_, ctx := createTestContext(true, State{})

		expectedState := State{
			GameState: replies.StartGameState,
			IsPromo:   true,
		}
		expectedResponse := &sdk.Response{
			Text: "Сыграем в детскую игру «Зоология»! Я буду задавать вопросы о животных, а вы попытайтесь ответить на них.\n" +
				"Если вы готовы, скажите «да».\n" +
				"Давайте начнём?",
			Tts: "Сыграем в детскую игру «Зоол+огиаа»! Я буду задавать вопросы о животных, а вы попытайтесь ответить на них.\n" +
				"Если вы готовы, скажите -- «да».\n" +
				"Давайте начнём?",
			Buttons: []sdk.Button{buttons.YesButton, buttons.RulesButton},
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_StateStartGame(t *testing.T) {
	t.Run("yes start", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.YesIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			RoundState: RoundState{
				PreviousAnimal: -1,
			},
		})

		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: testFirstRoundState(OkAnswer, 1),
		}
		expectedResponse := &sdk.Response{
			Text: "Отлично!\n" +
				"У кошки шерсть или перья?",
			Tts: "Отлично!\n" +
				"У к+ошки шерсть или перья?",
			Buttons: testButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("dont start", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NoIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
		})

		expectedState := State{
			GameState: replies.EndGameState,
		}
		expectedResponse := &sdk.Response{
			Text:       "Ну нет так нет.",
			Tts:        "Ну нет так нет.",
			EndSession: true,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("junk command", func(t *testing.T) {
		request := &sdk.Request{
			Command: "junk command",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
		})

		expectedState := State{
			GameState: replies.StartGameState,
		}
		expectedResponse := &sdk.Response{
			Text: "Извините, я вас не поняла.",
			Tts:  "Извините, я вас не поняла.",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_MatchGlobalCommands(t *testing.T) {
	t.Run("start game", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.StartGameIntent,
		}
		_, ctx := createTestContext(false, State{})

		expectedState := State{
			GameState: replies.StartGameState,
		}
		expectedResponse := &sdk.Response{
			Text: "Сыграем в детскую игру «Зоология»! Я буду задавать вопросы о животных, а вы попытайтесь ответить на них.\n" +
				"Если вы готовы, скажите «да».\n" +
				"Давайте начнём?",
			Tts: "Сыграем в детскую игру «Зоол+огиаа»! Я буду задавать вопросы о животных, а вы попытайтесь ответить на них.\n" +
				"Если вы готовы, скажите -- «да».\n" +
				"Давайте начнём?",
			Buttons: []sdk.Button{buttons.YesButton, buttons.RulesButton},
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("rules intent", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.RulesIntent,
		}
		_, ctx := createTestContext(false, State{})

		expectedState := State{
			GameState: replies.RulesState,
		}
		expectedResponse := &sdk.Response{
			Text: "Правила игры такие: я задаю вам разные вопросы о животных и даю два варианта ответа или прошу назвать число. В раунде 5 вопросов. " +
				"Постарайтесь ответить на все вопросы!\n" +
				"Продолжим?",
			Tts: "Правила игры такие: я задаю вам разные вопросы о животных и даю два варианта ответа или прошу назвать число. В раунде 5 вопросов. " +
				"Постарайтесь ответить на все вопросы!\n" +
				"Продолжим?",
			Buttons: []sdk.Button{buttons.YesButton},
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("restart", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.RestartGameIntent,
		}
		_, ctx := createTestContext(false, State{
			RoundState: RoundState{
				PreviousAnimal: -1,
			},
		})

		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: testFirstRoundState(OkAnswer, 1),
		}
		expectedResponse := &sdk.Response{
			Text: "Всегда рада.\n" +
				"У кошки шерсть или перья?",
			Tts: "Всегда рада.\n" +
				"У к+ошки шерсть или перья?",
			Buttons: testButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("undefined restart", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.RestartGameIntent,
		}
		_, ctx := createTestContext(false, State{
			RoundState: testFirstRoundState(UndefinedAnswer, 1),
		})

		expectedState := State{
			RoundState: testFirstRoundState(UndefinedAnswer, 1),
		}
		expectedResponse := &sdk.Response{
			Text: "Хорошо, повторяю вопрос.\n" +
				"У кошки шерсть или перья?",
			Tts: "Хорошо, повторяю вопрос.\n" +
				"У к+ошки шерсть или перья?",
			Buttons: testButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("how does alice know", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.HowDoesAliceKnowIntent,
		}
		_, ctx := createTestContext(false, State{})

		expectedState := State{}
		expectedResponse := &sdk.Response{
			Text: "Я нахожу ответы в земном Интернете.",
			Tts:  "Я нахожу ответы в земном Интернете.",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("cant say result", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ResultIntent,
		}
		_, ctx := createTestContext(false, State{})

		expectedState := State{}
		expectedResponse := &sdk.Response{
			Text: "Не могу сказать.",
			Tts:  "Не могу сказать.",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("result", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ResultIntent,
		}
		_, ctx := createTestContext(false, State{
			RoundState: RoundState{
				Questions:    4,
				RightAnswers: 3,
			},
		})

		expectedState := State{
			RoundState: RoundState{
				Questions:    4,
				RightAnswers: 3,
			},
		}
		expectedResponse := &sdk.Response{
			Text: "Вы ответили правильно на 3 вопроса из 4!",
			Tts:  "Вы ответили правильно на 3 вопроса из 4!",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("success result", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ResultIntent,
		}
		_, ctx := createTestContext(false, State{
			RoundState: RoundState{
				Questions:    4,
				RightAnswers: 4,
			},
		})

		expectedState := State{
			RoundState: RoundState{
				Questions:    4,
				RightAnswers: 4,
			},
		}
		expectedResponse := &sdk.Response{
			Text: "Вы ответили правильно на все 4 вопроса!",
			Tts:  "Вы ответили правильно на всиэ 4 вопроса!",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("lose result", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ResultIntent,
		}
		_, ctx := createTestContext(false, State{
			RoundState: RoundState{
				Questions:    4,
				RightAnswers: 0,
			},
		})

		expectedState := State{
			RoundState: RoundState{
				Questions:    4,
				RightAnswers: 0,
			},
		}
		expectedResponse := &sdk.Response{
			Text: "Вы не ответили ни на один вопрос из 4...",
			Tts:  "Вы не ответили ни на один вопрос из 4...",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("one success", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ResultIntent,
		}
		_, ctx := createTestContext(false, State{
			RoundState: RoundState{
				Questions:    1,
				RightAnswers: 1,
			},
		})

		expectedState := State{
			RoundState: RoundState{
				Questions:    1,
				RightAnswers: 1,
			},
		}
		expectedResponse := &sdk.Response{
			Text: "Был всего 1 вопрос, и вы ответили на него правильно!",
			Tts:  "Был всего 1 вопрос, и вы ответили на него правильно!",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("one lose", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ResultIntent,
		}
		_, ctx := createTestContext(false, State{
			RoundState: RoundState{
				Questions:    1,
				RightAnswers: 0,
			},
		})

		expectedState := State{
			RoundState: RoundState{
				Questions:    1,
				RightAnswers: 0,
			},
		}
		expectedResponse := &sdk.Response{
			Text: "Был всего один вопрос, и вы не смогли на него ответить!",
			Tts:  "Был всего один вопрос, и вы не смогли на него ответить!",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("end game", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.EndGameIntent,
		}
		_, ctx := createTestContext(false, State{})

		expectedState := State{}
		expectedResponse := &sdk.Response{
			Text:       "Давайте закончим!",
			Tts:        "Давайте закончим!",
			EndSession: true,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_StateRules(t *testing.T) {
	t.Run("yes continue", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.YesIntent,
		}
		_, ctx := createTestContext(false, State{
			RoundState: testFirstRoundState(OkAnswer, 1),
			GameState:  replies.RulesState,
		})

		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: testFirstRoundState(OkAnswer, 1),
		}
		expectedResponse := &sdk.Response{
			Text:    "У кошки шерсть или перья?",
			Tts:     "У к+ошки шерсть или перья?",
			Buttons: testButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("yes continue in the beginning", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.YesIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.RulesState,
			RoundState: RoundState{
				PreviousAnimal: -1,
			},
		})

		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: testFirstRoundState(OkAnswer, 1),
		}
		expectedResponse := &sdk.Response{
			Text:    "У кошки шерсть или перья?",
			Tts:     "У к+ошки шерсть или перья?",
			Buttons: testButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("no continue intent", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NoIntent,
		}
		_, ctx := createTestContext(false, State{
			RoundState: testFirstRoundState(OkAnswer, 1),
			GameState:  replies.RulesState,
		})

		expectedState := State{
			RoundState: testFirstRoundState(OkAnswer, 1),
			GameState:  replies.EndGameState,
		}
		expectedResponse := &sdk.Response{
			Text: "Давайте закончим!\n" +
				"Был всего один вопрос, и вы не смогли на него ответить!\nВ следующий раз наверняка будет лучше!",
			Tts: "Давайте закончим!\n" +
				"Был всего один вопрос, и вы не смогли на него ответить!\nВ следующий раз наверняка будет лучше!",
			EndSession: true,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("no continue in the beginning", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NoIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.RulesState,
		})

		expectedState := State{
			GameState: replies.EndGameState,
		}
		expectedResponse := &sdk.Response{
			Text: "Ну нет так нет.",
			Tts:  "Ну нет так нет.",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_SkipQuestion(t *testing.T) {
	t.Run("skip", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.UncertainAnswerIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState:  replies.QuestionState,
			RoundState: testSecondRoundState(),
		})

		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: testFirstRoundState(OkAnswer, 2),
		}
		expectedResponse := &sdk.Response{
			Text: "Правильный ответ: у коровы. Давайте перейдём к следующему вопросу.\n" +
				"У кошки шерсть или перья?",
			Tts: "Правильный ответ: у кор+овы. Давайте перейдём к следующему вопросу.\n" +
				"У к+ошки шерсть или перья?",
			Buttons: testButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_StateQuestion(t *testing.T) {
	t.Run("invalid answer", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.InvalidAnswerIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState:  replies.QuestionState,
			RoundState: testFirstRoundState(OkAnswer, 1),
		})

		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: testFirstRoundState(DontKnowAnswer, 1),
		}
		expectedResponse := &sdk.Response{
			Text: "Так не пойдет, вам нужно выбрать один вариант ответа.\n" +
				"У кошки шерсть или перья?",
			Tts: "Так не пойдет, вам нужно выбрать один вариант ответа.\n" +
				"У к+ошки шерсть или перья?",
			Buttons: testButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}
