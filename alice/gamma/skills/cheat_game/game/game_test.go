package game

import (
	"testing"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/replies"
)

func testRequest(t *testing.T, ctx *Context, request *sdk.Request, expectedState State, expectedResponse *sdk.Response) {
	game := NewCheatGame(sdkTest.CreateRandMock(seed))
	response, err := game.Handle(logger, ctx, request, sdkTest.DefaultMeta())
	assert.NoError(t, err)
	assert.Equal(t, expectedState, ctx.State)
	assert.Equal(t, expectedResponse, response)
}

func TestGame_StartGameNoFallback(t *testing.T) {
	request := &sdk.Request{
		Command: "junk command",
	}
	_, ctx := createTestContext(true, State{
		GameState: replies.GeneralState,
	})
	expectedState := State{
		GameState: replies.StartGameState,
	}
	expectedResponse := &sdk.Response{
		Text:       "Конечно! Угадайте, правду я говорю или нет. В раунде пять вопросов. Начинаем играть?",
		Tts:        "Конечно! Угадайте, правду я говорю или нет. В раунде пять вопросов. Начинаем играть?",
		Buttons:    buttons.StartGameButtons,
		EndSession: false,
	}
	testRequest(t, ctx, request, expectedState, expectedResponse)
}

func TestGame_StartGame(t *testing.T) {
	t.Run("StartGame", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.StartGameIntent,
		}
		_, ctx := createTestContext(true, State{
			GameState: replies.GeneralState,
		})
		expectedState := State{
			GameState: replies.StartGameState,
		}
		expectedResponse := &sdk.Response{
			Text:       "Конечно! Угадайте, правду я говорю или нет. В раунде пять вопросов. Начинаем играть?",
			Tts:        "Конечно! Угадайте, правду я говорю или нет. В раунде пять вопросов. Начинаем играть?",
			Buttons:    buttons.StartGameButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("StartGameYes", func(t *testing.T) {
		_, ctx := createTestContext(false, State{
			GameState:           replies.StartGameState,
			UsedQuestionIndexes: make(map[int]bool),
		})

		// Test start game yes
		request := &sdk.Request{
			Command: patterns.YesStartGameIntent,
		}

		expectedState := State{
			GameState: replies.QuestionState,
			RoundState: RoundState{
				CurQuestion:  1,
				RightAnswers: 0,
			},
			UsedQuestionIndexes: map[int]bool{
				0: true,
			},
		}
		expectedResponse := &sdk.Response{
			Text:       "Отлично. Начинаем!\nБыки ненавидят красный цвет.",
			Tts:        "Отлично. Начинаем!\nБыки ненавидят красный цвет.",
			Buttons:    buttons.QuestionButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("StartGameNo", func(t *testing.T) {
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
		})

		// Test start game and immediate end
		request := &sdk.Request{
			Command: patterns.NoStartGameIntent,
		}

		expectedState := State{
			GameState: replies.EndGameState,
		}
		expectedResponse := &sdk.Response{
			Text:       "Ну нет так нет.\nЗахотите поиграть ещё - скажите Алиса, давай поиграем в верю - не верю. ",
			Tts:        "Ну нет так нет.\nЗахотите поиграть ещё - скажите Алиса, давай поиграем в верю - не верю. ",
			EndSession: true,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

}

func TestGame_AskQuestions(t *testing.T) {
	t.Run("AskQuestion_CorrectAnswer", func(t *testing.T) {
		_, ctx := createTestContext(false, State{
			GameState: replies.QuestionState,
			UsedQuestionIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				LastQuestionIndex: 0,
				CurQuestion:       1,
				RightAnswers:      0,
			},
		})

		request := &sdk.Request{
			Command: patterns.LiesAnswerIntent,
		}

		expectedState := State{
			GameState: replies.QuestionState,
			UsedQuestionIndexes: map[int]bool{
				0: true,
				1: true,
			},
			RoundState: RoundState{
				LastQuestionIndex: 1,
				CurQuestion:       2,
				RightAnswers:      1,
			},
		}
		responseText := "Совершенно правильно не верите. " +
			"Быки — дальтоники, они не различают красный и зеленый, а реагируют на движение тряпки тореадора.\n" +
			"Поехали дальше.\n" +
			"Великую Китайскую стену видно из космоса."

		expectedResponse := &sdk.Response{
			Text:       responseText,
			Tts:        responseText,
			Buttons:    buttons.QuestionButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("AskQuestion_WrongAnswer", func(t *testing.T) {
		_, ctx := createTestContext(false, State{
			GameState: replies.QuestionState,
			UsedQuestionIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				LastQuestionIndex: 0,
				CurQuestion:       1,
				RightAnswers:      0,
			},
		})

		request := &sdk.Request{
			Command: patterns.TruthAnswerIntent,
		}

		expectedState := State{
			GameState: replies.QuestionState,
			UsedQuestionIndexes: map[int]bool{
				0: true,
				1: true,
			},
			RoundState: RoundState{
				LastQuestionIndex: 1,
				CurQuestion:       2,
				RightAnswers:      0,
			},
		}
		responseText := "А вот и нет. " +
			"Быки — дальтоники, они не различают красный и зеленый, а реагируют на движение тряпки тореадора.\n" +
			"Поехали дальше.\n" +
			"Великую Китайскую стену видно из космоса."

		expectedResponse := &sdk.Response{
			Text:       responseText,
			Tts:        responseText,
			Buttons:    buttons.QuestionButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("AskQuestion_PassAnswer", func(t *testing.T) {
		_, ctx := createTestContext(false, State{
			GameState: replies.QuestionState,
			UsedQuestionIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				LastQuestionIndex: 0,
				CurQuestion:       1,
				RightAnswers:      0,
			},
		})

		request := &sdk.Request{
			Command: patterns.PassAnswerIntent,
		}

		expectedState := State{
			GameState: replies.QuestionState,
			UsedQuestionIndexes: map[int]bool{
				0: true,
				1: true,
			},
			RoundState: RoundState{
				LastQuestionIndex: 1,
				CurQuestion:       2,
				RightAnswers:      0,
			},
		}
		responseText := "Поехали дальше.\nВеликую Китайскую стену видно из космоса."

		expectedResponse := &sdk.Response{
			Text:       responseText,
			Tts:        responseText,
			Buttons:    buttons.QuestionButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("AskQuestion_OtherAnswer", func(t *testing.T) {
		_, ctx := createTestContext(false, State{
			GameState: replies.QuestionState,
			UsedQuestionIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				LastQuestionIndex: 0,
				CurQuestion:       1,
				RightAnswers:      0,
			},
		})

		request := &sdk.Request{
			Command: patterns.OtherAnswerIntent,
		}

		expectedState := State{
			GameState: replies.QuestionState,
			UsedQuestionIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				LastQuestionIndex: 0,
				CurQuestion:       1,
				RightAnswers:      0,
			},
		}

		responseText := "Я вас не поняла. Просто скажите, верите вы мне или нет.\nБыки ненавидят красный цвет."

		expectedResponse := &sdk.Response{
			Text:       responseText,
			Tts:        responseText,
			Buttons:    buttons.QuestionButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("AskQuestion_EndRound", func(t *testing.T) {
		_, ctx := createTestContext(false, State{
			GameState: replies.QuestionState,
			UsedQuestionIndexes: map[int]bool{
				0: true,
				1: true,
				2: true,
				3: true,
				4: true,
			},
			RoundState: RoundState{
				LastQuestionIndex: 4,
				CurQuestion:       numQuestions,
				RightAnswers:      4,
			},
		})

		request := &sdk.Request{
			Command: patterns.TruthAnswerIntent,
		}

		expectedState := State{
			GameState: replies.StartGameState,
			UsedQuestionIndexes: map[int]bool{
				0: true,
				1: true,
				2: true,
				3: true,
				4: true,
			},
			RoundState: RoundState{
				LastQuestionIndex: 4,
				CurQuestion:       numQuestions,
				RightAnswers:      5,
			},
		}
		responseText := "Действительно, это так. " +
			"Продолжительность жизни гренландской полярной акулы может превышать 500 лет.\n" +
			"Идеальный результат! 5 из 5.\n" +
			"Давайте ещё?"

		expectedResponse := &sdk.Response{
			Text:       responseText,
			Tts:        responseText,
			Buttons:    buttons.StartGameButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_RestartGame(t *testing.T) {
	game := NewCheatGame(sdkTest.CreateRandMock(seed))

	game.questions = game.questions[:3]

	_, ctx := createTestContext(false, State{
		GameState: replies.QuestionState,
		UsedQuestionIndexes: map[int]bool{
			0: true,
			1: true,
			2: true,
		},
		RoundState: RoundState{
			LastQuestionIndex: 2,
			CurQuestion:       3,
			RightAnswers:      2,
		},
	})

	request := &sdk.Request{
		Command: patterns.TruthAnswerIntent,
	}
	response, err := game.Handle(logger, ctx, request, sdkTest.DefaultMeta())
	assert.NoError(t, err)

	expectedState := State{
		GameState:           replies.StartGameState,
		UsedQuestionIndexes: map[int]bool{},
		RoundState: RoundState{
			LastQuestionIndex: 2,
			CurQuestion:       3,
			RightAnswers:      3,
		},
	}
	responseText := "Действительно, такая программа была. " +
		"Дельфинов обучали искать подводные мины и обезвреживать диверсантов.\n" +
		"Поехали дальше.\n" +
		"3 из 3.\n" +
		"У меня закончились вопросы и я перезапускаю игру. Ещё раунд?"
	expectedResponse := &sdk.Response{
		Text:       responseText,
		Tts:        responseText,
		Buttons:    buttons.StartGameButtons,
		EndSession: false,
	}
	assert.Equal(t, expectedState, ctx.State)
	assert.Equal(t, expectedResponse, response)
}
