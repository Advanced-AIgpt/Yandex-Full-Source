package game

import (
	"testing"

	structpb "github.com/golang/protobuf/ptypes/struct"
	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/replies"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/words"
)

var seed int64 = 0

var matchesMap = map[string][]sdk.Hypothesis{
	patterns.StartGameIntent: {
		{Name: patterns.StartGameIntent},
	},
	patterns.RulesGameIntent: {
		{Name: patterns.RulesGameIntent},
	},
	patterns.ChangeWordIntent: {
		{Name: patterns.ChangeWordIntent},
	},
	patterns.GetDefinitionIntent: {
		{Name: patterns.GetDefinitionIntent},
	},
	patterns.NewTipIntent: {
		{Name: patterns.NewTipIntent},
	},
	patterns.NextTipIntent: {
		{Name: patterns.NextTipIntent},
	},
	patterns.YesContinueGameIntent: {
		{Name: patterns.YesContinueGameIntent},
	},
	patterns.NoContinueGameIntent: {
		{Name: patterns.NoContinueGameIntent},
	},
	patterns.UserMainWordIntent: {
		{Name: patterns.UserMainWordIntent},
	},
	patterns.UserChangeWordIntent: {
		{Name: patterns.UserChangeWordIntent},
	},
	patterns.EndGameIntent: {
		{Name: patterns.EndGameIntent},
	},
	"театр": {
		{Name: patterns.UserWordIntent, Variables: map[string][]interface{}{"Any": {&structpb.Value{Kind: &structpb.Value_StringValue{StringValue: "театр"}}}}},
	},
	"пат": {
		{Name: patterns.UserWordIntent, Variables: map[string][]interface{}{"Any": {&structpb.Value{Kind: &structpb.Value_StringValue{StringValue: "пат"}}}}},
	},
	"пар": {
		{Name: patterns.UserWordIntent, Variables: map[string][]interface{}{"Any": {&structpb.Value{Kind: &structpb.Value_StringValue{StringValue: "пар"}}}}},
	},
	"кратер": {
		{Name: patterns.UserWordIntent, Variables: map[string][]interface{}{"Any": {&structpb.Value{Kind: &structpb.Value_StringValue{StringValue: "кратер"}}}}},
	},
	"что такое парикмахер?": {
		{Name: patterns.GetDefinitionIntent, Variables: map[string][]interface{}{"Any": {&structpb.Value{Kind: &structpb.Value_StringValue{StringValue: "парикмахер"}}}}},
	},
	"Креативность": {
		{Name: patterns.UserWordIntent, Variables: map[string][]interface{}{"Any": {&structpb.Value{Kind: &structpb.Value_StringValue{StringValue: "креаТивность"}}}}},
	},
	"реактивность": {
		{Name: patterns.UserWordIntent, Variables: map[string][]interface{}{"Any": {&structpb.Value{Kind: &structpb.Value_StringValue{StringValue: "реактивность"}}}}},
	},
}

var logger = sdkTest.CreateTestLogger()

func createTestContext(session bool, state State) (*sdkTest.TestContext, *Context) {
	testContext := sdkTest.CreateTestContext(session, matchesMap)
	gameContext := &Context{
		State:   state,
		Context: testContext,
	}
	return testContext, gameContext
}

func testRequest(t *testing.T, ctx *Context, request *sdk.Request, expectedState State, expectedResponse *sdk.Response) {
	game := NewWordGame(sdkTest.CreateRandMock(seed))
	response, err := game.Handle(logger, ctx, request, sdkTest.DefaultMeta())
	assert.NoError(t, err)
	assert.Equal(t, expectedState, ctx.State)
	assert.Equal(t, expectedResponse, response)
}

