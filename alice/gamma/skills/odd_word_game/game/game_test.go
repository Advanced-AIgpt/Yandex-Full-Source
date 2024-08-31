package game

import (
	"testing"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/odd_word_game/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/odd_word_game/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/odd_word_game/resources/replies"
)

func testRequest(t *testing.T, ctx *Context, request *sdk.Request, expectedState State, expectedResponse *sdk.Response) {
	game := NewOddWordGame(sdkTest.CreateRandMock(seed))
	response, err := game.Handle(logger, ctx, request, sdkTest.DefaultMeta())
	assert.NoError(t, err)
	assert.Equal(t, expectedState, ctx.State)
	assert.Equal(t, expectedResponse, response)
}

func createTestRoundStateFirstLevel(isEasy bool) RoundState {
	return RoundState{
		MajorityTypeID: 0,
		OddTypeID:      1,
		OddWord:        "14",
		MajorityWords: []string{
			"0", "13", "12",
		},
		CurrentCountOfMajorityWords: 3,
		CurrentWords: []string{
			"0", "13", "12", "14",
		},
		IsEasy: isEasy,
	}
}

func createTestRoundStateSecondLevel(isEasy bool) RoundState {
	return RoundState{
		MajorityTypeID: 0,
		OddTypeID:      1,
		OddWord:        "21",
		MajorityWords: []string{
			"0", "20", "19",
		},
		CurrentCountOfMajorityWords: 3,
		CurrentWords: []string{
			"0", "20", "19", "21",
		},
		IsEasy: isEasy,
	}
}

var testButtonsFirstLevel = []sdk.Button{
	{
		Title: "волк",
		Hide:  true,
	},
	{
		Title: "жираф",
		Hide:  true,
	},
	{
		Title: "лев",
		Hide:  true,
	},
	{
		Title: "кошка",
		Hide:  true,
	},
}

var testButtonsSecondLevel = []sdk.Button{
	{
		Title: "волк",
		Hide:  true,
	},
	{
		Title: "корова",
		Hide:  true,
	},
	{
		Title: "коза",
		Hide:  true,
	},
	{
		Title: "индюк",
		Hide:  true,
	},
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
		RoundState: RoundState{
			CurrentCountOfMajorityWords: 3,
		},
		IsFirstGame: true,
	}
	expectedResponse := &sdk.Response{
		Text: "Сыграем в детскую игру «Найди лишнее»! " +
			"Я буду называть слова, а вы попробуйте угадать, какое среди них лишнее.\n" +
			"Если вы готовы, скажите «да». Давайте начнём?",
		Tts: "Сыграем в детскую игру «Найди лишнее»! " +
			"Я буду называть слова, а вы попробуйте угадать, какое среди них лишнее.\n" +
			"Если вы готовы, скажите «да». Давайте начнём?",
		Buttons: []sdk.Button{buttons.YesButton},
	}
	testRequest(t, ctx, request, expectedState, expectedResponse)
}

