package game

import (
	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/akinator/resources/replies"
)

type RoundState struct {
	LastActorIndex   int
	Hints            []string
	CountOfUsedHints int
	CurrentHint      string
	WantsKnowAnswer  bool
}

type State struct {
	GameState         string
	UsedActorsIndex   []int
	RoundState        RoundState
	CountOfUsedActors int
	IsPromo           bool
}
type Context struct {
	sdk.Context
	State
}

func (context *Context) GetState() interface{} {
	return &context.State
}
func CreateContext(context sdk.Context) *Context {
	return &Context{
		State: State{
			GameState:       replies.GeneralState,
			UsedActorsIndex: make([]int, 0),
			RoundState: RoundState{
				LastActorIndex: -1,
				Hints:          make([]string, 0),
			},
		},
		Context: context,
	}
}
