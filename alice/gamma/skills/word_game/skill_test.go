package main

import (
	"math/rand"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/word_game/game"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/replies"
)

var matchesMap = map[string][]sdk.Hypothesis{
	// start game patterns
	patterns.StartGameIntent: {
		{Name: patterns.StartGameIntent},
	},
	//endgame patterns
	patterns.EndGameIntent: {
		{Name: patterns.EndGameIntent},
	},
}

var skill *WordSkill

var logger sdk.Logger

func toState(context sdk.Context) *game.State {
	return &context.(*game.Context).State
}

func TestMain(m *testing.M) {
	source := rand.NewSource(time.Now().Unix())
	random := rand.New(source)
	skill = NewWordSkill(*random)
	logger = sdkTest.CreateTestLogger()
}

func TestStartGame(t *testing.T) {
	testContext := sdkTest.CreateTestContext(true, matchesMap)
	context := skill.CreateContext(testContext)

	// test start game intent
	request := &sdk.Request{
		Command: patterns.StartGameIntent,
	}

	response, err := skill.Handle(logger, context, request, nil)
	if err != nil {
		t.Error(err)
	}
	assert.False(t, response.EndSession)

	expectedState := game.State{
		GameState: replies.StartGameState,
	}
	state := toState(context)
	assert.Equal(t, expectedState, state)
}