func TestGame_StateStartGame(t *testing.T) {
	t.Run("yes start game", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.YesStartGameIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			RoundState: RoundState{
				CurrentCountOfMajorityWords: 3,
			},
			IsFirstGame: true,
		})
		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		}
		expectedResponse := &sdk.Response{
			Text:    "Отлично!\nИтак, назовите лишнее слово:\nволк - жираф - лев - кошка.",
			Tts:     "Отлично!\nИтак, назовите лишнее слово:\nволк - жираф - лев - кошка",
			Buttons: testButtonsFirstLevel,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("no start game", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NoStartGameIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
		})
		expectedState := State{
			GameState: replies.EndGameState,
			RoundState: RoundState{
				IsFirstQuestion: true,
			},
		}
		expectedResponse := &sdk.Response{
			Text:       "Очень жаль.",
			Tts:        "Очень жаль.",
			EndSession: true,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("immediately exit", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.EndGameIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
		})
		expectedState := State{
			GameState: replies.EndGameState,
		}
		expectedResponse := &sdk.Response{
			Text:       "Очень жаль.",
			Tts:        "Очень жаль.",
			EndSession: true,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_StateQuestion(t *testing.T) {
	t.Run("unsuccessful hint", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.UnsuccessfulGetHintIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.QuestionState,
		})
		expectedState := State{
			GameState: replies.QuestionState,
		}
		expectedResponse := &sdk.Response{
			Text: "Не знаю, как вам ещё подсказать. Подумайте и попытайтесь предположить ответ.",
			Tts:  "Не знаю, как вам ещё подсказать. Подумайте и попытайтесь предположить ответ.",
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("asking about the answer", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.GetAnswerIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.QuestionState,
		})
		expectedState := State{
			GameState: replies.QuestionState,
			RoundState: RoundState{
				WantsKnowAnswer: true,
			},
		}
		expectedResponse := &sdk.Response{
			Text:    "Может, подумаете ещё немного?",
			Tts:     "Может, подумаете ещё немного?",
			Buttons: buttons.ChooseButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("repeat", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.RepeatQuestionIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		})
		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		}
		expectedResponse := &sdk.Response{
			Text:    "Повторяю:\nволк - жираф - лев - кошка.",
			Tts:     "Повторяю:\nволк - жираф - лев - кошка",
			Buttons: testButtonsFirstLevel,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("get hint", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.GetHintIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		})
		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		}
		expectedResponse := &sdk.Response{
			Text: "Большинство из перечисленного относится к классу дикие животные. Что к нему не относится?\n" +
				"волк - жираф - лев - кошка.",
			Tts: "Большинство из перечисленного относится к классу дикие животные. Что к нему не относится?\n" +
				"волк - жираф - лев - кошка",
			Buttons: testButtonsFirstLevel,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("change level up", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ChangeLevelUpIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.QuestionState,
			RoundState: RoundState{
				CurrentCountOfMajorityWords: 3,
			},
		})
		expectedState := State{
			GameState:    replies.QuestionState,
			CurrentLevel: 1,
			RoundState:   createTestRoundStateSecondLevel(false),
		}
		expectedResponse := &sdk.Response{
			Text: "Ну, если вы так уверены в себе, попробуем что-нибудь посложнее.\n" +
				"Следующее:\nволк - корова - коза - индюк.",
			Tts: "Ну, если вы так уверены в себе, попробуем что-нибудь посложнее.\n" +
				"Следующее:\nволк - корова - коза - индюк",
			Buttons: testButtonsSecondLevel,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("change level down", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ChangeLevelDownIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState:    replies.QuestionState,
			CurrentLevel: 2,
			RoundState: RoundState{
				CurrentCountOfMajorityWords: 3,
			},
		})
		expectedState := State{
			GameState:    replies.QuestionState,
			CurrentLevel: 1,
			RoundState:   createTestRoundStateSecondLevel(false),
		}
		expectedResponse := &sdk.Response{
			Text: "Хорошо, попробую задать вопрос полегче.\n" +
				"Следующее:\nволк - корова - коза - индюк.",
			Tts: "Хорошо, попробую задать вопрос полегче.\n" +
				"Следующее:\nволк - корова - коза - индюк",
			Buttons: testButtonsSecondLevel,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("unsuccessful change level up", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ChangeLevelUpIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState:    replies.QuestionState,
			CurrentLevel: 3,
			RoundState: RoundState{
				MajorityTypeID: 0,
				OddTypeID:      1,
				OddWord:        "117",
				MajorityWords: []string{
					"0", "116", "115",
				},
				CurrentCountOfMajorityWords: 3,
				CurrentWords: []string{
					"0", "116", "115", "117",
				},
			},
		})
		expectedState := State{
			GameState:    replies.QuestionState,
			CurrentLevel: 3,
			RoundState: RoundState{
				MajorityTypeID: 0,
				OddTypeID:      1,
				OddWord:        "117",
				MajorityWords: []string{
					"0", "116", "115",
				},
				CurrentCountOfMajorityWords: 3,
				CurrentWords: []string{
					"0", "116", "115", "117",
				},
			},
		}
		expectedResponse := &sdk.Response{
			Text: "Сложнее уже некуда. Ответите на вопросы этого уровня — и вы выиграли! Итак, какое слово лишнее:\n" +
				"волк - можжевельник - ольха - молоток.",
			Tts: "Сложнее уже некуда. Ответите на вопросы этого уровня — и вы выиграли! Итак, какое слово лишнее:\n" +
				"волк - можжевельник - ольх+а - молоток",
			Buttons: []sdk.Button{
				{
					Title: "волк",
					Hide:  true,
				},
				{
					Title: "можжевельник",
					Hide:  true,
				},
				{
					Title: "ольха",
					Hide:  true,
				},
				{
					Title: "молоток",
					Hide:  true,
				},
			},
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("unsuccessful change level down", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ChangeLevelDownIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		})
		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(true),
		}
		expectedResponse := &sdk.Response{
			Text: "Не могу придумать ничего проще! Соберитесь и попробуйте определить, какое слово здесь лишнее:\n" +
				"волк - жираф - лев - кошка.",
			Tts: "Не могу придумать ничего проще! Соберитесь и попробуйте определить, какое слово здесь лишнее:\n" +
				"волк - жираф - лев - кошка",
			Buttons: testButtonsFirstLevel,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("invalid answer", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.InvalidAnswerIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		})
		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		}
		expectedResponse := &sdk.Response{
			Text: "Так не пойдёт! Нужно назвать одно слово.\n" +
				"волк - жираф - лев - кошка.",
			Tts: "Так не пойдёт! Нужно назвать одно слово.\n" +
				"волк - жираф - лев - кошка",
			Buttons: testButtonsFirstLevel,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("misunderstanding", func(t *testing.T) {
		request := &sdk.Request{
			Command: "junk command",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.QuestionState,
		})
		expectedState := State{
			GameState: replies.QuestionState,
		}
		expectedResponse := &sdk.Response{
			Text: "Попробуйте сказать ещё раз, я не очень поняла ваш ответ.",
			Tts:  "Попробуйте сказать ещё раз, я не очень поняла ваш ответ.",
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("user wants to think", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.UserWantsToThinkIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		})
		ctx.RoundState.WantsKnowAnswer = true

		expectedState := State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		}
		expectedResponse := &sdk.Response{
			Text: "Замечательно! Итак, найдите здесь лишнее слово:\n" +
				"волк - жираф - лев - кошка.",
			Tts: "Замечательно! Итак, найдите здесь лишнее слово:\n" +
				"волк - жираф - лев - кошка",
			Buttons: testButtonsFirstLevel,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("right answer", func(t *testing.T) {
		request := &sdk.Request{
			Command: "мне кажется, кошка",
		}
		_, ctx := createTestContext(false, State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		})

		expectedState := State{
			GameState: replies.QuestionState,
			RoundState: RoundState{
				OddTypeID: 1,
				OddWord:   "14",
				MajorityWords: []string{
					"0", "13", "12", "11",
				},
				CurrentCountOfMajorityWords: 4,
				CurrentWords: []string{
					"0", "13", "12", "11", "14",
				},
			},
			Answers:      1,
			RightAnswers: 1,
		}
		expectedResponse := &sdk.Response{
			Text: "Верно!\nКошка - это домашнее животное, а остальное - это дикие животные.\n" +
				"Следующее:\n" +
				"волк - жираф - лев - тигр - кошка.",
			Tts: "Верно!\nКошка - это домашнее животное, а остальное - это дикие животные.\n" +
				"Следующее:\n" +
				"волк - жираф - лев - тигр - кошка",
			Buttons: []sdk.Button{
				{
					Title: "волк",
					Hide:  true,
				},
				{
					Title: "жираф",
					Hide:  true,
				},
				{
					Title: "лев",
					Hide:  true,
				},
				{
					Title: "тигр",
					Hide:  true,
				},
				{
					Title: "кошка",
					Hide:  true,
				},
			},
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("wrong answer", func(t *testing.T) {
		request := &sdk.Request{
			Command: "наверное, это лев",
		}
		_, ctx := createTestContext(false, State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		})

		expectedState := State{
			GameState:    replies.QuestionState,
			RoundState:   createTestRoundStateFirstLevel(false),
			Answers:      1,
			RightAnswers: 0,
		}
		expectedResponse := &sdk.Response{
			Text: "Не совсем так!\n" +
				"Правильный ответ - это кошка. Потому что кошка - это домашнее животное," +
				" а остальное - это дикие животные.\n" +
				"Следующее:\n" +
				"волк - жираф - лев - кошка.",
			Tts: "Не совсем так!\n" +
				"Правильный ответ - это кошка. Потому что кошка - это домашнее животное," +
				" а остальное - это дикие животные.\n" +
				"Следующее:\n" +
				"волк - жираф - лев - кошка",
			Buttons: testButtonsFirstLevel,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("word not from the list", func(t *testing.T) {
		request := &sdk.Request{
			Command: "это тигр",
		}
		_, ctx := createTestContext(false, State{
			GameState:  replies.QuestionState,
			RoundState: createTestRoundStateFirstLevel(false),
		})

		expectedState := State{
			GameState:    replies.QuestionState,
			RoundState:   createTestRoundStateFirstLevel(false),
			Answers:      1,
			RightAnswers: 0,
		}
		expectedResponse := &sdk.Response{
			Text: "Пожалуйста, назовите слово из списка:\n" +
				"волк - жираф - лев - кошка.",
			Tts: "Пожалуйста, назовите слово из списка:\n" +
				"волк - жираф - лев - кошка",
			Buttons: testButtonsFirstLevel,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}
