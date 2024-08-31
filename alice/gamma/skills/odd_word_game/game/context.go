package game

import sdk "a.yandex-team.ru/alice/gamma/sdk/golang"

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
			RoundState: RoundState{},
		},
		Context: context,
	}
}
