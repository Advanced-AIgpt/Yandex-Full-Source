package game

import (
	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/song_search/api"
)

type State struct {
	GameState     string
	Results       []api.SearchResult
	LastIndex     int
	IsFirstSearch bool
}

func (state *State) ResetSearchResults() {
	state.Results = nil
	state.LastIndex = -1
}

type Context struct {
	State
	sdk.Context
}

func (context *Context) GetState() interface{} {
	return &context.State
}

func CreateContext(context sdk.Context) *Context {
	return &Context{
		State:   State{},
		Context: context,
	}
}
