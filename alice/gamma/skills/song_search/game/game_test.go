package game

import (
	"testing"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/song_search/api"
	"a.yandex-team.ru/alice/gamma/skills/song_search/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/song_search/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/song_search/resources/replies"
)

func testRequest(t *testing.T, ctx *Context, request *sdk.Request, expectedState State, expectedResponse *sdk.Response) {
	game := NewSongSearchGame(sdkTest.CreateRandMock(seed))
	response, err := game.Handle(&api.DummySearch{}, logger, ctx, request, sdkTest.DefaultMeta())
	assert.NoError(t, err)
	assert.Equal(t, expectedState, ctx.State)
	assert.Equal(t, expectedResponse, response)
}

func TestGame_GlobalCommands(t *testing.T) {
	t.Run("Fallback", func(t *testing.T) {
		request := &sdk.Request{
			Command: "junk command",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.GeneralState,
		})
		expectedState := State{
			GameState: replies.SearchState,
		}
		replyText := "Я угадаю любую мелодию с семи нот!\nНапомните мне какую-нибудь строчку из песни на русском языке."
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    buttons.GeneralButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("ShowRules", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.ShowRulesIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.GeneralState,
		})
		expectedState := State{
			GameState: replies.PauseState,
			LastIndex: -1,
		}
		expectedResponse := &sdk.Response{
			Text: "Вот правила игры: вы говорите строчку из песни на русском языке, " +
				"а я подхватываю и говорю следующие. Строчка должна состоять из трех и более слов. " +
				"Если я ошибусь, вы сможете попросить меня поискать другие варианты с этим текстом. " +
				"Продолжим?",
			Tts: "Вот правила игр+ы: вы говорите строчку из песни на русском языке, " +
				"а я подхватываю и говорю следующие. Строчка должна состоять из трех и более слов. " +
				"Если я ошибусь, вы сможете попросить меня поискать другие варианты с этим текстом. " +
				"Продолжим?",
			Buttons:    buttons.PauseButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_StartAndFirstSearch(t *testing.T) {
	t.Run("StartGame", func(t *testing.T) {
		request := &sdk.Request{
			Command: "any junk",
		}
		_, ctx := createTestContext(true, State{
			GameState: replies.GeneralState,
		})
		expectedState := State{
			GameState:     replies.SearchState,
			LastIndex:     -1,
			IsFirstSearch: true,
		}
		replyText := "Я угадаю любую мелодию с семи нот!\nНапомните мне какую-нибудь строчку из песни на русском языке."
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    buttons.GeneralButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("FirstSearch", func(t *testing.T) {
		request := &sdk.Request{
			Command: bigSearchCommand,
		}
		_, ctx := createTestContext(false, State{
			GameState:     replies.SearchState,
			IsFirstSearch: true,
		})
		expectedState := State{
			GameState:     replies.GuessState,
			Results:       bigSearchResults,
			LastIndex:     0,
			IsFirstSearch: false,
		}
		replyText := "И эта ночь и ее электрический голос манит меня к себе\n" +
			"И я не знаю, как мне прожить следующий день.\n\n" +
			"Кино - Ночь\n" +
			"Если я ошиблась, вы можете попросить меня поискать другие варианты с этим текстом. " +
			"Если всё правильно, я могу попробовать угадать ещё одну песню. Просто скажите строчку."
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    buttons.FormGuessButtons(bigSearchResults[0].URL),
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_Guesses(t *testing.T) {
	t.Run("ObsceneTitle", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.IncorrectGuessIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.GuessState,
			Results:   bigSearchResults,
			LastIndex: 2,
		})
		expectedState := State{
			GameState: replies.GuessState,
			LastIndex: 3,
			Results:   bigSearchResults,
		}
		replyText := "Вы что! Я это петь не буду. Даже у голосового помощника есть совесть."
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    buttons.FormGuessButtons(bigSearchResults[3].URL),
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("ObsceneText", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.IncorrectGuessIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.GuessState,
			Results:   bigSearchResults,
			LastIndex: 1,
		})
		expectedState := State{
			GameState: replies.GuessState,
			LastIndex: 2,
			Results:   bigSearchResults,
		}
		replyText := "Если я не ошибаюсь, это Кровосток - Ночь. Но я это петь не буду. Вдруг нас дети услышат."
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    buttons.FormGuessButtons(bigSearchResults[2].URL),
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("IncorrectGuess", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.IncorrectGuessIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.GuessState,
			Results:   bigSearchResults,
			LastIndex: 0,
		})
		expectedState := State{
			GameState: replies.GuessState,
			LastIndex: 1,
			Results:   bigSearchResults,
		}
		replyText := "Может быть, эта?\n" +
			"Был душой я молод, а теперь старик\n" +
			"Был душой я молод, а теперь старик\n\n" +
			"Любэ - Ночь"
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    buttons.FormGuessButtons(bigSearchResults[1].URL),
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("CorrectGuess", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.CorrectGuessIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.GuessState,
			Results:   bigSearchResults,
			LastIndex: 0,
		})
		expectedState := State{
			GameState: replies.SearchState,
			LastIndex: -1,
		}
		replyText := "Я старалась.\nЕсли хотите, чтобы я угадала ещё какую-нибудь песню, просто скажите строчку."
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    buttons.GeneralButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("SearchEnd", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.IncorrectGuessIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.GuessState,
			Results:   bigSearchResults,
			LastIndex: 4,
		})
		expectedState := State{
			GameState: replies.LoseState,
			LastIndex: -1,
		}
		replyText := "Больше ничего не могу найти. Даже я неидеальна.\nСыграем ещё раз?"
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    buttons.NewGameButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("MiddleSearch", func(t *testing.T) {
		request := &sdk.Request{
			Command: standardSearchCommand,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.GuessState,
			Results:   bigSearchResults,
			LastIndex: 2,
		})
		expectedState := State{
			GameState: replies.GuessState,
			Results:   standardSearchResults,
			LastIndex: 0,
		}
		replyText := "Смотри Илон Маск я тут что-то нажала\nИ всё исчезло\nИ всё исчезло\n\nКомсомольск - Всё исчезло"
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    buttons.FormGuessButtons(standardSearchResults[0].URL),
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("SmallSearch", func(t *testing.T) {
		request := &sdk.Request{
			Command: smallSearchCommand,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.GuessState,
			Results:   nil,
			LastIndex: 0,
		})
		expectedState := State{
			GameState: replies.SearchState,
			Results:   nil,
			LastIndex: 0,
		}
		replyText := "Как-то маловато слов.\nМожно поподробнее?"
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    nil,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("AnotherSearch", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.AnotherSearchIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.GuessState,
			Results:   bigSearchResults,
			LastIndex: 2,
		})
		expectedState := State{
			GameState: replies.SearchState,
			Results:   nil,
			LastIndex: -1,
		}
		replyText := "Напомните мне какую-нибудь строчку из песни на русском языке."
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    nil,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_PauseState(t *testing.T) {
	t.Run("YesContinue", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.YesContinueIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.PauseState,
		})
		expectedState := State{
			GameState: replies.SearchState,
		}
		replyText := "Отлично.\nНапомните мне какую-нибудь строчку из песни на русском языке."
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("NoContinue", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NoContinueIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.PauseState,
		})
		expectedState := State{
			GameState: replies.EndGameState,
			LastIndex: -1,
		}
		replyText := "Хорошо.\nЗахотите поиграть ещё - скажите Алиса, давай поиграем в Угадай песню."
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			EndSession: true,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("Fallback", func(t *testing.T) {
		request := &sdk.Request{
			Command: "pause junk command",
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.PauseState,
		})
		expectedState := State{
			GameState: replies.PauseState,
		}
		replyText := "Извините, я вас не поняла. Просто скажите мне, да или нет. Продолжим игру?"
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    buttons.PauseButtons,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_LoseState(t *testing.T) {
	t.Run("YesContinue", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.YesContinueIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.LoseState,
		})
		expectedState := State{
			GameState: replies.SearchState,
		}
		replyText := "Отлично.\nНапомните мне какую-нибудь строчку из песни на русском языке."
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("NoContinue", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NoContinueIntent,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.LoseState,
		})
		expectedState := State{
			GameState: replies.EndGameState,
			LastIndex: -1,
		}
		replyText := "Хорошо.\nЗахотите поиграть ещё - скажите Алиса, давай поиграем в Угадай песню."
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			EndSession: true,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("Fallback", func(t *testing.T) {
		request := &sdk.Request{
			Command: standardSearchCommand,
		}
		_, ctx := createTestContext(false, State{
			GameState: replies.LoseState,
		})
		expectedState := State{
			GameState: replies.GuessState,
			Results:   standardSearchResults,
			LastIndex: 0,
		}
		replyText := "Смотри Илон Маск я тут что-то нажала\nИ всё исчезло\nИ всё исчезло\n\nКомсомольск - Всё исчезло"
		expectedResponse := &sdk.Response{
			Text:       replyText,
			Tts:        replyText,
			Buttons:    buttons.FormGuessButtons(standardSearchResults[0].URL),
			EndSession: false,
		}
		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}
