package game

import (
	"testing"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/akinator/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/akinator/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/akinator/resources/replies"
)

func testRequest(t *testing.T, ctx *Context, request *sdk.Request, expectedState State, expectedResponse *sdk.Response) {
	game := NewAkinatorGame(sdkTest.CreateRandMock(seed))
	response, err := game.Handle(logger, ctx, request, sdkTest.DefaultMeta())
	assert.NoError(t, err)
	assert.Equal(t, expectedState, ctx.State)
	assert.Equal(t, expectedResponse, response)
}

func DefaultRoundState() RoundState {
	return RoundState{
		LastActorIndex: -1,
		CurrentHint:    "",
	}
}

func TestGame_StartGameNoError(t *testing.T) {
	request := &sdk.Request{
		Command: "junk command",
	}
	_, ctx := CreateTestContext(true, State{
		GameState:  replies.GeneralState,
		RoundState: DefaultRoundState(),
	})
	expectedState := State{
		GameState:  replies.StartGameState,
		RoundState: DefaultRoundState(),
		IsPromo:    true,
	}
	expectedResponse := &sdk.Response{
		Text:       "Я загадываю актёра – вы отгадываете. У вас 5 попыток. Сыграем?",
		Tts:        "Я загадываю актёра – вы отгадываете. У вас 5 попыток. Сыграем?",
		Buttons:    buttons.StartButtons,
		EndSession: false,
	}
	testRequest(t, ctx, request, expectedState, expectedResponse)
}

