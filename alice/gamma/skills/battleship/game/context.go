package game

import (
	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/replies"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/ships"
)

const fieldSize = 10

type State struct {
	Difficulty        string
	GameState         string
	Status            string
	MyField           [][]ships.MyCell
	UserField         [][]ships.UserCell
	Ships             []ships.Ship
	LastShoot         ships.Cell
	InjuryLastShoot   ships.Cell
	IsLastShootInjury bool
	UserShipsCounter  []int
	UserEmptyCells    int
	LifeShips         int
}

type Context struct {
	sdk.Context
	State
}

func (ctx *Context) GetState() interface{} {
	return &ctx.State
}

func CreateContext(context sdk.Context) *Context {
	return &Context{
		State: State{
			GameState: replies.GeneralState,
		},
		Context: context,
	}
}
