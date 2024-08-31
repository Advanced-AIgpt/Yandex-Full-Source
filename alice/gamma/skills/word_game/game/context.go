package game

import (
	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/replies"
)

type Context struct {
	State
	sdk.Context
}

func (context *Context) GetState() interface{} {
	return &context.State
}

func CreateContext(context sdk.Context, MainWords int) *Context {
	return &Context{
		State: State{
			GameState:            replies.GeneralState,
			UsedMainWordsIndexes: make(map[int]bool),
			LeftMainWords:        MainWords,
			RoundState: RoundState{
				GuessedWords: 0,
			},
		},
		Context: context,
	}
}