func TestWordGame_StartGame(t *testing.T) {
	t.Run("StartGame", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.StartGameIntent,
		}
		_, ctx := createTestContext(true, State{
			GameState:            replies.GeneralState,
			UsedMainWordsIndexes: make(map[int]bool),
			LeftMainWords:        51,
		})
		smallWords, _ := words.GetRoundContent()
		expectedState := State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			LeftMainWords: 50,
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},
				LastWord:         "креативность",
				Words:            smallWords[0].Words,
			},
		}
		expectedResponse := &sdk.Response{
			Text: "Отлично! Мы с вами по очереди составляем маленькие слова из большого. " +
				"Вы можете сменить большое слово, если скажете «новое слово» или «хочу загадать слово». " +
				"Или в любой момент попросить у меня подсказку. А пока давайте попробуем со словом КРЕАТИВНОСТЬ",
			Tts: "Отлично! Мы с вами по очереди составляем маленькие слова из большого. " +
				"Вы можете сменить большое слово, если скажете «новое слово» или «хочу загадать слово». " +
				"Или в любой момент попросить у меня подсказку. А пока давайте попробуем со словом КРЕАТИВНОСТЬ",
			Buttons:    buttons.DefaultGameButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestWordGame_ContinueGame(t *testing.T) {
	t.Run("YesContinueGame", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.YesContinueGameIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.RulesState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			LeftMainWords: 50,
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
			},
		})
		expectedState := State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool(nil),
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Текущее слово: КРЕАТИВНОСТЬ",
			Tts:        "Текущее слово: КРЕАТИВНОСТЬ",
			Buttons:    buttons.DefaultGameButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("NoContinueGame", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NoContinueGameIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.RulesState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			LeftMainWords: 50,
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
			},
		})
		expectedState := State{
			GameState: replies.EndGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool(nil),
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Если захотите ещё поиграть, скажите «Давай сыграем в слова».",
			Tts:        "Если захотите ещё поиграть, скажите «Давай сыграем в слова».",
			EndSession: true,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestWordGame_MainWordInit(t *testing.T) {
	smallWords, _ := words.GetRoundContent()
	t.Run("MainWordInit", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ChangeWordIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.RulesState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			LeftMainWords: 50,
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
			},
		})
		expectedState := State{
			GameState: replies.AfterMainWordChangeState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
				1: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "ЕСТЕСТВОЗНАНИЕ",
					Voice: "ЕСТЕСТВОЗНАНИЕ",
				},
				Words:            smallWords[1].Words,
				LastWord:         "естествознание",
				UsedWordsIndexes: map[int]bool{},
			},
			LeftMainWords: 49,
		}
		expectedResponse := &sdk.Response{
			Text:       "Хорошо, пусть будет слово ЕСТЕСТВОЗНАНИЕ. Начинайте!",
			Tts:        "Хорошо, пусть будет слово ЕСТЕСТВОЗНАНИЕ. Начинайте!",
			Buttons:    buttons.DefaultGameButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestWordGame_MainWordChangeState(t *testing.T) {
	smallWords, _ := words.GetRoundContent()
	t.Run("NoStartGame", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NoContinueGameIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.AfterMainWordChangeState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},
				LastWord:         "креативность",
				Words:            smallWords[0].Words,
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.EndGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},
				LastWord:         "креативность",
				GuessedWords:     0,
				Words:            smallWords[0].Words,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Если захотите ещё поиграть, скажите «Давай сыграем в слова».",
			Tts:        "Если захотите ещё поиграть, скажите «Давай сыграем в слова».",
			EndSession: true,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestWordGame_Rules(t *testing.T) {
	t.Run("Rules", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.RulesGameIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState:            replies.GeneralState,
			UsedMainWordsIndexes: make(map[int]bool),
			LeftMainWords:        51,
		},
		)
		expectedState := State{
			GameState:            replies.RulesState,
			UsedMainWordsIndexes: make(map[int]bool),
			LeftMainWords:        51,
		}
		expectedResponse := &sdk.Response{
			Text: "Правила игры просты: я выбираю или вы выбираете какое-нибудь длинное слово. " +
				"Дальше из его букв мы составляем более короткие слова. " +
				"Слова должны быть существительными в единственном числе. " +
				"Составлять слова будем по очереди. " +
				"Вы можете сменить большое слово, если скажете «новое слово» или «хочу загадать слово»." +
				" Или в любой момент попросить у меня подсказку. \n Продолжаем?",
			Tts: "Правила игры просты: - я выбираю - или вы выбираете - какое-нибудь длинное слово." +
				"  Дальше из его букв мы составляем более короткие слова." +
				" Слов+а должны быть существительными в единственном числе." +
				"  Составлять слов+а будем по очереди. " +
				"Вы можете сменить большое слово, если скажете «новое слово» или «хочу загадать слово»." +
				" Или в любой момент попросить у меня подсказку. \n Продолжаем?",
			Buttons:    buttons.DefaultContinueButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestWordGame_AcceptedWord(t *testing.T) {
	smallWords, _ := words.GetRoundContent()
	t.Run("AcceptedWord", func(t *testing.T) {
		request := &sdk.Request{
			Command: "театр",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},

				Words: smallWords[0].Words,
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{
					0:   true,
					214: true,
				},
				LastWord:     "рать",
				GuessedWords: 1,
				Words:        smallWords[0].Words,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Моё слово: РАТЬ\nТекущее слово: КРЕАТИВНОСТЬ",
			Tts:        "Моё слово: РАТЬ\nТекущее слово: КРЕАТИВНОСТЬ",
			EndSession: false,
			Buttons:    buttons.DefaultGameButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("AcceptedWord2", func(t *testing.T) {
		request := &sdk.Request{
			Command: "пар",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				17: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "ПАРИКМАХЕРСКАЯ",
					Voice: "ПАРИКМАХЕРСКАЯ",
				},
				UsedWordsIndexes: map[int]bool{},
				Words:            smallWords[17].Words,
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				17: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "ПАРИКМАХЕРСКАЯ",
					Voice: "ПАРИКМАХЕРСКАЯ",
				},
				Words:    smallWords[17].Words,
				LastWord: "кепка",
				UsedWordsIndexes: map[int]bool{
					0:  true,
					13: true,
				},
				GuessedWords: 1,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Моё слово: КЕПКА\nТекущее слово: ПАРИКМАХЕРСКАЯ",
			Tts:        "Моё слово: КЕПКА\nТекущее слово: ПАРИКМАХЕРСКАЯ",
			EndSession: false,
			Buttons:    buttons.DefaultGameButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("GreatAcceptedWord", func(t *testing.T) {
		request := &sdk.Request{
			Command: "реактивность",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},

				Words: smallWords[0].Words,
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{
					0:   true,
					478: true,
				},
				LastWord:     "рать",
				GuessedWords: 1,
				Words:        smallWords[0].Words,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Вау! Длинное слово.\nМоё слово: РАТЬ\nТекущее слово: КРЕАТИВНОСТЬ",
			Tts:        "Вау! Длинное слово.\nМоё слово: РАТЬ\nТекущее слово: КРЕАТИВНОСТЬ",
			EndSession: false,
			Buttons:    buttons.DefaultGameButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestWordGame_Used(t *testing.T) {
	smallWords, _ := words.GetRoundContent()
	t.Run("UsedWord", func(t *testing.T) {
		request := &sdk.Request{
			Command: "театр",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				Words:            smallWords[0].Words,
				UsedWordsIndexes: map[int]bool{214: true},
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				Words:            smallWords[0].Words,
				UsedWordsIndexes: map[int]bool{214: true},
				GuessedWords:     0,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Это слово уже было. Давайте другое!",
			Tts:        "Это слово уже было. Давайте другое!",
			EndSession: false,
			Buttons:    buttons.DefaultGameButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("UsingMainWord", func(t *testing.T) {
		request := &sdk.Request{
			Command: "Креативность",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				Words:            smallWords[0].Words,
				UsedWordsIndexes: map[int]bool{214: true},
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				Words:            smallWords[0].Words,
				UsedWordsIndexes: map[int]bool{214: true},
				GuessedWords:     0,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Это загаданное слово. Оно не засчитывается.",
			Tts:        "Это загаданное слово. Оно не засчитывается.",
			EndSession: false,
			Buttons:    buttons.DefaultGameButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestWordGame_Unsuitable(t *testing.T) {
	t.Run("WrongLetter", func(t *testing.T) {
		request := &sdk.Request{
			Command: "пат",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},
				GuessedWords:     0,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Буквы «п» нет в слове КРЕАТИВНОСТЬ.",
			Tts:        "Буквы п+э - нет в слове КРЕАТИВНОСТЬ.",
			EndSession: false,
			Buttons:    buttons.DefaultGameButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("WrongSequence", func(t *testing.T) {
		request := &sdk.Request{
			Command: "кратер",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},
				GuessedWords:     0,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Ещё одной буквы «р» нет в слове КРЕАТИВНОСТЬ.",
			Tts:        "Ещё одной буквы +эр - нет в слове КРЕАТИВНОСТЬ.",
			EndSession: false,
			Buttons:    buttons.DefaultGameButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestWordGame_GetDefinition(t *testing.T) {
	smallWords, _ := words.GetRoundContent()
	t.Run("GetDefinition", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.GetDefinitionIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				17: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "ПАРИКМАХЕРСКАЯ",
					Voice: "ПАРИКМАХЕРСКАЯ",
				},
				UsedWordsIndexes: map[int]bool{},
				Words:            smallWords[17].Words,
				LastWord:         "парикмахерская",
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.DefinitionState,
			UsedMainWordsIndexes: map[int]bool{
				17: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "ПАРИКМАХЕРСКАЯ",
					Voice: "ПАРИКМАХЕРСКАЯ",
				},
				Words:            smallWords[17].Words,
				LastWord:         "парикмахерская",
				UsedWordsIndexes: map[int]bool{},
				GuessedWords:     0,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Значения слова ПАРИКМАХЕРСКАЯ:\n1. предприятие, предоставляющее услуги по уходу за волосами, такие, как стрижка, завивка, создание причёски, окраска, бритьё и т. п.\n\nПродолжаем?",
			Tts:        "Значения сл+ова ПАРИКМАХЕРСКАЯ:\n1. предприятие, предоставляющее услуги по уходу за волосами, такие, как стрижка, завивка, создание причёски, окраска, бритьё и т. п.\n\nПродолжаем?",
			EndSession: false,
			Buttons:    buttons.NewContinueDefinitionButtons("Посмотреть в Викисловаре", "https://ru.wiktionary.org/wiki/парикмахерская"),
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("GetDefinition2", func(t *testing.T) {
		request := &sdk.Request{
			Command: "что такое парикмахер?",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				17: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "ПАРИКМАХЕРСКАЯ",
					Voice: "ПАРИКМАХЕРСКАЯ",
				},
				UsedWordsIndexes: map[int]bool{},
				Words:            smallWords[17].Words,
				LastWord:         "парикмахерская",
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.DefinitionState,
			UsedMainWordsIndexes: map[int]bool{
				17: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "ПАРИКМАХЕРСКАЯ",
					Voice: "ПАРИКМАХЕРСКАЯ",
				},
				Words:            smallWords[17].Words,
				LastWord:         "парикмахерская",
				UsedWordsIndexes: map[int]bool{},
				GuessedWords:     0,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "Значения слова ПАРИКМАХЕР:\n1. человек, стригущий другим волосы, делающий причёски\n\nПродолжаем?",
			Tts:        "Значения сл+ова ПАРИКМАХЕР:\n1. человек, стригущий другим волосы, делающий причёски\n\nПродолжаем?",
			EndSession: false,
			Buttons:    buttons.NewContinueDefinitionButtons("Посмотреть в Викисловаре", "https://ru.wiktionary.org/wiki/парикмахер"),
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestWordGame_GetTip(t *testing.T) {
	smallWords, _ := words.GetRoundContent()
	t.Run("NewTip", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NewTipIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
			UsedMainWordsIndexes: map[int]bool{
				17: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "ПАРИКМАХЕРСКАЯ",
					Voice: "ПАРИКМАХЕРСКАЯ",
				},
				UsedWordsIndexes: map[int]bool{},
				Words:            smallWords[17].Words,
				LastWord:         "парикмахерская",
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.TipState,
			UsedMainWordsIndexes: map[int]bool{
				17: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "ПАРИКМАХЕРСКАЯ",
					Voice: "ПАРИКМАХЕРСКАЯ",
				},
				Words:            smallWords[17].Words,
				LastWord:         "парикмахерская",
				UsedWordsIndexes: map[int]bool{},
				GuessedWords:     0,
				WasInTip: map[string]bool{
					"аеккп": true,
				},
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "пкеак — переставьте эти буквы так, чтобы получилось подходящее слово.",
			Tts:        "пкеак — переставьте эти буквы так, чтобы получилось подходящее слово.",
			EndSession: false,
			Buttons:    buttons.DefaultGameButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("NextTip", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NextTipIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.TipState,
			UsedMainWordsIndexes: map[int]bool{
				17: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "ПАРИКМАХЕРСКАЯ",
					Voice: "ПАРИКМАХЕРСКАЯ",
				},
				UsedWordsIndexes: map[int]bool{},
				Words:            smallWords[17].Words,
				LastWord:         "парикмахерская",
				WasInTip: map[string]bool{
					"аеккп": true,
				},
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.TipState,
			UsedMainWordsIndexes: map[int]bool{
				17: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "ПАРИКМАХЕРСКАЯ",
					Voice: "ПАРИКМАХЕРСКАЯ",
				},
				Words:            smallWords[17].Words,
				LastWord:         "парикмахерская",
				UsedWordsIndexes: map[int]bool{},
				GuessedWords:     0,
				WasInTip: map[string]bool{
					"аакпс": true,
					"аеккп": true,
				},
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "апкас — переставьте эти буквы так, чтобы получилось подходящее слово.",
			Tts:        "апкас — переставьте эти буквы так, чтобы получилось подходящее слово.",
			EndSession: false,
			Buttons:    buttons.DefaultGameButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestWordGame_SwitchStates(t *testing.T) {
	smallWords, _ := words.GetRoundContent()
	t.Run("RulesToUserChangeWord", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.UserChangeWordIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.RulesState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},
				LastWord:         "креативность",
				Words:            smallWords[0].Words,
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.UserSetMainWordState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},
				LastWord:         "креативность",
				GuessedWords:     0,
				Words:            smallWords[0].Words,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text:       "С каким словом будем играть?",
			Tts:        "С каким словом будем играть?",
			EndSession: false,
			Buttons:    buttons.UserMainWordButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("UserChangeWordToRules", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.RulesGameIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.UserSetMainWordState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},
				LastWord:         "креативность",
				Words:            smallWords[0].Words,
			},
			LeftMainWords: 50,
		},
		)
		expectedState := State{
			GameState: replies.RulesState,
			UsedMainWordsIndexes: map[int]bool{
				0: true,
			},
			RoundState: RoundState{
				MainWord: dialoglib.Cue{
					Text:  "КРЕАТИВНОСТЬ",
					Voice: "КРЕАТИВНОСТЬ",
				},
				UsedWordsIndexes: map[int]bool{},
				LastWord:         "креативность",
				GuessedWords:     0,
				Words:            smallWords[0].Words,
			},
			LeftMainWords: 50,
		}
		expectedResponse := &sdk.Response{
			Text: "Правила игры просты: я выбираю или вы выбираете какое-нибудь длинное слово. " +
				"Дальше из его букв мы составляем более короткие слова. " +
				"Слова должны быть существительными в единственном числе. " +
				"Составлять слова будем по очереди. " +
				"Вы можете сменить большое слово, если скажете «новое слово» или «хочу загадать слово»." +
				" Или в любой момент попросить у меня подсказку. \n Продолжаем?",
			Tts: "Правила игры просты: - я выбираю - или вы выбираете - какое-нибудь длинное слово." +
				"  Дальше из его букв мы составляем более короткие слова." +
				" Слов+а должны быть существительными в единственном числе." +
				"  Составлять слов+а будем по очереди. " +
				"Вы можете сменить большое слово, если скажете «новое слово» или «хочу загадать слово»." +
				" Или в любой момент попросить у меня подсказку. \n Продолжаем?",
			EndSession: false,
			Buttons:    buttons.DefaultContinueButtons,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestWordGame_EndGame(t *testing.T) {
	t.Run("EndGame", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.EndGameIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.StartGameState,
		},
		)
		expectedState := State{
			GameState: replies.EndGameState,
		}
		expectedResponse := &sdk.Response{
			Text:       "Если захотите ещё поиграть, скажите «Давай сыграем в слова».",
			Tts:        "Если захотите ещё поиграть, скажите «Давай сыграем в слова».",
			EndSession: true,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}
