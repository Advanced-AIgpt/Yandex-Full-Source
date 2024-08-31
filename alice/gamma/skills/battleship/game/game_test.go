package game

import (
	"testing"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/replies"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/ships"
)

var defaultMyField = [][]ships.MyCell{
	{
		{ShipIndex: 0}, {ShipIndex: -1}, {ShipIndex: 1}, {ShipIndex: -1}, {ShipIndex: 2},
		{ShipIndex: -1}, {ShipIndex: 3}, {ShipIndex: -1}, {ShipIndex: 4}, {ShipIndex: -1},
	},
	{
		{ShipIndex: 0}, {ShipIndex: -1}, {ShipIndex: 1}, {ShipIndex: -1}, {ShipIndex: 2},
		{ShipIndex: -1}, {ShipIndex: 3}, {ShipIndex: -1}, {ShipIndex: 4}, {ShipIndex: -1},
	},
	{
		{ShipIndex: 0}, {ShipIndex: -1}, {ShipIndex: 1}, {ShipIndex: -1}, {ShipIndex: 2},
		{ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
	},
	{
		{ShipIndex: 0}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
		{ShipIndex: -1}, {ShipIndex: 5}, {ShipIndex: -1}, {ShipIndex: 6}, {ShipIndex: -1},
	},
	{
		{ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: 7}, {ShipIndex: -1}, {ShipIndex: 8},
		{ShipIndex: -1}, {ShipIndex: 5}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
	},
	{
		{ShipIndex: 9}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
		{ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
	},
	{
		{ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
		{ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
	},
	{
		{ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
		{ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
	},
	{
		{ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
		{ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
	},
	{
		{ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
		{ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1}, {ShipIndex: -1},
	},
}

var defaultShips = []ships.Ship{
	{Cells: []ships.Cell{{X: 0, Y: 0}, {X: 1, Y: 0}, {X: 2, Y: 0}, {X: 3, Y: 0}}, LifeCells: 4},
	{Cells: []ships.Cell{{X: 0, Y: 2}, {X: 1, Y: 2}, {X: 2, Y: 2}}, LifeCells: 3},
	{Cells: []ships.Cell{{X: 0, Y: 4}, {X: 1, Y: 4}, {X: 2, Y: 4}}, LifeCells: 3},
	{Cells: []ships.Cell{{X: 0, Y: 6}, {X: 1, Y: 6}}, LifeCells: 2},
	{Cells: []ships.Cell{{X: 0, Y: 8}, {X: 1, Y: 8}}, LifeCells: 2},
	{Cells: []ships.Cell{{X: 3, Y: 6}, {X: 4, Y: 6}}, LifeCells: 2},
	{Cells: []ships.Cell{{X: 3, Y: 8}}, LifeCells: 1},
	{Cells: []ships.Cell{{X: 4, Y: 2}}, LifeCells: 1},
	{Cells: []ships.Cell{{X: 4, Y: 4}}, LifeCells: 1},
	{Cells: []ships.Cell{{X: 5, Y: 0}}, LifeCells: 1},
}

var defaultUserShipsCounter = []int{4, 3, 2, 1}

var loseUserShipsCounter = []int{0, 0, 0, 0}

func TestGame_Generate(t *testing.T) {
	t.Run("Generate MyField", func(t *testing.T) {
		var ctx = Context{}
		GenerateField(sdkTest.CreateRandMock(seed), &ctx)
		expectedField := defaultMyField
		assert.Equal(t, expectedField, ctx.MyField)
	})
}

func testRequest(t *testing.T, ctx *Context, request *sdk.Request, expectedState State, expectedResponse *sdk.Response) {
	game := NewBattleshipGame(sdkTest.CreateRandMock(seed), false)
	response, err := game.Handle(logger, ctx, request, sdkTest.DefaultMeta())
	response.Card = nil
	assert.NoError(t, err)
	assert.Equal(t, expectedState, ctx.State)
	assert.Equal(t, expectedResponse, response)
}

func TestGame_StateGeneral(t *testing.T) {
	t.Run("no fallback", func(t *testing.T) {
		request := &sdk.Request{
			Command: "junk command",
		}
		_, ctx := CreateTestContext(true, State{})

		expectedState := State{
			GameState: replies.StartState,
		}
		expectedResponse := &sdk.Response{
			Text:    "Конечно, давайте сыграем в \"Морской бой\". Начнём?",
			Tts:     "Конечно, давайте сыграем в \"Морской бой\". Начнём?",
			Buttons: buttons.DefaultContinueButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_StateStart(t *testing.T) {
	t.Run("yes", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.YesIntent,
		}
		_, ctx := CreateTestContext(false, State{
			GameState: replies.StartState,
		})

		expectedState := State{
			GameState:        replies.UserShootState,
			Status:           PlayStatus,
			MyField:          defaultMyField,
			UserField:        DefaultUserField(),
			Ships:            defaultShips,
			LifeShips:        10,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize * fieldSize,
			Difficulty:       NormalDifficulty,
		}
		expectedResponse := &sdk.Response{
			Text:    "Приготовьте, пожалуйста, поле. Как будете готовы, стреляйте.",
			Tts:     "Приготовьте, пожалуйста, поле. Как будете готовы, стреляйте.",
			Buttons: buttons.DefaultGameButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("no", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.NoIntent,
		}
		_, ctx := CreateTestContext(false, State{
			GameState: replies.StartState,
		})

		expectedState := State{
			GameState: replies.EndGameState,
		}
		expectedResponse := &sdk.Response{
			Text:       "Если захотите сыграть ещё раз, просто скажите \"Алиса, давай сыграем в морской бой\".",
			Tts:        "Если захотите сыграть ещё раз, просто скажите \"Алиса, давай сыграем в морской бой\".",
			EndSession: true,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("junk", func(t *testing.T) {
		request := &sdk.Request{
			Command: "junk command",
		}
		_, ctx := CreateTestContext(false, State{
			GameState: replies.StartState,
		})

		expectedState := State{
			GameState: replies.StartState,
		}
		expectedResponse := &sdk.Response{
			Text: "Извините, я вас не поняла.",
			Tts:  "Извините, я вас не поняла.",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_MatchGlobalCommands(t *testing.T) {
	t.Run("start", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.StartIntent,
		}
		_, ctx := CreateTestContext(false, State{})

		expectedState := State{
			GameState: replies.StartState,
		}
		expectedResponse := &sdk.Response{
			Text:    "Конечно, давайте сыграем в \"Морской бой\". Начнём?",
			Tts:     "Конечно, давайте сыграем в \"Морской бой\". Начнём?",
			Buttons: buttons.DefaultContinueButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("restart", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.RestartIntent,
		}
		_, ctx := CreateTestContext(false, State{})

		expectedState := State{
			GameState: replies.StartState,
		}
		expectedResponse := &sdk.Response{
			Text:    "Хорошо, давайте сыграем заново. Начинаем?",
			Tts:     "Хорошо, давайте сыграем заново. Начинаем?",
			Buttons: buttons.DefaultContinueButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("rules", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.RulesIntent,
		}
		_, ctx := CreateTestContext(false, State{})

		expectedState := State{}
		expectedResponse := &sdk.Response{
			Text: "Напоминаю правила: в начале игры каждый из игроков размещает флот кораблей на своем игровом поле — квадрат 10×10.\n" +
				"Горизонтали нумеруются сверху вниз от 1 до 10, а вертикали помечаются буквами слева направо от А до К. " +
				"Всего мы выставляем по 10 кораблей: 1 четырёхпалубный корабль, 2 трёхпалубных корабля, 3 двухпалубных корабля, 4 однопалубных корабля. " +
				"Корабли не должны иметь клетки, которые нахоядстя рядом, даже по диагонали.\n" +
				"Затем мы по очереди стреляем и стараемся потопить чужие корабли. Ход игрока заканчивается, когда он промахивается.\n" +
				"Во время своего хода можно назвать букву и число или имя и число. Во втором случае я возьму первую букву из имени вместо буквы.",
			Tts: "Напоминаю правила: в начале игры каждый из игроков размещает флот кораблей на своем игровом поле — квадрат 10×10.\n" +
				"Горизонтали нумеруются сверху вниз от 1 до 10, а вертикали помечаются буквами слева направо от А до К. " +
				"Всего мы выставляем по 10 кораблей: 1 четырёхпалубный корабль, 2 трёхпалубных корабля, 3 двухпалубных корабля, 4 однопалубных корабля. " +
				"Корабли не должны иметь клетки, которые нахоядстя рядом, даже по диагонали.\n" +
				"Затем мы по очереди стреляем и стараемся потопить чужие корабли. Ход игрока заканчивается, когда он промахивается.\n" +
				"Во время своего хода можно назвать букву и число или имя и число. Во втором случае я возьму первую букву из имени вместо буквы.",
			Buttons: buttons.DefaultGameButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("wait", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.WaitIntent,
		}
		_, ctx := CreateTestContext(false, State{})

		expectedState := State{}
		expectedResponse := &sdk.Response{
			Text:    "Хорошо. Я подожду.",
			Tts:     "Хорошо. Я подожду.",
			Buttons: buttons.DefaultGameButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("surrender", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.SurrenderIntent,
		}
		_, ctx := CreateTestContext(false, State{})

		expectedState := State{
			GameState: replies.StartState,
			Status:    SurrenderedStatus,
		}
		expectedResponse := &sdk.Response{
			Text:    "К сожалению, вы сдались. Хотите сыграть ещё раз?",
			Tts:     "К сожалению, вы сдались. Хотите сыграть ещё раз?",
			Buttons: buttons.DefaultContinueButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("end", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.EndIntent,
		}
		_, ctx := CreateTestContext(false, State{})

		expectedState := State{
			GameState: replies.EndGameState,
		}
		expectedResponse := &sdk.Response{
			Text:       "Если захотите сыграть ещё раз, просто скажите \"Алиса, давай сыграем в морской бой\".",
			Tts:        "Если захотите сыграть ещё раз, просто скажите \"Алиса, давай сыграем в морской бой\".",
			EndSession: true,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_StateMyShoot(t *testing.T) {
	t.Run("kill", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.KillIntent,
		}
		_, ctx := CreateTestContext(false, State{
			GameState:        replies.MyShootState,
			UserField:        DefaultUserField(),
			MyField:          defaultMyField,
			Ships:            defaultShips,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize * fieldSize,
			Status:           PlayStatus,
		})
		expectedState := State{
			GameState:        replies.MyShootState,
			Status:           PlayStatus,
			MyField:          defaultMyField,
			UserField:        DefaultUserField(),
			Ships:            defaultShips,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize*fieldSize - 4,
			LastShoot:        ships.Cell{X: 0, Y: 2},
		}
		expectedState.UserField[0][0] = ships.KilledCell
		expectedState.UserField[0][1] = ships.NearShipCell
		expectedState.UserField[1][0] = ships.NearShipCell
		expectedState.UserField[1][1] = ships.NearShipCell
		expectedResponse := &sdk.Response{
			Text:    "Ура!\nМой выстрел: «В» 1",
			Tts:     "Ура!\nМой выстрел: вээ 1",
			Buttons: buttons.AnswersOnShootingButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("injured", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.InjuredIntent,
		}
		_, ctx := CreateTestContext(false, State{
			GameState:        replies.MyShootState,
			UserField:        DefaultUserField(),
			MyField:          defaultMyField,
			Ships:            defaultShips,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize * fieldSize,
			Status:           PlayStatus,
		})
		expectedState := State{
			GameState:         replies.MyShootState,
			Status:            PlayStatus,
			MyField:           defaultMyField,
			UserField:         DefaultUserField(),
			Ships:             defaultShips,
			UserShipsCounter:  defaultUserShipsCounter,
			IsLastShootInjury: true,
			UserEmptyCells:    fieldSize*fieldSize - 1,
			LastShoot:         ships.Cell{X: 0, Y: 1},
		}
		expectedState.UserField[0][0] = ships.InjuredCell
		expectedResponse := &sdk.Response{
			Text:    "Это было несложно.\nМой выстрел: «Б» 1",
			Tts:     "Это было несложно.\nМой выстрел: б 1",
			Buttons: buttons.AnswersOnShootingButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("clever_injured", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.InjuredIntent,
		}
		_, ctx := CreateTestContext(false, State{
			GameState:        replies.MyShootState,
			UserField:        DefaultUserField(),
			MyField:          defaultMyField,
			Ships:            defaultShips,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize * fieldSize,
			Difficulty:       NormalDifficulty,
			Status:           PlayStatus,
			LastShoot:        ships.Cell{X: 5, Y: 5},
		})
		expectedState := State{
			GameState:         replies.MyShootState,
			Status:            PlayStatus,
			MyField:           defaultMyField,
			UserField:         DefaultUserField(),
			Ships:             defaultShips,
			UserShipsCounter:  defaultUserShipsCounter,
			UserEmptyCells:    fieldSize*fieldSize - 1,
			Difficulty:        NormalDifficulty,
			IsLastShootInjury: true,
			LastShoot:         ships.Cell{X: 4, Y: 5},
			InjuryLastShoot:   ships.Cell{X: 5, Y: 5},
		}
		expectedState.UserField[5][5] = ships.InjuredCell
		expectedResponse := &sdk.Response{
			Text:    "Это было несложно.\nМой выстрел: «Е» 5",
			Tts:     "Это было несложно.\nМой выстрел: ye 5",
			Buttons: buttons.AnswersOnShootingButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("away", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.AwayIntent,
		}
		_, ctx := CreateTestContext(false, State{
			GameState:        replies.MyShootState,
			UserField:        DefaultUserField(),
			MyField:          defaultMyField,
			Ships:            defaultShips,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize * fieldSize,
			Status:           PlayStatus,
		})
		expectedState := State{
			GameState:        replies.UserShootState,
			Status:           PlayStatus,
			MyField:          defaultMyField,
			UserField:        DefaultUserField(),
			Ships:            defaultShips,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize*fieldSize - 1,
		}
		expectedState.UserField[0][0] = ships.AwayShipCell
		expectedResponse := &sdk.Response{
			Text:    "Как жаль. Ваша очередь стрелять.",
			Tts:     "Как жаль. Ваша очередь стрелять.",
			Buttons: buttons.DefaultGameButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("win_false", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.WinIntent,
		}
		_, ctx := CreateTestContext(false, State{
			GameState:        replies.MyShootState,
			Status:           PlayStatus,
			MyField:          defaultMyField,
			UserField:        DefaultUserField(),
			Ships:            defaultShips,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize * fieldSize,
		})
		expectedState := State{
			GameState:        replies.MyShootState,
			Status:           PlayStatus,
			MyField:          defaultMyField,
			UserField:        DefaultUserField(),
			Ships:            defaultShips,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize * fieldSize,
		}
		expectedResponse := &sdk.Response{
			Text:    "Пока флот полностью не потоплен, вы можете продолжать игру.",
			Tts:     "Пока флот полностью не потоплен, вы можете продолжать игру.",
			Buttons: buttons.DefaultGameButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("win_true", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.WinIntent,
		}
		_, ctx := CreateTestContext(false, State{
			GameState:        replies.MyShootState,
			Status:           PlayStatus,
			MyField:          defaultMyField,
			UserField:        DefaultUserField(),
			Ships:            defaultShips,
			UserShipsCounter: loseUserShipsCounter,
			UserEmptyCells:   fieldSize * fieldSize,
		})
		expectedState := State{
			GameState:        replies.StartState,
			Status:           LoseStatus,
			MyField:          defaultMyField,
			UserField:        DefaultUserField(),
			Ships:            defaultShips,
			UserShipsCounter: loseUserShipsCounter,
			UserEmptyCells:   fieldSize * fieldSize,
		}
		expectedResponse := &sdk.Response{
			Text:    "Ура! Я победила! Хотите сыграть ещё раз?",
			Tts:     "Ура! Я победила! Хотите сыграть ещё раз?",
			Buttons: buttons.DefaultContinueButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_StateUserShoot(t *testing.T) {

	t.Run("fallback", func(t *testing.T) {
		request := &sdk.Request{
			Command: "junk command",
		}
		_, ctx := CreateTestContext(false, State{
			GameState: replies.UserShootState,
		})

		expectedState := State{
			GameState: replies.UserShootState,
		}
		expectedResponse := &sdk.Response{
			Text: "Извините, я вас не поняла.",
			Tts:  "Извините, я вас не поняла.",
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("ShootIntent", func(t *testing.T) {
		request := &sdk.Request{
			Command: "А 1",
		}
		_, ctx := CreateTestContext(false, State{
			GameState: replies.UserShootState,
			MyField:   defaultMyField,
			Ships:     defaultShips,
		})

		expectedState := State{
			GameState: replies.UserShootState,
			MyField:   defaultMyField,
			Ships:     defaultShips,
		}
		expectedResponse := &sdk.Response{
			Text:    "Ранен. Стреляйте дальше.",
			Tts:     "Ранен. Стреляйте дальше.",
			Buttons: buttons.DefaultGameButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("ReShooting", func(t *testing.T) {
		request := &sdk.Request{
			Command: "А 1",
		}
		_, ctx := CreateTestContext(false, State{
			GameState: replies.UserShootState,
			MyField:   defaultMyField,
			Ships:     defaultShips,
		})

		expectedState := State{
			GameState: replies.UserShootState,
			MyField:   defaultMyField,
			Ships:     defaultShips,
		}
		expectedResponse := &sdk.Response{
			Text:    "Вы уже стреляли в эту клетку!\nИ тут стояла часть моего корабля. Поменяйте ход, пожалуйста.",
			Tts:     "Вы уже стреляли в эту клетку!\nИ тут стояла часть моего корабля. Поменяйте ход, пожалуйста.",
			Buttons: buttons.DefaultGameButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})

	t.Run("FioShootIntent", func(t *testing.T) {
		request := &sdk.Request{
			Command: "Алексей 6",
		}
		_, ctx := CreateTestContext(false, State{
			GameState: replies.UserShootState,
			MyField:   defaultMyField,
			Ships:     defaultShips,
			LifeShips: 10,
		})

		expectedState := State{
			GameState: replies.UserShootState,
			MyField:   defaultMyField,
			Ships:     defaultShips,
			LifeShips: 9,
		}
		expectedResponse := &sdk.Response{
			Text:    "Вы убили мой корабль. Ваш ход.",
			Tts:     "Вы убили мой корабль. Ваш ход.",
			Buttons: buttons.DefaultGameButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}

func TestGame_StateUserCheating(t *testing.T) {
	t.Run("WrongShipSequence", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.KillIntent,
		}
		_, ctx := CreateTestContext(false, State{
			GameState:        replies.MyShootState,
			UserField:        DefaultUserField(),
			MyField:          defaultMyField,
			Ships:            defaultShips,
			UserShipsCounter: []int{0, 3, 2, 1},
			UserEmptyCells:   fieldSize * fieldSize,
			Status:           PlayStatus,
			LastShoot:        ships.Cell{X: 0, Y: 0},
		})
		expectedState := State{
			GameState:        replies.StartState,
			Status:           CheatingStatus,
			MyField:          defaultMyField,
			UserField:        DefaultUserField(),
			Ships:            defaultShips,
			UserShipsCounter: []int{-1, 3, 2, 1},
			UserEmptyCells:   fieldSize*fieldSize - 4,
		}
		expectedState.UserField[0][0] = ships.KilledCell
		expectedState.UserField[0][1] = ships.NearShipCell
		expectedState.UserField[1][0] = ships.NearShipCell
		expectedState.UserField[1][1] = ships.NearShipCell
		expectedResponse := &sdk.Response{
			Text:    "Кажется, у ваc неправильный набор кораблей на поле.\nК сожалению, вы проиграли. Хотите сыграть ещё раз?",
			Tts:     "Кажется, у ваc неправильный набор кораблей на поле.\nК сожалению, вы проиграли. Хотите сыграть ещё раз?",
			Buttons: buttons.DefaultContinueButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("LargeShip", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.KillIntent,
		}
		_, ctx := CreateTestContext(false, State{
			GameState:        replies.MyShootState,
			UserField:        DefaultUserField(),
			MyField:          defaultMyField,
			Ships:            defaultShips,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize*fieldSize - 5,
			Status:           PlayStatus,
			LastShoot:        ships.Cell{X: 0, Y: 5},
		})
		for i := 0; i < 5; i++ {
			ctx.UserField[0][i] = ships.InjuredCell
		}
		expectedState := State{
			GameState:        replies.StartState,
			Status:           CheatingStatus,
			MyField:          defaultMyField,
			UserField:        DefaultUserField(),
			Ships:            defaultShips,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize*fieldSize - 5,
			LastShoot:        ships.Cell{X: 0, Y: 5},
		}
		for i := 0; i < 5; i++ {
			expectedState.UserField[0][i] = ships.InjuredCell
		}
		expectedResponse := &sdk.Response{
			Text:    "Похоже, что вы поставили слишком большой корабль на свое поле.\nК сожалению, вы проиграли. Хотите сыграть ещё раз?",
			Tts:     "Похоже, что вы поставили слишком большой корабль на свое поле.\nК сожалению, вы проиграли. Хотите сыграть ещё раз?",
			Buttons: buttons.DefaultContinueButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
	t.Run("WrongInjury", func(t *testing.T) {
		request := &sdk.Request{
			Command: patterns.InjuredIntent,
		}
		_, ctx := CreateTestContext(false, State{
			GameState:        replies.MyShootState,
			UserField:        DefaultUserField(),
			MyField:          defaultMyField,
			Ships:            defaultShips,
			UserShipsCounter: defaultUserShipsCounter,
			UserEmptyCells:   fieldSize*fieldSize - 3,
			Status:           PlayStatus,
			LastShoot:        ships.Cell{X: 0, Y: 5},
			Difficulty:       NormalDifficulty,
		})
		ctx.UserField[0][3] = ships.AwayShipCell
		ctx.UserField[0][4] = ships.InjuredCell
		ctx.UserField[0][6] = ships.AwayShipCell
		expectedState := State{
			GameState:         replies.MyShootState,
			Status:            PlayStatus,
			MyField:           defaultMyField,
			UserField:         DefaultUserField(),
			Ships:             defaultShips,
			UserShipsCounter:  defaultUserShipsCounter,
			UserEmptyCells:    fieldSize*fieldSize - 8,
			LastShoot:         ships.Cell{X: 0, Y: 0},
			InjuryLastShoot:   ships.Cell{X: 0, Y: 5},
			IsLastShootInjury: false,
			Difficulty:        NormalDifficulty,
		}
		expectedState.UserField[0][3] = ships.NearShipCell
		expectedState.UserField[0][4] = ships.KilledCell
		expectedState.UserField[0][5] = ships.KilledCell
		expectedState.UserField[0][6] = ships.NearShipCell
		expectedState.UserField[1][3] = ships.NearShipCell
		expectedState.UserField[1][4] = ships.NearShipCell
		expectedState.UserField[1][5] = ships.NearShipCell
		expectedState.UserField[1][6] = ships.NearShipCell
		expectedResponse := &sdk.Response{
			Text: "Это было несложно.\nВаш корабль ранен, но все клетки рядом с ним не могут содержать часть корабля." +
				" Будем считать, что я потопила ваш корабль.\nМой выстрел: «А» 1",
			Tts: "Это было несложно.\nВаш корабль ранен, но все клетки рядом с ним не могут содержать часть корабля." +
				" Будем считать, что я потопила ваш корабль.\nМой выстрел: а 1",
			Buttons: buttons.AnswersOnShootingButtons,
		}

		testRequest(t, ctx, request, expectedState, expectedResponse)
	})
}