func TestGame_StartGame(t *testing.T) {
	t.Run("StartGame", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.StartGameIntent,
		}
		_, ctx := CreateTestContext(true, State{
			GameState:  replies.GeneralState,
			RoundState: DefaultRoundState(),
		})
		expectedState := State{
			GameState:  replies.StartGameState,
			RoundState: DefaultRoundState(),
			IsPromo:    true,
		}
		expectedResponse := &sdk.Response{
			Text:       "Я загадываю актёра – вы отгадываете. У вас 5 попыток. Сыграем?",
			Tts:        "Я загадываю актёра – вы отгадываете. У вас 5 попыток. Сыграем?",
			Buttons:    buttons.StartButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("StartGameYes", func(t *testing.T) {
		_, ctx := CreateTestContext(false, State{
			GameState:  replies.StartGameState,
			RoundState: DefaultRoundState(),
		})
		request := &sdk.Request{
			Command: patterns.YesStartGameIntent,
		}
		expectedState := State{
			GameState: replies.GuessState,
			RoundState: RoundState{
				LastActorIndex: 0,
				Hints: []string{
					"Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
					"Сыграл комедийную роль неуклюжего разведчика в фильме «Шпион».",
					"Он играл в фильмах «Форсаж 7», «Форсаж 8».",
					"Этот актер женат на британской модели Розе Хантингтон Уайтли.",
					"Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
				},
				CountOfUsedHints: 1,
				CurrentHint:      "Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
			},
			UsedActorsIndex: []int{
				36, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
				21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 0,
			},
			CountOfUsedActors: 1,
		}
		expectedResponse := &sdk.Response{
			Text: "Начинаем.\n" +
				"Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
			Tts: "Начинаем.\n" +
				"Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
			Buttons: buttons.HintButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func testNextHint(t *testing.T, nameOfTheTest string) {
	t.Run(nameOfTheTest, func(t *testing.T) {
		_, ctx := CreateTestContext(false, State{
			GameState: replies.GuessState,
			RoundState: RoundState{
				LastActorIndex: 0,
				Hints: []string{
					"Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
					"Сыграл комедийную роль неуклюжего разведчика в фильме «Шпион».",
					"Он играл в фильмах «Форсаж 7», «Форсаж 8».",
					"Этот актер женат на британской модели Розе Хантингтон Уайтли.",
					"Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
				},
				CountOfUsedHints: 1,
				CurrentHint:      "Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
			},
		})
		request := &sdk.Request{
			Command: patterns.NextHintIntent,
		}
		expectedState := State{
			GameState: replies.GuessState,
			RoundState: RoundState{
				LastActorIndex: 0,
				Hints: []string{
					"Этот актер женат на британской модели Розе Хантингтон Уайтли.",
					"Сыграл комедийную роль неуклюжего разведчика в фильме «Шпион».",
					"Он играл в фильмах «Форсаж 7», «Форсаж 8».",
					"Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
					"Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
				},
				CountOfUsedHints: 2,
				CurrentHint:      "Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
			},
		}
		expectedResponse := &sdk.Response{
			Text:    "Ок.\n" + "Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
			Tts:     "Ок." + "\nТакже снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
			Buttons: buttons.HintButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_GuessActor(t *testing.T) {
	t.Run("GuessActorFalseAnswer, count of used hints <= 5", func(t *testing.T) {
		_, ctx := CreateTestContext(false, State{
			GameState: replies.GuessState,
			RoundState: RoundState{
				LastActorIndex: 0,
				Hints: []string{
					"Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
					"Сыграл комедийную роль неуклюжего разведчика в фильме «Шпион».",
					"Он играл в фильмах «Форсаж 7», «Форсаж 8».",
					"Этот актер женат на британской модели Розе Хантингтон Уайтли.",
					"Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
				},
				CountOfUsedHints: 1,
				CurrentHint:      "Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
			},
		})
		request := &sdk.Request{
			Command: patterns.AnswerIntent,
		}
		expectedState := State{
			GameState: replies.GuessState,
			RoundState: RoundState{
				LastActorIndex: 0,
				Hints: []string{
					"Этот актер женат на британской модели Розе Хантингтон Уайтли.",
					"Сыграл комедийную роль неуклюжего разведчика в фильме «Шпион».",
					"Он играл в фильмах «Форсаж 7», «Форсаж 8».",
					"Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
					"Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
				},
				CountOfUsedHints: 2,
				CurrentHint:      "Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
			},
		}
		expectedResponse := &sdk.Response{
			Text:    "Нет." + "\nТакже снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
			Tts:     "Нет." + "\nТакже снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
			Buttons: buttons.HintButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("Test repeat hint", func(t *testing.T) {
		_, ctx := CreateTestContext(false, State{
			GameState: replies.GuessState,
			RoundState: RoundState{
				CurrentHint: "Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
			},
		})
		request := &sdk.Request{
			Command: patterns.RepeatHintIntent,
		}
		expectedState := State{
			GameState: replies.GuessState,
			RoundState: RoundState{
				CurrentHint: "Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
			},
		}
		expectedResponse := &sdk.Response{
			Text:    "Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
			Tts:     "Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
			Buttons: buttons.HintButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("Test user thinks", func(t *testing.T) {
		_, ctx := CreateTestContext(false, State{
			GameState:  replies.GuessState,
			RoundState: RoundState{},
		})
		request := &sdk.Request{
			Command: patterns.LetUserThinkIntent,
		}
		expectedState := State{
			GameState:  replies.GuessState,
			RoundState: RoundState{},
		}
		expectedResponse := &sdk.Response{
			Text:    "Вижу, вам требуется время подумать. Я подожду. Только чур не искать в Яндексе!",
			Tts:     "Вижу, вам требуется время подумать. Я подожду. Только чур не искать в Яндексе!",
			Buttons: buttons.LetThinkButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("Alice ask about saying asnwer", func(t *testing.T) {
		_, ctx := CreateTestContext(false, State{
			GameState:  replies.GuessState,
			RoundState: RoundState{},
		})
		request := &sdk.Request{
			Command: patterns.SayAnswerIntent,
		}
		expectedState := State{
			GameState: replies.GuessState,
			RoundState: RoundState{
				WantsKnowAnswer: true,
			},
		}
		expectedResponse := &sdk.Response{
			Text:    "Рано вы сдаётесь! Может, ещё подумаете?",
			Tts:     "Рано вы сдаётесь! Может, ещё подумаете?",
			Buttons: buttons.AnswerButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	testNextHint(t, "User doesn't know the answer, count of used hints <= 5")
	testNextHint(t, "User doesn't know the answer, count of used hints <= 5")

	t.Run("Test next hint, when count of used hints == 5", func(t *testing.T) {
		_, ctx := CreateTestContext(false, State{
			GameState: replies.GuessState,
			RoundState: RoundState{
				LastActorIndex: 0,
				Hints: []string{
					"Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
					"Сыграл комедийную роль неуклюжего разведчика в фильме «Шпион».",
					"Он играл в фильмах «Форсаж 7», «Форсаж 8».",
					"Этот актер женат на британской модели Розе Хантингтон Уайтли.",
					"Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
				},
				CountOfUsedHints: 5,
				CurrentHint:      "Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
			},
			CountOfUsedActors: 1,
		})
		request := &sdk.Request{
			Command: patterns.NextHintIntent,
		}
		expectedState := State{
			GameState: replies.StartGameState,
			RoundState: RoundState{
				LastActorIndex: -1,
				Hints: []string{
					"Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
					"Сыграл комедийную роль неуклюжего разведчика в фильме «Шпион».",
					"Он играл в фильмах «Форсаж 7», «Форсаж 8».",
					"Этот актер женат на британской модели Розе Хантингтон Уайтли.",
					"Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
				},
				CountOfUsedHints: 5,
				CurrentHint:      "Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
			},
			CountOfUsedActors: 1,
		}
		expectedResponse := &sdk.Response{
			Text: "Все подсказки закончились. Правильный ответ - Джейсон Стэтэм. Не расстраивайтесь.\n" +
				"Хотите сыграть ещё?",
			Tts: "Все подсказки закончились. Правильный ответ - Джейсон Стэтэм. Не расстраивайтесь.\n" +
				"Хотите сыграть ещё?",
			Buttons: buttons.ContinueButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("Exit", func(t *testing.T) {
		_, ctx := CreateTestContext(false, State{
			GameState:  replies.GuessState,
			RoundState: RoundState{},
		})
		request := &sdk.Request{
			Command: patterns.ExitIntent,
		}
		expectedState := State{
			GameState: replies.EndGameState,
			RoundState: RoundState{
				LastActorIndex: -1,
			},
		}
		expectedResponse := &sdk.Response{
			Text: "Если захотите ещё раз поиграть в «Угадай актёра»," +
				" просто скажите «Давай сыграем в Угадай актёра».",
			Tts: "Если захотите ещё раз поиграть в «Угадай актёра»," +
				" просто скажите «Давай сыграем в Угадай актёра».",
			EndSession: true,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_TestEndRound(t *testing.T) {
	t.Run("End round, no more actors", func(t *testing.T) {
		_, ctx := CreateTestContext(false, State{
			GameState: replies.StartGameState,
			RoundState: RoundState{
				LastActorIndex: -1,
			},
			CountOfUsedActors: 37,
		})
		request := &sdk.Request{
			Command: patterns.YesStartGameIntent,
		}
		expectedState := State{
			GameState: replies.EndGameState,
			RoundState: RoundState{
				LastActorIndex:  -1,
				WantsKnowAnswer: false,
			},
			CountOfUsedActors: 37,
		}
		expectedResponse := &sdk.Response{
			Text: "Вы угадали всех актёров в моей базе! Пожалуйста, выберите другую игру.",
			Tts:  "Вы угадали всех актёров в моей базе! Пожалуйста, выберите другую игру.",
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("Exit game immediately", func(t *testing.T) {
		_, ctx := CreateTestContext(false, State{
			RoundState: RoundState{},
		})
		request := &sdk.Request{
			Command: patterns.ExitIntent,
		}
		expectedState := State{
			GameState: replies.EndGameState,
			RoundState: RoundState{
				LastActorIndex:   -1,
				CountOfUsedHints: 0,
				WantsKnowAnswer:  false,
			},
		}
		expectedResponse := &sdk.Response{
			Text: "Если захотите ещё раз поиграть в «Угадай актёра»," +
				" просто скажите «Давай сыграем в Угадай актёра».",
			Tts: "Если захотите ещё раз поиграть в «Угадай актёра»," +
				" просто скажите «Давай сыграем в Угадай актёра».",
			EndSession: true,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_SayAnswer(t *testing.T) {
	t.Run("User want know answer", func(t *testing.T) {
		_, ctx := CreateTestContext(false, State{
			GameState: replies.GuessState,
			RoundState: RoundState{
				LastActorIndex: 0,
				Hints: []string{
					"Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
					"Сыграл комедийную роль неуклюжего разведчика в фильме «Шпион».",
					"Он играл в фильмах «Форсаж 7», «Форсаж 8».",
					"Этот актер женат на британской модели Розе Хантингтон Уайтли.",
					"Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
				},
				CountOfUsedHints: 1,
				CurrentHint:      "Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
				WantsKnowAnswer:  true,
			},
			CountOfUsedActors: 1,
		})

		request := &sdk.Request{
			Command: patterns.UserDoesntWantToThinkIntent,
		}
		expectedState := State{
			GameState: replies.StartGameState,
			RoundState: RoundState{
				LastActorIndex: -1,
				Hints: []string{
					"Также снимался в фильме Гая Ричи «Карты, деньги, два ствола».",
					"Сыграл комедийную роль неуклюжего разведчика в фильме «Шпион».",
					"Он играл в фильмах «Форсаж 7», «Форсаж 8».",
					"Этот актер женат на британской модели Розе Хантингтон Уайтли.",
					"Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
				},
				CountOfUsedHints: 1,
				CurrentHint:      "Этот британский актер известен своей ролью в культовом боевике «Перевозчик».",
				WantsKnowAnswer:  false,
			},
			CountOfUsedActors: 1,
		}

		expectedResponse := &sdk.Response{
			Text:    "Ну и ладно. Ответ - Джейсон Стэтэм.\n" + "Хотите сыграть ещё?",
			Tts:     "Ну и ладно. Ответ - Джейсон Стэтэм.\n" + "Хотите сыграть ещё?",
			Buttons: buttons.ContinueButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}
