package game

import (
	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/replies"
)

type Context struct {
	State
	sdk.Context
}

func (context *Context) GetState() interface{} {
	return &context.State
}

func CreateContext(context sdk.Context) *Context {
	return &Context{
		State: State{
			GameState:           replies.GeneralState,
			UsedQuestionIndexes: make(map[int]bool),
		},
		Context: context,
	}
}
